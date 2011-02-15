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
 * WidgetDetector.cpp
 * Handles creating UsbWidget objects.
 * Copyright (C) 2010 Simon Newton
 *
 * The device represents the widget.
 */


#include <string.h>

#include <map>
#include <string>

#include "ola/Logging.h"
#include "ola/network/Socket.h"
#include "ola/network/NetworkUtils.h"
#include "plugins/usbpro/WidgetDetector.h"
#include "plugins/usbpro/UsbWidget.h"

namespace ola {
namespace plugin {
namespace usbpro {

using std::map;
using ola::network::NetworkToHost;

DeviceInformation& DeviceInformation::operator=(
    const DeviceInformation &other) {
  esta_id = other.esta_id;
  device_id = other.device_id;
  manufactuer = other.manufactuer;
  device = other.device;
  memcpy(serial, other.serial, SERIAL_LENGTH);
  return *this;
}


/*
 * Delete any in-discovery widgets
 */
WidgetDetector::~WidgetDetector() {
  map<UsbWidget*, DiscoveryState>::iterator iter;
  for (iter = m_widgets.begin(); iter != m_widgets.end(); ++iter) {
    if (m_failure_callback)
      m_failure_callback->Run(iter->first);
    else
      delete iter->first;
    RemoveTimeout(&iter->second);
  }
  m_widgets.clear();

  if (m_callback)
    delete m_callback;
  if (m_failure_callback)
    delete m_failure_callback;
}


/**
 * Set the callback to be run when new widgets are detected.
 */
void WidgetDetector::SetSuccessHandler(
        ola::Callback2<void, UsbWidget*, const DeviceInformation&> *callback) {
  if (m_callback)
    delete m_callback;
  m_callback = callback;
}


/**
 * Set the callback to be run when widgets fail to respond.
 */
void WidgetDetector::SetFailureHandler(
        ola::Callback1<void, UsbWidget*> *callback) {
  if (m_failure_callback)
    delete m_failure_callback;
  m_failure_callback = callback;
}


/*
 * Start the discovery process for a widget
 * @param widget, the UsbWidget object
 * @return true if the process started ok, false otherwise.
 */
bool WidgetDetector::Discover(UsbWidget *widget) {
  widget->SetMessageHandler(
    NewCallback(this, &WidgetDetector::HandleMessage, widget));

  if (!widget->SendMessage(UsbWidget::MANUFACTURER_LABEL, NULL, 0)) {
    delete widget;
    return false;
  }

  // Set delete_on_close here so that device removal works
  /*
  widget->SetOnClose(NewSingleCallback(this,
                     &WidgetDetector::DeviceClosed,
                     widget));
  */

  // register a timeout for this widget
  SetupTimeout(widget, &m_widgets[widget]);
  return true;
}


/*
 * Called by the widgets when they get messages
 */
void WidgetDetector::HandleMessage(UsbWidget *widget,
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


void WidgetDetector::SetupTimeout(UsbWidget *widget,
                                  DiscoveryState *discovery_state) {
  discovery_state->timeout_id = m_ss->RegisterSingleTimeout(
      m_timeout_ms,
      NewSingleCallback(this, &WidgetDetector::DiscoveryTimeout, widget));
}


void WidgetDetector::RemoveTimeout(DiscoveryState *discovery_state) {
  if (discovery_state->timeout_id != ola::network::INVALID_TIMEOUT)
    m_ss->RemoveTimeout(discovery_state->timeout_id);
}


void WidgetDetector::SendNameRequest(UsbWidget *widget) {
  widget->SendMessage(UsbWidget::DEVICE_LABEL, NULL, 0);
  DiscoveryState &discovery_state = m_widgets[widget];
  discovery_state.discovery_state = DiscoveryState::DEVICE_SENT;
  SetupTimeout(widget, &discovery_state);
}


void WidgetDetector::SendSerialRequest(UsbWidget *widget) {
  widget->SendMessage(UsbWidget::SERIAL_LABEL, NULL, 0);
  DiscoveryState &discovery_state = m_widgets[widget];
  discovery_state.discovery_state = DiscoveryState::SERIAL_SENT;
  SetupTimeout(widget, &discovery_state);
}


/*
 * Called if a widget fails to respond in a given interval
 */
void WidgetDetector::DiscoveryTimeout(UsbWidget *widget) {
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
        if (m_failure_callback)
          m_failure_callback->Run(widget);
        else
          delete widget;
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
void WidgetDetector::HandleIdResponse(UsbWidget *widget,
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
    iter->second.information.device = string(
        reinterpret_cast<const char*>(response.text));
    if (iter->second.discovery_state == DiscoveryState::DEVICE_SENT) {
      RemoveTimeout(&iter->second);
      SendSerialRequest(widget);
    }
  } else {
    iter->second.information.esta_id = id;
    iter->second.information.manufactuer = string(
        reinterpret_cast<const char*>(response.text));
    if (iter->second.discovery_state == DiscoveryState::MANUFACTURER_SENT) {
      RemoveTimeout(&iter->second);
      SendNameRequest(widget);
    }
  }
}


/*
 * Handle a serial response, this ends the device detection phase.
 */
void WidgetDetector::HandleSerialResponse(UsbWidget *widget,
                                         unsigned int length,
                                         const uint8_t *data) {
  map<UsbWidget*, DiscoveryState>::iterator iter = m_widgets.find(widget);

  if (iter == m_widgets.end())
    return;
  RemoveTimeout(&iter->second);
  DeviceInformation information = iter->second.information;

  if (length == DeviceInformation::SERIAL_LENGTH)
    memcpy(information.serial, data, DeviceInformation::SERIAL_LENGTH);
  else
    OLA_WARN << "Serial number response size " << length << " != " <<
      DeviceInformation::SERIAL_LENGTH;

  m_widgets.erase(iter);

  OLA_INFO << "Detected USB Device: ESTA Id: 0x" << std::hex <<
    information.esta_id  << " (" << information.manufactuer << "), device: "
    << information.device_id << " (" << information.device << ")";

  if (!m_callback) {
    OLA_WARN << "No listener provided";
    return;
  }

  m_callback->Run(widget, information);
  return;
}
}  // usbpro
}  // plugin
}  // ola
