/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
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
 *
 * Requests are sent at an interval specified by message_interval in the
 * constructor. Of these, the only message a widget must respond to is
 * SERIAL_LABEL. The other two messages are part of the Usb Pro Extensions
 * (http://www.opendmx.net/index.php/USB_Protocol_Extensions) and allow us to
 * determine more specfically what type of device this is.
 *
 * If the widget responds to SERIAL_LABEL the on_success callback is run.
 * Otherwise on_failure is run. It's important you register callbacks for each
 * of these otherwise you'll leak ConnectedDescriptor objects.
 */


#include <string.h>

#include <map>
#include <string>

#include "ola/Logging.h"
#include "ola/network/Socket.h"
#include "ola/network/NetworkUtils.h"
#include "plugins/usbpro/UsbProWidgetDetector.h"
#include "plugins/usbpro/BaseUsbProWidget.h"

namespace ola {
namespace plugin {
namespace usbpro {

UsbProWidgetInformation& UsbProWidgetInformation::operator=(
    const UsbProWidgetInformation &other) {
  esta_id = other.esta_id;
  device_id = other.device_id;
  manufacturer = other.manufacturer;
  device = other.device;
  serial = other.serial;
  return *this;
}


/**
 * Constructor
 * @param scheduler a SchedulingExecutorInterface to use to register events.
 * @param on_success A callback to run if discovery succeeds.
 * @param on_failure A callback to run if discovery fails.
 * @param timeout the time in ms between each discovery message.
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
  if (!m_callback)
    OLA_WARN << "on_success callback not set, this will leak memory!";
  if (!m_failure_callback)
    OLA_WARN << "on_failure callback not set, this will leak memory!";
}


/*
 * Fail any widgets that are still in the discovery process.
 */
UsbProWidgetDetector::~UsbProWidgetDetector() {
  WidgetStateMap::iterator iter;
  for (iter = m_widgets.begin(); iter != m_widgets.end(); ++iter) {
    iter->first->GetDescriptor()->SetOnClose(NULL);
    if (m_failure_callback)
      m_failure_callback->Run(iter->first->GetDescriptor());
    RemoveTimeout(&iter->second);
  }
  m_widgets.clear();

  if (m_callback)
    delete m_callback;
  if (m_failure_callback)
    delete m_failure_callback;
}


/*
 * Start the discovery process for a widget
 * @param widget, the DispatchingUsbProWidget to run discovery on.
 * @return true if the process started ok, false otherwise.
 */
bool UsbProWidgetDetector::Discover(
    ola::network::ConnectedDescriptor *descriptor) {
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
      NewSingleCallback(this,
                        &UsbProWidgetDetector::WidgetRemoved,
                        widget));

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
    default:
      OLA_WARN << "Unknown response label";
  }
}


/**
 * Called if the widget is removed mid-discovery process
 */
void UsbProWidgetDetector::WidgetRemoved(DispatchingUsbProWidget *widget) {
  WidgetStateMap::iterator iter = m_widgets.find(widget);
  if (iter == m_widgets.end()) {
    OLA_FATAL << "Widget " << widget <<
      " removed but it doesn't exist in the widget map";
  } else {
    RemoveTimeout(&(iter->second));
    m_widgets.erase(iter);
  }

  ola::network::ConnectedDescriptor *descriptor = widget->GetDescriptor();
  delete widget;
  descriptor->SetOnClose(NULL);
  descriptor->Close();
  if (m_failure_callback)
    m_failure_callback->Run(descriptor);
}


/**
 * Setup a timeout for a widget
 */
void UsbProWidgetDetector::SetupTimeout(DispatchingUsbProWidget *widget,
                                        DiscoveryState *discovery_state) {
  discovery_state->timeout_id = m_scheduler->RegisterSingleTimeout(
      m_timeout_ms,
      NewSingleCallback(this,
                        &UsbProWidgetDetector::DiscoveryTimeout, widget));
}


/**
 * Remove a timeout for a widget.
 */
void UsbProWidgetDetector::RemoveTimeout(DiscoveryState *discovery_state) {
  if (discovery_state->timeout_id != ola::thread::INVALID_TIMEOUT)
    m_scheduler->RemoveTimeout(discovery_state->timeout_id);
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
      default:
        OLA_WARN << "Usb Widget didn't respond to messages, esta id " <<
          iter->second.information.esta_id << ", device id " <<
          iter->second.information.device_id;
        ola::network::ConnectedDescriptor *descriptor =
          widget->GetDescriptor();
        descriptor->SetOnClose(NULL);
        delete widget;
        if (m_failure_callback)
          m_failure_callback->Run(descriptor);
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
  id_response response;
  memset(&response, 0, sizeof(response));
  memcpy(&response, data, length);

  WidgetStateMap::iterator iter = m_widgets.find(widget);

  if (iter == m_widgets.end())
    return;

  uint16_t id = (response.id_high << 8) + response.id_low;
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

  if (iter == m_widgets.end())
    return;
  RemoveTimeout(&iter->second);
  UsbProWidgetInformation information = iter->second.information;

  if (length == sizeof(information.serial)) {
    memcpy(reinterpret_cast<uint8_t*>(&information.serial),
           data,
           sizeof(information.serial));
    information.serial = ola::network::LittleEndianToHost(information.serial);
  } else {
    OLA_WARN << "Serial number response size " << length << " != " <<
      sizeof(information.serial);
  }

  m_widgets.erase(iter);

  OLA_INFO << "Detected USB Device: ESTA Id: 0x" << std::hex <<
    information.esta_id  << " (" << information.manufacturer << "), device: "
    << information.device_id << " (" << information.device << "), serial: " <<
    "0x" << information.serial;

  const UsbProWidgetInformation *widget_info = new UsbProWidgetInformation(
      information);
  // given that we've been called via the widget's stack, schedule execution of
  // the method that deletes the widget.
  m_scheduler->Execute(
      NewSingleCallback(this,
                        &UsbProWidgetDetector::DispatchWidget,
                        widget,
                        widget_info));
}


/**
 * Called once we have confirmed a new widget
 */
void UsbProWidgetDetector::DispatchWidget(
    DispatchingUsbProWidget *widget,
    const UsbProWidgetInformation *info) {
  ola::network::ConnectedDescriptor *descriptor = widget->GetDescriptor();
  descriptor->SetOnClose(NULL);
  delete widget;
  if (m_callback) {
    m_callback->Run(descriptor, info);
  } else {
    delete info;
    OLA_FATAL << "No listener provided, leaking descriptors";
  }
}
}  // usbpro
}  // plugin
}  // ola
