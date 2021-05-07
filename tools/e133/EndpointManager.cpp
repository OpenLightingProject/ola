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
 * EndpointManager.cpp
 * Copyright (C) 2012 Simon Newton
 */

#include <ola/Logging.h>
#include <vector>
#include "ola/stl/STLUtils.h"
#include "tools/e133/EndpointManager.h"

using std::vector;

/**
 * Register a E133Endpoint
 * @param endpoint_id the endpoint index
 * @param endpoint E133Endpoint to register
 * @return true if the registration succeeded, false otherwise.
 */
bool EndpointManager::RegisterEndpoint(uint16_t endpoint_id,
                                       E133Endpoint *endpoint) {
  if (!endpoint_id) {
    OLA_WARN << "Can't register the root endpoint";
  }

  if (ola::STLInsertIfNotPresent(&m_endpoint_map, endpoint_id, endpoint)) {
    RunNotifications(endpoint_id, ADD);
    m_list_change_number++;
    return true;
  }
  return false;
}


/**
 * Unregister a E133Endpoint
 * @param endpoint_id the index of the endpoint to un-register
 */
void EndpointManager::UnRegisterEndpoint(uint16_t endpoint_id) {
  if (ola::STLRemove(&m_endpoint_map, endpoint_id)) {
    RunNotifications(endpoint_id, REMOVE);
    m_list_change_number++;
  }
}


/**
 * Lookup an endpoint by number.
 */
E133Endpoint* EndpointManager::GetEndpoint(uint16_t endpoint_id) const {
  return ola::STLFindOrNull(m_endpoint_map, endpoint_id);
}


/**
 * Fetch a list of all registered endpoints ids.
 * @param id_list pointer to a vector to be filled in with the endpoint ids.
 */
void EndpointManager::EndpointIDs(vector<uint16_t> *id_list) const {
  id_list->clear();
  ola::STLKeys(m_endpoint_map, id_list);
}


/**
 * Register a callback to run when endpoint are added or removed.
 * @param event_type the events to trigger this notification, ADD, REMOVE or
 * BOTH.
 * @param callback the Callback to run. Ownership is not transferred.
 */
void EndpointManager::RegisterNotification(
    EndpointNoticationEvent event_type,
    EndpointNotificationCallback *callback) {
  // if this callback already exists update it
  vector<EndpointNotification>::iterator iter = m_callbacks.begin();
  for (; iter != m_callbacks.end(); ++iter) {
    if (iter->callback == callback) {
      iter->event_type = event_type;
      return;
    }
  }
  EndpointNotification notification = {event_type, callback};
  m_callbacks.push_back(notification);
}


/*
 * Unregister a callback for notifications
 * @param callback the Callback to remove.
 * @return true if the notification was removed, false if it wasn't found.
 */
bool EndpointManager::UnRegisterNotification(
    EndpointNotificationCallback *callback) {
  vector<EndpointNotification>::iterator iter = m_callbacks.begin();
  for (; iter != m_callbacks.end(); ++iter) {
    if (iter->callback == callback) {
      m_callbacks.erase(iter);
      return true;
    }
  }
  return false;
}


/**
 * Run all notifications of a particular type
 * @param endpoint_id the id of the endpoint to pass to the callbacks.
 * @param event_type the type of notifications to trigger.
 */
void EndpointManager::RunNotifications(uint16_t endpoint_id,
                                       EndpointNoticationEvent event_type) {
  vector<EndpointNotification>::iterator iter = m_callbacks.begin();
  for (; iter != m_callbacks.end(); ++iter) {
    if (iter->event_type == event_type || event_type == BOTH)
      iter->callback->Run(endpoint_id);
  }
}
