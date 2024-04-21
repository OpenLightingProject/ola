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
 * E133Endpoint.h
 * Copyright (C) 2012 Simon Newton
 */

#include <stdint.h>
#include <string>
#include "ola/e133/E133Enums.h"
#include "ola/rdm/RDMControllerInterface.h"
#include "ola/rdm/UIDSet.h"

#ifndef TOOLS_E133_E133ENDPOINT_H_
#define TOOLS_E133_E133ENDPOINT_H_

using ola::rdm::UIDSet;
using std::string;


static const uint16_t ROOT_E133_ENDPOINT = 0;

/**
 * The base class for E1.33 Endpoints.
 * Endpoints are tasked with handling RDM requests.
 */
class E133EndpointInterface
    : public ola::rdm::DiscoverableRDMControllerInterface {
 public:
    E133EndpointInterface() {}
    virtual ~E133EndpointInterface() {}

    // virtual bool supports_pid(ola::rdm::rdm_pid pid) const = 0;

    // IDENTIFY_ENDPOINT
    virtual bool identify_mode() const = 0;
    virtual void set_identify_mode(bool identify_on) = 0;

    // ENDPOINT_TO_UNIVERSE
    virtual uint16_t universe() const = 0;
    virtual void set_universe(uint16_t universe) = 0;
    virtual bool is_physical() const = 0;

    // RDM_TRAFFIC_ENABLE
    // virtual bool rdm_enabled() const = 0;

    // ENDPOINT_MODE
    // virtual ola::acn::EndpointMode endpoint_mode() const = 0;

    // ENDPOINT_LABEL
    virtual string label() const = 0;
    virtual void set_label(const string &endpoint_label)  = 0;

    // DISCOVERY_STATE

    // BACKGROUND_DISCOVERY

    // BACKGROUND_QUEUED_STATUS_POLICY

    // BACKGROUND_QUEUED_STATUS_POLICY_DESCRIPTION

    // BACKGROUND_STATUS_TYPE

    // QUEUED_STATUS_ENDPOINT_COLLECTION

    // QUEUED_STATUS_UID_COLLECTION

    // ENDPOINT_TIMING

    // ENDPOINT_TIMING_DESCRIPTION

    // ENDPOINT_RESPONDER_LIST_CHANGE
    virtual uint32_t responder_list_change() const = 0;

    // ENDPOINT_RESPONDERS
    virtual void EndpointResponders(UIDSet *uids) const = 0;

    // BINDING_AND_CONTROL_FIELDS

    static const uint16_t UNPATCHED_UNIVERSE;
    static const uint16_t COMPOSITE_UNIVERSE;
};


/**
 * An E133Endpoint which wraps another RDM controller. This just passes
 * everything through to the controller.
 */
class E133Endpoint: public E133EndpointInterface {
 public:
    // Callbacks which run various actions take place.
    // TODO(simon): if we expect the callee to read the state, perhaps there
    // should just be one callback with an enum indicating what changed?
    /*
    struct EventHandlers {
      public:
        ola::Callback0<void> *identify_changed;
        ola::Callback0<void> *universe_changed;

        EventHandlers()
          : identify_changed(NULL),
            universe_changed(NULL) {
        }
    };
    */

    /*
     * The constant properties of an endpoint
     */
    struct EndpointProperties {
      bool is_physical;

      EndpointProperties() : is_physical(false) {}
    };

    E133Endpoint(DiscoverableRDMControllerInterface *controller,
                 const EndpointProperties &properties);
    ~E133Endpoint() {}

    bool identify_mode() const { return m_identify_mode; }
    void set_identify_mode(bool identify_on);

    uint16_t universe() const { return m_universe; }
    void set_universe(uint16_t universe) { m_universe = universe; }
    bool is_physical() const { return m_is_physical; }

    string label() const { return m_endpoint_label; }
    void set_label(const string &endpoint_label) {
      m_endpoint_label = endpoint_label;
    }

    uint32_t responder_list_change() const { return m_responder_list_change; }
    void EndpointResponders(UIDSet *uids) const { *uids = m_uids; }

    virtual void RunFullDiscovery(ola::rdm::RDMDiscoveryCallback *callback);
    virtual void RunIncrementalDiscovery(
        ola::rdm::RDMDiscoveryCallback *callback);

    virtual void SendRDMRequest(ola::rdm::RDMRequest *request,
                                ola::rdm::RDMCallback *on_complete);

 private:
    bool m_identify_mode;
    const bool m_is_physical;
    uint16_t m_universe;
    string m_endpoint_label;
    uint32_t m_responder_list_change;
    UIDSet m_uids;
    DiscoverableRDMControllerInterface *m_controller;
};
#endif  // TOOLS_E133_E133ENDPOINT_H_
