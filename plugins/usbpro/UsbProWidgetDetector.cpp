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
 * Handles creating UsbWidget objects.
 * Copyright (C) 2010 Simon Newton
 *
 * This class accepts UsbWidget objects and runs the discovery process
 * to determine if the widget behaves like a Usb Pro device.
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
 * If the widget responses to SERIAL_LABEL the on_success callback is run.
 * Otherwise on_failure is run. It's important you register callbacks for each
 * of these otherwise you'll leak UsbWidget objects.
 */


#include <string.h>

#include <map>
#include <string>

#include "ola/Logging.h"
#include "ola/network/Socket.h"
#include "ola/network/NetworkUtils.h"
#include "plugins/usbpro/UsbProWidgetDetector.h"
#include "plugins/usbpro/UsbWidget.h"

namespace ola {
namespace plugin {
namespace usbpro {

using std::map;

WidgetInformation& WidgetInformation::operator=(
    const WidgetInformation &other) {
  esta_id = other.esta_id;
  device_id = other.device_id;
  manufactuer = other.manufactuer;
  device = other.device;
  serial = other.serial;
  return *this;
}


/**
 * Constructor
 * @param ss a SelectServerInterface to use to register events.
 * @param on_success A callback to run if discovery succeeds.
 * @param on_failure A callback to run if discovery fails.
 * @param timeout the time in ms between each discovery message.
 */
UsbProWidgetDetector::UsbProWidgetDetector(
    ola::network::SelectServerInterface *ss,
    ola::Callback2<void, UsbWidget*, const WidgetInformation*> *on_success,
    ola::Callback1<void, UsbWidget*> *on_failure,
    unsigned int message_interval)
    : m_ss(ss),
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
  map<UsbWidget*, DiscoveryState>::iterator iter;
  for (iter = m_widgets.begin(); iter != m_widgets.end(); ++iter) {
    iter->first->SetOnRemove(NULL);
    if (m_failure_callback)
      m_failure_callback->Run(iter->first);
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
 * @param widget, the UsbWidget to run discovery on.
 * @return true if the process started ok, false otherwise.
 */
bool UsbProWidgetDetector::Discover(UsbWidget *widget) {
  widget->SetMessageHandler(
    NewCallback(this, &UsbProWidgetDetector::HandleMessage, widget));

  if (!widget->SendMessage(UsbWidget::MANUFACTURER_LABEL, NULL, 0)) {
    delete widget;
    return false;
  }

  // Set the onclose handler so we can mark this as failed.
  widget->SetOnRemove(NewSingleCallback(this,
                      &UsbProWidgetDetector::WidgetRemoved,
                      widget));

  // register a timeout for this widget
  SetupTimeout(widget, &m_widgets[widget]);
  return true;
}


/*
 * Called by the widgets when they receive a response.
 */
void UsbProWidgetDetector::HandleMessage(UsbWidget *widget,
                                         uint8_t label,
                                         const uint8_t *data,
                                         unsigned int length) {
  switch (label) {
    case UsbWidget::MANUFACTURER_LABEL:
      HandleIdResponse(widget, length, data, false);
      break;
    case UsbWidget::DEVICE_LABEL:
      HandleIdResponse(widget, length, data, true);
      break;
    case UsbWidget::SERIAL_LABEL:
      HandleSerialResponse(widget, length, data);
      break;
    default:
      OLA_WARN << "Unknown response label";
  }
}


/**
 * Called if the widget is removed mid-discovery process
 */
void UsbProWidgetDetector::WidgetRemoved(UsbWidget *widget) {
  WidgetStateMap::iterator iter = m_widgets.find(widget);
  if (iter == m_widgets.end()) {
    OLA_FATAL << "Widget " << widget <<
      " removed but it doesn't exist in the widget map";
  } else {
    RemoveTimeout(&(iter->second));
    m_widgets.erase(iter);
  }

  widget->SetOnRemove(NULL);
  if (m_failure_callback)
    m_failure_callback->Run(widget);
}


/**
 * Setup a timeout for a widget
 */
void UsbProWidgetDetector::SetupTimeout(UsbWidget *widget,
                                        DiscoveryState *discovery_state) {
  discovery_state->timeout_id = m_ss->RegisterSingleTimeout(
      m_timeout_ms,
      NewSingleCallback(this,
                        &UsbProWidgetDetector::DiscoveryTimeout, widget));
}


/**
 * Remove a timeout for a widget.
 */
void UsbProWidgetDetector::RemoveTimeout(DiscoveryState *discovery_state) {
  if (discovery_state->timeout_id != ola::network::INVALID_TIMEOUT)
    m_ss->RemoveTimeout(discovery_state->timeout_id);
}


/**
 * Send a DEVICE_LABEL request
 */
void UsbProWidgetDetector::SendNameRequest(UsbWidget *widget) {
  widget->SendMessage(UsbWidget::DEVICE_LABEL, NULL, 0);
  DiscoveryState &discovery_state = m_widgets[widget];
  discovery_state.discovery_state = DiscoveryState::DEVICE_SENT;
  SetupTimeout(widget, &discovery_state);
}


/**
 * Set a SERIAL_LABEL request
 */
void UsbProWidgetDetector::SendSerialRequest(UsbWidget *widget) {
  widget->SendMessage(UsbWidget::SERIAL_LABEL, NULL, 0);
  DiscoveryState &discovery_state = m_widgets[widget];
  discovery_state.discovery_state = DiscoveryState::SERIAL_SENT;
  SetupTimeout(widget, &discovery_state);
}


/*
 * Called if a widget fails to respond in a given interval
 */
void UsbProWidgetDetector::DiscoveryTimeout(UsbWidget *widget) {
  map<UsbWidget*, DiscoveryState>::iterator iter = m_widgets.find(widget);

  if (iter != m_widgets.end()) {
    iter->second.timeout_id = ola::network::INVALID_TIMEOUT;

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
        widget->SetOnRemove(NULL);
        if (m_failure_callback)
          m_failure_callback->Run(widget);
        m_widgets.erase(iter);
    }
  }
}


/*
 * Handle a Device Manufacturer or Device Name response.
 * @param widget the UsbWidget
 * @param length length of the response
 * @param data pointer to the response data
 * @param is_device true if this is a device response, false if it's a
 *   manufactuer response.
 */
void UsbProWidgetDetector::HandleIdResponse(UsbWidget *widget,
                                            unsigned int length,
                                            const uint8_t *data,
                                            bool is_device) {
  id_response response;
  memset(&response, 0, sizeof(response));
  memcpy(&response, data, length);

  map<UsbWidget*, DiscoveryState>::iterator iter = m_widgets.find(widget);

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
    iter->second.information.manufactuer = string(response.text);
    if (iter->second.discovery_state == DiscoveryState::MANUFACTURER_SENT) {
      RemoveTimeout(&iter->second);
      SendNameRequest(widget);
    }
  }
}


/*
 * Handle a serial response, this ends the device detection phase.
 */
void UsbProWidgetDetector::HandleSerialResponse(UsbWidget *widget,
                                                unsigned int length,
                                                const uint8_t *data) {
  map<UsbWidget*, DiscoveryState>::iterator iter = m_widgets.find(widget);

  if (iter == m_widgets.end())
    return;
  RemoveTimeout(&iter->second);
  WidgetInformation information = iter->second.information;

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
    information.esta_id  << " (" << information.manufactuer << "), device: "
    << information.device_id << " (" << information.device << "), serial: " <<
    "0x" << information.serial;

  if (m_callback) {
    widget->SetOnRemove(NULL);
    WidgetInformation *device_info = new WidgetInformation(information);
    m_callback->Run(widget, device_info);
  } else {
    OLA_WARN << "No listener provided";
  }
}
}  // usbpro
}  // plugin
}  // ola
