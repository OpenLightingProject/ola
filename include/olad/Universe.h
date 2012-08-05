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
 * Universe.h
 * Header file for the Universe class, see Universe.cpp for details.
 * Copyright (C) 2005-2009 Simon Newton
 */

#ifndef INCLUDE_OLAD_UNIVERSE_H_
#define INCLUDE_OLAD_UNIVERSE_H_

#include <set>
#include <map>
#include <vector>
#include <string>
#include <ola/Clock.h>  // NOLINT
#include <ola/DmxBuffer.h>  // NOLINT
#include <ola/ExportMap.h>  // NOLINT
#include <ola/rdm/RDMCommand.h>  // NOLINT
#include <ola/rdm/RDMControllerInterface.h>  // NOLINT
#include <ola/rdm/UID.h>  // NOLINT
#include <ola/rdm/UIDSet.h>  // NOLINT
#include <olad/DmxSource.h>  // NOLINT

namespace ola {

using ola::rdm::UID;
using std::pair;
using std::set;
using ola::rdm::RDMDiscoveryCallback;

class Client;
class InputPort;
class OutputPort;

class Universe: public ola::rdm::RDMControllerInterface {
  public:
    enum merge_mode {
      MERGE_HTP,
      MERGE_LTP
    };

    Universe(unsigned int uid, class UniverseStore *store,
             ExportMap *export_map,
             Clock *clock);
    ~Universe();

    // Properties for this universe
    string Name() const { return m_universe_name; }
    unsigned int UniverseId() const { return m_universe_id; }
    merge_mode MergeMode() const { return m_merge_mode; }
    bool IsActive() const;
    uint8_t ActivePriority() const { return m_active_priority; }

    /**
     * Return the time between RDM discovery operations.
     * @return the amount of time in seconds between RDM discovery runs. A
     * value of 0 means that periodic discovery is disabled for this universe.
     */
    const TimeInterval& RDMDiscoveryInterval() const {
      return m_rdm_discovery_interval;
    }

    /**
     * Get the time of the last discovery run
     */
    const TimeStamp& LastRDMDiscovery() const {
      return m_last_discovery_time;
    }

    // Used to adjust the properties
    void SetName(const string &name);
    void SetMergeMode(merge_mode merge_mode);

    /**
     * Set the time between periodic RDM discovery operations.
     */
    void SetRDMDiscoveryInterval(const TimeInterval &discovery_interval) {
      m_rdm_discovery_interval = discovery_interval;
    }

    // Each universe has a DMXBuffer
    bool SetDMX(const DmxBuffer &buffer);
    const DmxBuffer &GetDMX() const { return m_buffer; }

    // These are the ports we need to nofity when data changes
    bool AddPort(InputPort *port);
    bool AddPort(OutputPort *port);
    bool RemovePort(InputPort *port);
    bool RemovePort(OutputPort *port);
    bool ContainsPort(InputPort *port) const;
    bool ContainsPort(OutputPort *port) const;
    unsigned int InputPortCount() const { return m_input_ports.size(); }
    unsigned int OutputPortCount() const { return m_output_ports.size(); }
    void InputPorts(vector<InputPort*> *ports);
    void OutputPorts(vector<OutputPort*> *ports);

    // Source clients are those that provide us with data
    bool AddSourceClient(Client *client);
    bool RemoveSourceClient(Client *client);
    bool ContainsSourceClient(Client *client) const;
    unsigned int SourceClientCount() const { return m_source_clients.size(); }

    // Sink clients are those that we need to send data
    bool AddSinkClient(Client *client);
    bool RemoveSinkClient(Client *client);
    bool ContainsSinkClient(Client *client) const;
    unsigned int SinkClientCount() const { return m_sink_clients.size(); }

    // These are called when new data arrives on a port/client
    bool PortDataChanged(InputPort *port);
    bool SourceClientDataChanged(Client *client);

    // RDM methods
    void SendRDMRequest(const ola::rdm::RDMRequest *request,
                        ola::rdm::RDMCallback *callback);
    void RunRDMDiscovery(RDMDiscoveryCallback *on_complete,
                         bool full = true);
    void NewUIDList(OutputPort *port, const ola::rdm::UIDSet &uids);
    void GetUIDs(ola::rdm::UIDSet *uids) const;
    unsigned int UIDCount() const;

    bool operator==(const Universe &other) {
      return m_universe_id == other.UniverseId();
    }

    static const char K_FPS_VAR[];
    static const char K_MERGE_HTP_STR[];
    static const char K_MERGE_LTP_STR[];
    static const char K_UNIVERSE_INPUT_PORT_VAR[];
    static const char K_UNIVERSE_MODE_VAR[];
    static const char K_UNIVERSE_NAME_VAR[];
    static const char K_UNIVERSE_OUTPUT_PORT_VAR[];
    static const char K_UNIVERSE_RDM_REQUESTS[];
    static const char K_UNIVERSE_SINK_CLIENTS_VAR[];
    static const char K_UNIVERSE_SOURCE_CLIENTS_VAR[];
    static const char K_UNIVERSE_UID_COUNT_VAR[];

  private:
    typedef struct {
      unsigned int expected_count;
      unsigned int current_count;
      ola::rdm::rdm_response_code response_code;
      ola::rdm::RDMCallback *callback;
      vector<string> packets;
    } broadcast_request_tracker;

    string m_universe_name;
    unsigned int m_universe_id;
    string m_universe_id_str;
    uint8_t m_active_priority;
    enum merge_mode m_merge_mode;  // merge mode
    vector<InputPort*> m_input_ports;
    vector<OutputPort*> m_output_ports;
    set<Client*> m_sink_clients;  // clients that require updates
    set<Client*> m_source_clients;  // clients that provide data
    class UniverseStore *m_universe_store;
    DmxBuffer m_buffer;
    ExportMap *m_export_map;
    map<UID, OutputPort*> m_output_uids;
    Clock *m_clock;
    TimeInterval m_rdm_discovery_interval;
    TimeStamp m_last_discovery_time;

    Universe(const Universe&);
    Universe& operator=(const Universe&);
    void HandleBroadcastAck(broadcast_request_tracker *tracker,
                            ola::rdm::rdm_response_code code,
                            const ola::rdm::RDMResponse *response,
                            const std::vector<std::string> &packets);
    void HandleBroadcastDiscovery(broadcast_request_tracker *tracker,
                                  ola::rdm::rdm_response_code code,
                                  const ola::rdm::RDMResponse *response,
                                  const std::vector<std::string> &packets);
    bool UpdateDependants();
    void UpdateName();
    void UpdateMode();
    bool RemoveClient(Client *client, bool is_source);
    bool AddClient(Client *client, bool is_source);
    void HTPMergeSources(const vector<DmxSource> &sources);
    bool MergeAll(const InputPort *port, const Client *client);
    void PortDiscoveryComplete(BaseCallback0<void> *on_complete,
                               OutputPort *output_port,
                               const ola::rdm::UIDSet &uids);
    void DiscoveryComplete(RDMDiscoveryCallback *on_complete);

    template<class PortClass>
    bool GenericAddPort(PortClass *port,
                        vector<PortClass*> *ports);

    template<class PortClass>
    bool GenericRemovePort(PortClass *port,
                          vector<PortClass*> *ports,
                          map<UID, PortClass*> *uid_map = NULL);

    template<class PortClass>
    bool GenericContainsPort(PortClass *port,
                             const vector<PortClass*> &ports) const;
};
}  // ola
#endif  // INCLUDE_OLAD_UNIVERSE_H_
