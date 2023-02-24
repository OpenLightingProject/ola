/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * UsbProWidgetDetector.cpp
 * Handles the discovery process for widgets that implement the Usb Pro frame
 * format.
 * Copyright (C) 2010 Simon Newton
 *
 * This class accepts a ConnectedDescriptor objects and runs the discovery
 * process to determine if the widget behaves like a Usb Pro device.
 *
 * The discovery process sends the following request messages:
 *   - MANUFACTURER_LABEL
 *   - DEVICE_LABEL
 *   - SERIAL_LABEL
 *   - HARDWARE_VERSION (optional)
 *
 * Requests are sent at an interval specified by message_interval in the
 * constructor. Of these, the only message a widget must respond to is
 * SERIAL_LABEL. The other two messages are part of the Usb Pro Extensions
 * (https://wiki.openlighting.org/index.php/USB_Protocol_Extensions) and allow
 * us to determine more specifically what type of device this is.
 *
 * If the widget responds to SERIAL_LABEL the on_success callback is run.
 * Otherwise on_failure is run. It's important you register callbacks for each
 * of these otherwise you'll leak ConnectedDescriptor objects.
 */


#include <string.h>

#include <string>
#include <ostream>

#include "ola/Logging.h"
#include "ola/io/Descriptor.h"
#include "ola/network/NetworkUtils.h"
#include "ola/strings/Format.h"
#include "ola/util/Utils.h"
#include "plugins/usbpro/BaseUsbProWidget.h"
#include "plugins/usbpro/GenericUsbProWidget.h"
#include "plugins/usbpro/UsbProWidgetDetector.h"

namespace ola {
namespace plugin {
namespace usbpro {

using ola::strings::ToHex;
using std::string;


UsbProWidgetInformation& UsbProWidgetInformation::operator=(
    const UsbProWidgetInformation &other) {
  esta_id = other.esta_id;
  device_id = other.device_id;
  manufacturer = other.manufacturer;
  device = other.device;
  serial = other.serial;
  has_firmware_version = other.has_firmware_version;
  firmware_version = other.firmware_version;
  return *this;
}


/**
 * Constructor
 * @param scheduler a SchedulingExecutorInterface to use to register events.
 * @param on_success A callback to run if discovery succeeds.
 * @param on_failure A callback to run if discovery fails.
 * @param message_interval the time in ms between each discovery message.
 */
UsbProWidgetDetector::UsbProWidgetDetector(
    ola::thread::SchedulingExecutorInterface *scheduler,
    SuccessHandler *on_success,
    FailureHandler *on_failure,
    unsigned int message_interval)
    : m_scheduler(scheduler),
      m_callback(on_success),
      m_failure_callback(on_failure),
      m_timeout_ms(message_interval) {
  if (!on_success) {
    OLA_WARN << "on_success callback not set, this will leak memory!";
  }
  if (!on_failure) {
    OLA_WARN << "on_failure callback not set, this will leak memory!";
  }
}


/*
 * Fail any widgets that are still in the discovery process.
 */
UsbProWidgetDetector::~UsbProWidgetDetector() {
  m_scheduler->DrainCallbacks();

  WidgetStateMap::iterator iter;
  for (iter = m_widgets.begin(); iter != m_widgets.end(); ++iter) {
    iter->first->GetDescriptor()->SetOnClose(NULL);
    if (m_failure_callback.get()) {
      m_failure_callback->Run(iter->first->GetDescriptor());
    }
    RemoveTimeout(&iter->second);
  }
  m_widgets.clear();
}


/*
 * Start the discovery process for a widget
 * @param widget, the DispatchingUsbProWidget to run discovery on.
 * @return true if the process started ok, false otherwise.
 */
bool UsbProWidgetDetector::Discover(
    ola::io::ConnectedDescriptor *descriptor) {
  DispatchingUsbProWidget *widget = new DispatchingUsbProWidget(
      descriptor,
      NULL);
  widget->SetHandler(
      NewCallback(this, &UsbProWidgetDetector::HandleMessage, widget));

  if (!widget->SendMessage(BaseUsbProWidget::MANUFACTURER_LABEL, NULL, 0)) {
    delete widget;
    return false;
  }

  // Set the onclose handler so we can mark this as failed.
  descriptor->SetOnClose(
      NewSingleCallback(this, &UsbProWidgetDetector::WidgetRemoved, widget));

  // register a timeout for this widget
  SetupTimeout(widget, &m_widgets[widget]);
  return true;
}


/*
 * Called by the widgets when they receive a response.
 */
void UsbProWidgetDetector::HandleMessage(DispatchingUsbProWidget *widget,
                                         uint8_t label,
                                         const uint8_t *data,
                                         unsigned int length) {
  switch (label) {
    case BaseUsbProWidget::MANUFACTURER_LABEL:
      HandleIdResponse(widget, length, data, false);
      break;
    case BaseUsbProWidget::DEVICE_LABEL:
      HandleIdResponse(widget, length, data, true);
      break;
    case BaseUsbProWidget::SERIAL_LABEL:
      HandleSerialResponse(widget, length, data);
      break;
    case BaseUsbProWidget::GET_PARAMS:
      HandleGetParams(widget, length, data);
      break;
    case BaseUsbProWidget::HARDWARE_VERSION_LABEL:
      HandleHardwareVersionResponse(widget, length, data);
      break;
    case ENTTEC_SNIFFER_LABEL:
      HandleSnifferPacket(widget);
      break;
    case GenericUsbProWidget::RECEIVED_DMX_LABEL:
      break;
    default:
      OLA_WARN << "Unknown response label: " << ToHex(label) << ", length "
               << length;
  }
}


/**
 * Called if the widget is removed mid-discovery process
 */
void UsbProWidgetDetector::WidgetRemoved(DispatchingUsbProWidget *widget) {
  WidgetStateMap::iterator iter = m_widgets.find(widget);
  if (iter == m_widgets.end()) {
    OLA_FATAL << "Widget " << widget
              << " removed but it doesn't exist in the widget map";
  } else {
    RemoveTimeout(&(iter->second));
    m_widgets.erase(iter);
  }

  ola::io::ConnectedDescriptor *descriptor = widget->GetDescriptor();
  delete widget;
  descriptor->SetOnClose(NULL);
  descriptor->Close();
  if (m_failure_callback.get()) {
    m_failure_callback->Run(descriptor);
  }
}


/**
 * Setup a timeout for a widget
 */
void UsbProWidgetDetector::SetupTimeout(DispatchingUsbProWidget *widget,
                                        DiscoveryState *discovery_state) {
  discovery_state->timeout_id = m_scheduler->RegisterSingleTimeout(
      m_timeout_ms,
      NewSingleCallback(this, &UsbProWidgetDetector::DiscoveryTimeout, widget));
}


/**
 * Remove a timeout for a widget.
 */
void UsbProWidgetDetector::RemoveTimeout(DiscoveryState *discovery_state) {
  if (discovery_state->timeout_id != ola::thread::INVALID_TIMEOUT) {
    m_scheduler->RemoveTimeout(discovery_state->timeout_id);
  }
}


/**
 * Send a DEVICE_LABEL request
 */
void UsbProWidgetDetector::SendNameRequest(DispatchingUsbProWidget *widget) {
  widget->SendMessage(DispatchingUsbProWidget::DEVICE_LABEL, NULL, 0);
  DiscoveryState &discovery_state = m_widgets[widget];
  discovery_state.discovery_state = DiscoveryState::DEVICE_SENT;
  SetupTimeout(widget, &discovery_state);
}


/**
 * Set a SERIAL_LABEL request
 */
void UsbProWidgetDetector::SendSerialRequest(DispatchingUsbProWidget *widget) {
  widget->SendMessage(DispatchingUsbProWidget::SERIAL_LABEL, NULL, 0);
  DiscoveryState &discovery_state = m_widgets[widget];
  discovery_state.discovery_state = DiscoveryState::SERIAL_SENT;
  SetupTimeout(widget, &discovery_state);
}

/**
 * Set a GET_PARAMS request
 */
void UsbProWidgetDetector::SendGetParams(DispatchingUsbProWidget *widget) {
  uint16_t data = 0;
  widget->SendMessage(DispatchingUsbProWidget::GET_PARAMS,
                      reinterpret_cast<uint8_t*>(&data), sizeof(data));
  DiscoveryState &discovery_state = m_widgets[widget];
  discovery_state.discovery_state = DiscoveryState::GET_PARAM_SENT;
  SetupTimeout(widget, &discovery_state);
}

/**
 * Send a Hardware version message, this is only valid for Enttec Usb Pro MkII
 * widgets.
 */
void UsbProWidgetDetector::MaybeSendHardwareVersionRequest(
    DispatchingUsbProWidget *widget) {
  WidgetStateMap::iterator iter = m_widgets.find(widget);
  if (iter == m_widgets.end()) {
    return;
  }

  UsbProWidgetInformation &information = iter->second.information;
  if (information.esta_id == 0 && information.device_id == 0) {
    // This widget didn't respond to Manufacturer or Device messages,
    // but did respond to GetSerial, so it's probably a USB Pro. Now we
    // need to check if it's a MK II widget.
    widget->SendMessage(
        DispatchingUsbProWidget::HARDWARE_VERSION_LABEL, NULL, 0);
    DiscoveryState &discovery_state = m_widgets[widget];
    discovery_state.discovery_state = DiscoveryState::HARDWARE_VERSION_SENT;
    SetupTimeout(widget, &discovery_state);
  } else {
    // otherwise there are no more messages to send.
    CompleteWidgetDiscovery(widget);
  }
}


/**
 * Send OLA's API key to unlock the second port of a Usb Pro MkII Widget.
 * Note: The labels for the messages used to control the 2nd port of the Mk II
 * depend on this key value. If you're writing other software you can obtain a
 * key by emailing Enttec, rather than just copying the value here.
 */
void UsbProWidgetDetector::SendAPIRequest(DispatchingUsbProWidget *widget) {
  uint32_t key = ola::network::LittleEndianToHost(USB_PRO_MKII_API_KEY);
  widget->SendMessage(USB_PRO_MKII_API_LABEL, reinterpret_cast<uint8_t*>(&key),
                      sizeof(key));
}


/*
 * Called if a widget fails to respond in a given interval
 */
void UsbProWidgetDetector::DiscoveryTimeout(DispatchingUsbProWidget *widget) {
  WidgetStateMap::iterator iter = m_widgets.find(widget);

  if (iter != m_widgets.end()) {
    iter->second.timeout_id = ola::thread::INVALID_TIMEOUT;
    switch (iter->second.discovery_state) {
      case DiscoveryState::MANUFACTURER_SENT:
        SendNameRequest(widget);
        break;
      case DiscoveryState::DEVICE_SENT:
        SendSerialRequest(widget);
        break;
      case DiscoveryState::GET_PARAM_SENT:
        MaybeSendHardwareVersionRequest(widget);
        break;
      case DiscoveryState::HARDWARE_VERSION_SENT:
        CompleteWidgetDiscovery(widget);
        break;
      default:
        OLA_WARN << "USB Widget didn't respond to messages, esta id "
                 << iter->second.information.esta_id << ", device id "
                 << iter->second.information.device_id;
        OLA_WARN << "Is device in USB Controller mode if it's a Goddard?";
        ola::io::ConnectedDescriptor *descriptor =
            widget->GetDescriptor();
        descriptor->SetOnClose(NULL);
        delete widget;
        if (m_failure_callback.get()) {
          m_failure_callback->Run(descriptor);
        }
        m_widgets.erase(iter);
    }
  }
}


/*
 * Handle a Device Manufacturer or Device Name response.
 * @param widget the DispatchingUsbProWidget
 * @param length length of the response
 * @param data pointer to the response data
 * @param is_device true if this is a device response, false if it's a
 *   manufacturer response.
 */
void UsbProWidgetDetector::HandleIdResponse(DispatchingUsbProWidget *widget,
                                            unsigned int length,
                                            const uint8_t *data,
                                            bool is_device) {
  struct {
    uint8_t id_low;
    uint8_t id_high;
    char text[32];
    uint8_t terminator;
  } response;
  memset(&response, 0, sizeof(response));
  memcpy(&response, data, length);

  WidgetStateMap::iterator iter = m_widgets.find(widget);

  if (iter == m_widgets.end()) {
    return;
  }

  uint16_t id = ola::utils::JoinUInt8(response.id_high, response.id_low);
  if (length < sizeof(id)) {
    OLA_WARN << "Received small response packet";
    return;
  }

  if (is_device) {
    iter->second.information.device_id = id;
    iter->second.information.device = string(response.text);
    if (iter->second.discovery_state == DiscoveryState::DEVICE_SENT) {
      RemoveTimeout(&iter->second);
      SendSerialRequest(widget);
    }
  } else {
    iter->second.information.esta_id = id;
    iter->second.information.manufacturer = string(response.text);
    if (iter->second.discovery_state == DiscoveryState::MANUFACTURER_SENT) {
      RemoveTimeout(&iter->second);
      SendNameRequest(widget);
    }
  }
}


/*
 * Handle a serial response, this ends the device detection phase.
 */
void UsbProWidgetDetector::HandleSerialResponse(
    DispatchingUsbProWidget *widget,
    unsigned int length,
    const uint8_t *data) {
  WidgetStateMap::iterator iter = m_widgets.find(widget);
  if (iter == m_widgets.end()) {
    return;
  }
  RemoveTimeout(&iter->second);
  UsbProWidgetInformation information = iter->second.information;

  if (length == sizeof(information.serial)) {
    UsbProWidgetInformation::DeviceSerialNumber serial;
    memcpy(reinterpret_cast<uint8_t*>(&serial), data, sizeof(serial));
    iter->second.information.serial = ola::network::LittleEndianToHost(serial);
  } else {
    OLA_WARN << "Serial number response size " << length << " != "
             << sizeof(information.serial);
  }

  SendGetParams(widget);
}

void UsbProWidgetDetector::HandleGetParams(DispatchingUsbProWidget *widget,
                                           unsigned int length,
                                           const uint8_t *data) {
  WidgetStateMap::iterator iter = m_widgets.find(widget);
  if (iter == m_widgets.end()) {
    return;
  }

  struct widget_params {
    uint8_t firmware_lo;
    uint8_t firmware_hi;
    uint8_t break_time;
    uint8_t mab_time;
    uint8_t output_rate;
  };

  if (length < sizeof(widget_params)) {
    OLA_WARN << "Response to GET_PARAMS too small, ignoring";
  } else {
    const widget_params *params = reinterpret_cast<const widget_params*>(data);
    UsbProWidgetInformation &information = iter->second.information;
    information.SetFirmware((params->firmware_hi << 8) + params->firmware_lo);
  }

  MaybeSendHardwareVersionRequest(widget);
}


/*
 * Handle a hardware version response.
 */
void UsbProWidgetDetector::HandleHardwareVersionResponse(
    DispatchingUsbProWidget *widget,
    unsigned int length,
    const uint8_t *data) {
  if (length != 1) {
    OLA_WARN << "Wrong size of hardware version response, was " << length;
    return;
  }

  OLA_DEBUG << "Hardware version response was " << ToHex(data[0]);

  WidgetStateMap::iterator iter = m_widgets.find(widget);
  if (iter == m_widgets.end()) {
    return;
  }
  RemoveTimeout(&iter->second);
  if ((data[0] == DMX_PRO_MKII_VERSION) ||
      (data[0] == DMX_PRO_MKII_B_VERSION)) {
    iter->second.information.dual_port = true;
    SendAPIRequest(widget);
  }

  CompleteWidgetDiscovery(widget);
}


/**
 * Handle a possible sniffer packet.
 * Enttec sniffers are very boisterous and continuously send frames. This
 * causes all sorts of problems and for now we don't want to use these devices.
 * We track the number of sniffer frames received and if it's more than one we
 * declare this device a sniffer.
 */
void UsbProWidgetDetector::HandleSnifferPacket(
    DispatchingUsbProWidget *widget) {
  WidgetStateMap::iterator iter = m_widgets.find(widget);

  if (iter == m_widgets.end()) {
    return;
  }
  OLA_DEBUG << "Received Enttec Sniffer Packet";
  iter->second.sniffer_packets++;
}


/**
 * Called when the last timeout expires, or we receive the final message. This
 * cleans up state and executes the m_callback in the scheduler thread.
 */
void UsbProWidgetDetector::CompleteWidgetDiscovery(
    DispatchingUsbProWidget *widget) {
  WidgetStateMap::iterator iter = m_widgets.find(widget);
  if (iter == m_widgets.end()) {
    return;
  }

  unsigned int sniffer_packets = iter->second.sniffer_packets;
  const UsbProWidgetInformation information = iter->second.information;
  m_widgets.erase(iter);

  if (sniffer_packets > 1) {
    OLA_WARN << "Enttec sniffer found (" << sniffer_packets
             << " packets), discarding";
    // we can't delete the widget here since it called us.
    // instead schedule a call in the other thread.
    widget->GetDescriptor()->SetOnData(NULL);
    m_scheduler->Execute(
        NewSingleCallback(this,
                          &UsbProWidgetDetector::HandleSniffer,
                          widget));
    return;
  }

  std::ostringstream str;
  str << "ESTA Id: " << ToHex(information.esta_id);
  if (!information.manufacturer.empty()) {
    str << " (" << information.manufacturer << ")";
  }
  str << ", device Id: " << ToHex(information.device_id);
  if (!information.device.empty()) {
    str << " (" << information.device << ")";
  }
  str << ", serial: " << ToHex(information.serial) << ", f/w version: ";
  if (information.has_firmware_version) {
     str << (information.firmware_version >> 8) << "."
         << (information.firmware_version & 0xff);
  } else {
    str << "N/A";
  }
  OLA_INFO << "Detected USB Device: " << str.str();

  const UsbProWidgetInformation *widget_info = new UsbProWidgetInformation(
      information);
  // Given that we've been called via the widget's stack, schedule execution of
  // the method that deletes the widget.
  m_scheduler->Execute(
      NewSingleCallback(this, &UsbProWidgetDetector::DispatchWidget, widget,
                        widget_info));
}


/**
 * Called once we have confirmed a new widget. This runs in the scheduler
 * thread, so it can't access any non-const member data.
 */
void UsbProWidgetDetector::DispatchWidget(
    DispatchingUsbProWidget *widget,
    const UsbProWidgetInformation *info) {
  ola::io::ConnectedDescriptor *descriptor = widget->GetDescriptor();
  descriptor->SetOnClose(NULL);
  delete widget;
  if (m_callback.get()) {
    m_callback->Run(descriptor, info);
  } else {
    delete info;
    OLA_FATAL << "No listener provided, leaking descriptors";
  }
}


/**
 * Delete a widget which we've decided belongs to a sniffer.
 */
void UsbProWidgetDetector::HandleSniffer(DispatchingUsbProWidget *widget) {
  ola::io::ConnectedDescriptor *descriptor = widget->GetDescriptor();
  delete widget;
  descriptor->SetOnClose(NULL);
  if (m_failure_callback.get()) {
    m_failure_callback->Run(descriptor);
  }
}
}  // namespace usbpro
}  // namespace plugin
}  // namespace ola
