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
 * EndpointManager.h
 * Copyright (C) 2012 Simon Newton
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H

#ifndef TOOLS_E133_ENDPOINTMANAGER_H_
#define TOOLS_E133_ENDPOINTMANAGER_H_

#include <stdint.h>
#include <ola/Callback.h>
#include <vector>

#include HASH_MAP_H

#include "tools/e133/E133Endpoint.h"


/**
 * The EndpointManager holds all endpoints.
 * The manager provides a mechanism to send notifications when endpoints are
 * added & removed. This is done through callbacks.
 */
class EndpointManager {
 public:
    typedef ola::Callback1<void, uint16_t> EndpointNotificationCallback;
    typedef enum { ADD, REMOVE, BOTH } EndpointNotificationEvent;

    EndpointManager()
        : m_list_change_number(0) {
    }

    ~EndpointManager() {}

    uint32_t list_change_number() const { return m_list_change_number; }

    // register and unregister endpoints
    bool RegisterEndpoint(uint16_t endpoint_id, class E133Endpoint *endpoint);
    void UnRegisterEndpoint(uint16_t endpoint);

    // lookup methods
    E133Endpoint* GetEndpoint(uint16_t endpoint_id) const;
    void EndpointIDs(std::vector<uint16_t> *id_list) const;

    // control notifications
    void RegisterNotification(EndpointNotificationEvent event_type,
                              EndpointNotificationCallback *callback);
    bool UnRegisterNotification(EndpointNotificationCallback *callback);

 private:
    // hash_map of non-root endpoints
    typedef HASH_NAMESPACE::HASH_MAP_CLASS<uint16_t, class E133Endpoint*>
      EndpointMap;
    EndpointMap m_endpoint_map;
    uint32_t m_list_change_number;

    // list of callbacks to run
    typedef struct {
      EndpointNotificationEvent event_type;
      EndpointNotificationCallback *callback;
    } EndpointNotification;
    std::vector<EndpointNotification> m_callbacks;

    void RunNotifications(uint16_t endpoint_id,
                          EndpointNotificationEvent event_type);
};
#endif  // TOOLS_E133_ENDPOINTMANAGER_H_
