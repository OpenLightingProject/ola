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
 * RobeWidgetDetector.cpp
 * Handles creating RobeWidget objects.
 * Copyright (C) 2010 Simon Newton
 *
 * This class accepts RobeWidget objects and runs the discovery process
 * to determine if the widget behaves like a Robe device.
 *
 * The discovery process sends the following request messages:
 *   - INFO_REQUEST
 *
 */


#include <string.h>

#include <map>
#include <string>

#include "ola/Logging.h"
#include "ola/network/Socket.h"
#include "ola/network/NetworkUtils.h"
#include "plugins/usbpro/RobeWidget.h"
#include "plugins/usbpro/RobeWidgetDetector.h"
#include "plugins/usbpro/RobeWidget.h"

namespace ola {
namespace plugin {
namespace usbpro {


/**
 * Constructor
 * @param ss a SelectServerInterface to use to register events.
 * @param on_success A callback to run if discovery succeeds.
 * @param on_failure A callback to run if discovery fails.
 * @param timeout the time in ms between each discovery message.
 */
RobeWidgetDetector::RobeWidgetDetector(
    ola::network::SelectServerInterface *ss,
    SuccessHandler *on_success,
    FailureHandler *on_failure,
    unsigned int timeout)
    : m_ss(ss),
      m_timeout_ms(timeout),
      m_callback(on_success),
      m_failure_callback(on_failure) {
  if (!m_callback)
    OLA_WARN << "on_success callback not set, this will leak memory!";
  if (!m_failure_callback)
    OLA_WARN << "on_failure callback not set, this will leak memory!";
}


/*
 * Fail any widgets that are still in the discovery process.
 */
RobeWidgetDetector::~RobeWidgetDetector() {
  WidgetStateMap::iterator iter;
  for (iter = m_widgets.begin(); iter != m_widgets.end(); ++iter) {
    RemoveTimeout(&iter->second);
    CleanupWidget(iter->first);
  }
  m_widgets.clear();

  if (m_callback)
    delete m_callback;
  if (m_failure_callback)
    delete m_failure_callback;
}


/*
 * Start the discovery process for a widget
 * @param widget, the RobeWidget to run discovery on.
 * @return true if the process started ok, false otherwise.
 */
bool RobeWidgetDetector::Discover(
    ola::network::ConnectedDescriptor *descriptor) {
  RobeWidget *widget = new RobeWidget(descriptor);
  widget->SetMessageHandler(
    NewCallback(this, &RobeWidgetDetector::HandleMessage, widget));

  if (!widget->SendMessage(RobeWidget::INFO_REQUEST, NULL, 0)) {
    delete widget;
    return false;
  }

  // Set the onclose handler so we can mark this as failed.
  widget->SetOnRemove(NewSingleCallback(this,
                      &RobeWidgetDetector::FailWidget,
                      widget));

  // register a timeout for this widget
  WidgetDiscoveryState &state = m_widgets[widget];
  state.timeout_id = m_ss->RegisterSingleTimeout(
      m_timeout_ms,
      NewSingleCallback(this,
                        &RobeWidgetDetector::FailWidget, widget));
  return true;
}


/*
 * Called by the widgets when they receive a response.
 */
void RobeWidgetDetector::HandleMessage(RobeWidget *widget,
                                       uint8_t label,
                                       const uint8_t *data,
                                       unsigned int length) {
  if (label != RobeWidget::INFO_RESPONSE) {
    OLA_WARN << "Unknown response label: 0x" << std::hex <<
      (int) label << ", size is " << length;
    FailWidget(widget);
    return;
  }

  WidgetStateMap::iterator iter = m_widgets.find(widget);
  if (iter == m_widgets.end()) {
    OLA_WARN << "Can't find robe widget in active map";
    FailWidget(widget);
    return;
  }

  RemoveTimeout(&iter->second);
  info_response_t info_response;
  RobeWidgetInformation information = iter->second.information;

  if (length == sizeof(info_response)) {
    memcpy(reinterpret_cast<uint8_t*>(&info_response),
           data,
           sizeof(info_response));
    information.hardware_version = info_response.hardware_version;
    information.software_version = info_response.software_version;
    information.eeprom_version = info_response.eeprom_version;
  } else {
    OLA_WARN << "Info response size " << length << " != " <<
      sizeof(info_response);
  }

  m_widgets.erase(iter);

  OLA_INFO << "Detected Robe Device: Hardware version: 0x" << std::hex <<
    (int) information.hardware_version  << ", software version: 0x" <<
    (int) information.software_version << ", eeprom version 0x" <<
    (int) information.eeprom_version;

  if (m_callback) {
    widget->SetOnRemove(NULL);
    RobeWidgetInformation *info = new RobeWidgetInformation(information);
    m_callback->Run(widget, info);
  } else {
    OLA_WARN << "No listener provided";
    FailWidget(widget);
  }
}


/*
 * Called if a widget fails to respond in a given interval or responds with an
 * invalid message.
 */
void RobeWidgetDetector::FailWidget(RobeWidget *widget) {
  // remove from the active map
  WidgetStateMap::iterator iter = m_widgets.find(widget);
  if (iter != m_widgets.end()) {
    RemoveTimeout(&(iter->second));
    m_widgets.erase(iter);
  }
  CleanupWidget(widget);
}


/**
 * Delete a widget and run the failure callback.
 */
void RobeWidgetDetector::CleanupWidget(RobeWidget *widget) {
  widget->SetOnRemove(NULL);
  if (m_failure_callback)
    m_failure_callback->Run(widget->GetDescriptor());
  delete widget;
}


/**
 * Remove a timeout associated with this widget discovery state.
 */
void RobeWidgetDetector::RemoveTimeout(WidgetDiscoveryState *discovery_state) {
  if (discovery_state->timeout_id != ola::network::INVALID_TIMEOUT)
    m_ss->RemoveTimeout(discovery_state->timeout_id);
}
}  // usbpro
}  // plugin
}  // ola
