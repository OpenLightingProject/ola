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
 * Runs the Robe discovery routine and handles creating RobeWidget objects.
 * Copyright (C) 2011 Simon Newton
 *
 * This class accepts a ConnectedDescriptor and runs the discovery process
 * to determine if the widget behaves like a Robe device.
 *
 * The discovery process sends the following request messages:
 *   - INFO_REQUEST
 *   - RDM_UID_REQUEST
 *
 * Early Robe Universe Interface widgets are 'locked' meaning they can only be
 * used with the Robe software. You can unlocked these by upgrading the widget
 * firmware, see http://www.robe.cz/nc/support/search-for/DSU%20RUNIT/.
 *
 * The newer WTX widgets aren't locked. We can tell the type of widget from the
 * RDM UID.
 */


#include <string.h>
#include <map>

#include "ola/Logging.h"
#include "ola/network/Socket.h"
#include "ola/rdm/UID.h"
#include "plugins/usbpro/RobeWidget.h"
#include "plugins/usbpro/RobeWidgetDetector.h"

namespace ola {
namespace plugin {
namespace usbpro {


/**
 * Constructor
 * @param scheduler a instance of SchedulerInterface used to register events.
 * @param on_success A callback to run if discovery succeeds.
 * @param on_failure A callback to run if discovery fails.
 * @param timeout the time in ms between each discovery message.
 */
RobeWidgetDetector::RobeWidgetDetector(
    ola::thread::SchedulingExecutorInterface *scheduler,
    SuccessHandler *on_success,
    FailureHandler *on_failure,
    unsigned int timeout)
    : m_scheduler(scheduler),
      m_timeout_ms(timeout),
      m_callback(on_success),
      m_failure_callback(on_failure) {
  if (!on_success)
    OLA_WARN << "on_success callback not set, this will leak memory!";
  if (!on_failure)
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
  descriptor->SetOnClose(NewSingleCallback(this,
                         &RobeWidgetDetector::WidgetRemoved,
                         widget));

  // Register a timeout for this widget
  SetupTimeout(widget, &m_widgets[widget]);
  return true;
}


/*
 * Called by the widgets when they receive a response.
 */
void RobeWidgetDetector::HandleMessage(RobeWidget *widget,
                                       uint8_t label,
                                       const uint8_t *data,
                                       unsigned int length) {
  switch (label) {
    case RobeWidget::INFO_RESPONSE:
      HandleInfoMessage(widget, data, length);
      break;
    case RobeWidget::UID_RESPONSE:
      HandleUidMessage(widget, data, length);
      break;
    default:
      OLA_WARN << "Unknown response label: 0x" << std::hex <<
        static_cast<int>(label) << ", size is " << length;
  }
}


/**
 * Handle a INFO message
 */
void RobeWidgetDetector::HandleInfoMessage(RobeWidget *widget,
                                           const uint8_t *data,
                                           unsigned int length) {
  WidgetStateMap::iterator iter = m_widgets.find(widget);
  if (iter == m_widgets.end())
    return;

  info_response_t info_response;
  if (length != sizeof(info_response)) {
    OLA_WARN << "Info response size " << length << " != " <<
      sizeof(info_response);
    return;
  } else {
    memcpy(reinterpret_cast<uint8_t*>(&info_response),
           data,
           sizeof(info_response));
    iter->second.information.hardware_version = info_response.hardware_version;
    iter->second.information.software_version = info_response.software_version;
    iter->second.information.eeprom_version = info_response.eeprom_version;
  }

  RemoveTimeout(&iter->second);
  SetupTimeout(widget, &iter->second);
  widget->SendMessage(RobeWidget::UID_REQUEST, NULL, 0);
}


/**
 * Handle a RDM UID Message
 */
void RobeWidgetDetector::HandleUidMessage(RobeWidget *widget,
                                          const uint8_t *data,
                                          unsigned int length) {
  WidgetStateMap::iterator iter = m_widgets.find(widget);
  if (iter == m_widgets.end())
    return;

  if (length != ola::rdm::UID::UID_SIZE) {
    OLA_INFO << "Robe widget returned invalid UID size: " << length;
    return;
  }

  iter->second.information.uid = ola::rdm::UID(data);

  if (!IsUnlocked(iter->second.information)) {
    OLA_WARN << "This Robe widget isn't unlocked, please visit "
      "http://www.robe.cz/nc/support/search-for/DSU%20RUNIT/ to download "
      "the new firmware.";
    return;
  }

  // ok this is a good interface at this point
  RemoveTimeout(&iter->second);

  const RobeWidgetInformation *widget_info = new RobeWidgetInformation(
      iter->second.information);
  m_widgets.erase(iter);

  OLA_INFO << "Detected Robe Device, UID : " << widget_info->uid <<
    ", Hardware version: 0x" << std::hex <<
    static_cast<int>(widget_info->hardware_version) << ", software version: 0x"
    << static_cast<int>(widget_info->software_version) << ", eeprom version 0x"
    << static_cast<int>(widget_info->eeprom_version);

  // given that we've been called via the widget's stack, schedule execution of
  // the method that deletes the widget.
  m_scheduler->Execute(
      NewSingleCallback(this,
                        &RobeWidgetDetector::DispatchWidget,
                        widget,
                        widget_info));
}


/**
 * Called if a widget is removed.
 */
void RobeWidgetDetector::WidgetRemoved(RobeWidget *widget) {
  widget->GetDescriptor()->Close();
  FailWidget(widget);
}


/*
 * Called if a widget fails to respond in a given interval or responds with an
 * invalid message.
 */
void RobeWidgetDetector::FailWidget(RobeWidget *widget) {
  WidgetStateMap::iterator iter = m_widgets.find(widget);
  if (iter != m_widgets.end()) {
    m_scheduler->RemoveTimeout(&iter->second);
    m_widgets.erase(iter);
  }
  CleanupWidget(widget);
}


/**
 * Delete a widget and run the failure callback.
 */
void RobeWidgetDetector::CleanupWidget(RobeWidget *widget) {
  ola::network::ConnectedDescriptor *descriptor = widget->GetDescriptor();
  descriptor->SetOnClose(NULL);
  delete widget;
  if (m_failure_callback.get())
    m_failure_callback->Run(descriptor);
}


/**
 * Called once we have confirmed a new widget
 */
void RobeWidgetDetector::DispatchWidget(
    RobeWidget *widget,
    const RobeWidgetInformation *info) {
  if (m_callback.get()) {
    widget->GetDescriptor()->SetOnClose(NULL);
    m_callback->Run(widget, info);
  } else {
    OLA_FATAL << "No listener provided, leaking descriptor";
    FailWidget(widget);
  }
}


/**
 * Remove a timeout for a widget.
 */
void RobeWidgetDetector::RemoveTimeout(DiscoveryState *discovery_state) {
  if (discovery_state->timeout_id != ola::thread::INVALID_TIMEOUT)
    m_scheduler->RemoveTimeout(discovery_state->timeout_id);
}


/**
 * Setup a timeout for a widget
 */
void RobeWidgetDetector::SetupTimeout(RobeWidget *widget,
                                      DiscoveryState *discovery_state) {
  discovery_state->timeout_id = m_scheduler->RegisterSingleTimeout(
      m_timeout_ms,
      NewSingleCallback(this,
                        &RobeWidgetDetector::FailWidget, widget));
}


/**
 * Returns true if the Robe interface is 'unlocked'.
 */
bool RobeWidgetDetector::IsUnlocked(const RobeWidgetInformation &info) {
  switch (info.uid.DeviceId() & MODEL_MASK) {
    case RUI_DEVICE_PREFIX:
      // RUI are unlocked past a certain version #
      return info.software_version >= RUI_MIN_UNLOCKED_SOFTWARE_VERSION;
    case WTX_DEVICE_PREFIX:
      // WTX devices are all unlocked
      return true;
    default:
      // default to no
      return false;
  }
}
}  // usbpro
}  // plugin
}  // ola
