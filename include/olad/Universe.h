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
 * Universe.h
 * Header file for the Universe class, see Universe.cpp for details.
 * Copyright (C) 2005 Simon Newton
 */

#ifndef INCLUDE_OLAD_UNIVERSE_H_
#define INCLUDE_OLAD_UNIVERSE_H_

#include <ola/Clock.h>
#include <ola/DmxBuffer.h>
#include <ola/ExportMap.h>
#include <ola/base/Macro.h>
#include <ola/rdm/RDMCommand.h>
#include <ola/rdm/RDMControllerInterface.h>
#include <ola/rdm/UID.h>
#include <ola/rdm/UIDSet.h>
#include <ola/util/SequenceNumber.h>
#include <olad/DmxSource.h>

#include <set>
#include <map>
#include <vector>
#include <string>

namespace ola {

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
    std::string Name() const { return m_universe_name; }
    unsigned int UniverseId() const { return m_universe_id; }
    merge_mode MergeMode() const { return m_merge_mode; }
    bool IsActive() const;
    uint8_t ActivePriority() const { return m_active_priority; }

    /**
     * @brief Return the time between RDM discovery operations.
     * @return the amount of time in seconds between RDM discovery runs. A
     * value of 0 means that periodic discovery is disabled for this universe.
     */
    const TimeInterval& RDMDiscoveryInterval() const {
      return m_rdm_discovery_interval;
    }

    /**
     * @brief Get the time of the last discovery run
     */
    const TimeStamp& LastRDMDiscovery() const {
      return m_last_discovery_time;
    }

    // Used to adjust the properties
    void SetName(const std::string &name);
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

    // These are the ports we need to notify when data changes
    bool AddPort(InputPort *port);
    bool AddPort(OutputPort *port);
    bool RemovePort(InputPort *port);
    bool RemovePort(OutputPort *port);
    bool ContainsPort(InputPort *port) const;
    bool ContainsPort(OutputPort *port) const;
    unsigned int InputPortCount() const { return m_input_ports.size(); }
    unsigned int OutputPortCount() const { return m_output_ports.size(); }
    void InputPorts(std::vector<InputPort*> *ports) const;
    void OutputPorts(std::vector<OutputPort*> *ports) const;

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

    // This is can be called periodically to clean stale clients
    //    stale == client that has not sent data
    void CleanStaleSourceClients();

    // RDM methods
    void SendRDMRequest(ola::rdm::RDMRequest *request,
                        ola::rdm::RDMCallback *callback);
    void RunRDMDiscovery(ola::rdm::RDMDiscoveryCallback *on_complete,
                         bool full = true);
    void NewUIDList(OutputPort *port, const ola::rdm::UIDSet &uids);
    void GetUIDs(ola::rdm::UIDSet *uids) const;
    unsigned int UIDCount() const;
    uint8_t GetRDMTransactionNumber();

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
      ola::rdm::RDMStatusCode status_code;
      ola::rdm::RDMCallback *callback;
      std::vector<rdm::RDMFrame> frames;
    } broadcast_request_tracker;

    typedef std::map<Client*, bool> SourceClientMap;

    std::string m_universe_name;
    unsigned int m_universe_id;
    std::string m_universe_id_str;
    uint8_t m_active_priority;
    enum merge_mode m_merge_mode;  // merge mode
    std::vector<InputPort*> m_input_ports;
    std::vector<OutputPort*> m_output_ports;
    std::set<Client*> m_sink_clients;  // clients that require updates
    /**
     * Tracks current source clients and whether or not they are stale.
     * true == stale and can be removed, false == active is to be kept
     */
    SourceClientMap m_source_clients;
    class UniverseStore *m_universe_store;
    DmxBuffer m_buffer;
    ExportMap *m_export_map;
    std::map<ola::rdm::UID, OutputPort*> m_output_uids;
    Clock *m_clock;
    TimeInterval m_rdm_discovery_interval;
    TimeStamp m_last_discovery_time;
    ola::SequenceNumber<uint8_t> m_transaction_number_sequence;

    void HandleBroadcastAck(broadcast_request_tracker *tracker,
                            ola::rdm::RDMReply *reply);
    void HandleBroadcastDiscovery(broadcast_request_tracker *tracker,
                                  ola::rdm::RDMReply *reply);
    bool UpdateDependants();
    void UpdateName();
    void UpdateMode();
    void HTPMergeSources(const std::vector<DmxSource> &sources);
    bool MergeAll(const InputPort *port, const Client *client);
    void PortDiscoveryComplete(BaseCallback0<void> *on_complete,
                               OutputPort *output_port,
                               const ola::rdm::UIDSet &uids);
    void DiscoveryComplete(ola::rdm::RDMDiscoveryCallback *on_complete);

    void SafeIncrement(const std::string &name);
    void SafeDecrement(const std::string &name);

    template<class PortClass>
    bool GenericAddPort(PortClass *port,
                        std::vector<PortClass*> *ports);

    template<class PortClass>
    bool GenericRemovePort(PortClass *port,
                          std::vector<PortClass*> *ports,
                          std::map<ola::rdm::UID, PortClass*> *uid_map = NULL);

    template<class PortClass>
    bool GenericContainsPort(PortClass *port,
                             const std::vector<PortClass*> &ports) const;

    DISALLOW_COPY_AND_ASSIGN(Universe);
};
}  // namespace ola
#endif  // INCLUDE_OLAD_UNIVERSE_H_
