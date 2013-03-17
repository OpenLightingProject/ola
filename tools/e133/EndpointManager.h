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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * EndpointManager.h
 * Copyright (C) 2012 Simon Newton
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#ifndef TOOLS_E133_ENDPOINTMANAGER_H_
#define TOOLS_E133_ENDPOINTMANAGER_H_

#include <stdint.h>
#include <ola/Callback.h>
#include <vector>

#include HASH_MAP_H

#include "tools/e133/E133Endpoint.h"


/**
 * The EndpointManager holds all non-root endpoints.
 * The manager provides a mechanism to send notifications when endpoints are
 * added & removed. This is done through callbacks.
 */
class EndpointManager {
  public:
    typedef ola::Callback1<void, uint16_t> EndpointNotificationCallback;
    typedef enum { ADD, REMOVE, BOTH } EndpointNoticationEvent;

    explicit EndpointManager() {}
    ~EndpointManager() {}

    // register and unregister endpoints
    bool RegisterEndpoint(uint16_t endpoint_id, class E133Endpoint *endpoint);
    void UnRegisterEndpoint(uint16_t endpoint);

    // lookup methods
    E133Endpoint* GetEndpoint(uint16_t endpoint_id) const;
    void EndpointIDs(std::vector<uint16_t> *id_list) const;

    // control notifications
    void RegisterNotification(EndpointNoticationEvent event_type,
                              EndpointNotificationCallback *callback);
    bool UnRegisterNotification(EndpointNotificationCallback *callback);

  private:
    // hash_map of non-root endpoints
    typedef HASH_NAMESPACE::HASH_MAP_CLASS<
      uint16_t,
      class E133Endpoint*> endpoint_map;
    endpoint_map m_endpoint_map;

    // list of callbacks to run
    typedef struct {
      EndpointNoticationEvent event_type;
      EndpointNotificationCallback *callback;
    } EndpointNotification;
    std::vector<EndpointNotification> m_callbacks;

    void RunNotifications(uint16_t endpoint_id,
                          EndpointNoticationEvent event_type);
};
#endif  // TOOLS_E133_ENDPOINTMANAGER_H_
