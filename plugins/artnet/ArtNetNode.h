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
 * ArtNetNode.h
 * Header file for the ArtNetNode class
 * Copyright (C) 2005-2010 Simon Newton
 */

#ifndef PLUGINS_ARTNET_ARTNETNODE_H_
#define PLUGINS_ARTNET_ARTNETNODE_H_

#include <map>
#include <queue>
#include <string>
#include <utility>

#include "ola/Clock.h"
#include "ola/Closure.h"
#include "ola/DmxBuffer.h"
#include "ola/network/Interface.h"
#include "ola/network/SelectServerInterface.h"
#include "ola/network/Socket.h"
#include "ola/rdm/RDMCommand.h"
#include "ola/rdm/UIDSet.h"
#include "plugins/artnet/ArtNetPackets.h"

namespace ola {
namespace plugin {
namespace artnet {

using std::map;
using std::queue;
using std::string;
using ola::rdm::RDMCommand;
using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;
using ola::rdm::UID;
using ola::rdm::UIDSet;


class ArtNetNode {
  public:
    // The directions are the opposite from what OLA uses
    typedef enum {
      ARTNET_INPUT_PORT,  // sends ArtNet data
      ARTNET_OUTPUT_PORT,  // receives ArtNet data
    } artnet_port_type;

    typedef enum {
      ARTNET_MERGE_HTP,  // default
      ARTNET_MERGE_LTP,
    } artnet_merge_mode;

    typedef struct in_addr IPAddress;

    struct ltIpAddress {
      bool operator()(const IPAddress &ip1, const IPAddress &ip2) {
        return ip1.s_addr < ip2.s_addr;
      }
    };

    // This can be passed to SetPortUniverse to disable ports
    static const uint8_t ARTNET_DISABLE_PORT = 0xf0;

    // Typedef our callbacks
    typedef ola::Callback1<void, const UIDSet&> rdm_tod_callback;
    typedef ola::Callback1<void, const RDMResponse*> rdm_response_callback;
    typedef ola::Callback1<void, const RDMRequest*> rdm_request_callback;

  private:
    struct GenericPort {
      uint8_t universe_address;
      uint8_t sequence_number;
      bool enabled;
    };

    typedef map<UID, std::pair<IPAddress, uint8_t> > uid_map;

    struct InputPort: public GenericPort {
      map<IPAddress, TimeStamp, ltIpAddress> subscribed_nodes;
      // This maps each uid to the IP address and the number of TodRequests
      // since it was last seen
      uid_map uids;
      rdm_tod_callback *on_tod;
      rdm_response_callback *on_rdm_response;
      bool discovery_running;

      // these control the sending of RDM requests.
      ola::network::timeout_id rdm_send_timeout;
      queue<const class ola::rdm::RDMRequest *> pending_rdm_requests;
      const RDMResponse *overflowed_response;
    };

    enum { MAX_MERGE_SOURCES = 2 };

    struct DMXSource {
      DmxBuffer buffer;
      TimeStamp timestamp;
      IPAddress address;
    };

    struct OutputPort: public GenericPort {
      artnet_merge_mode merge_mode;
      bool is_merging;
      DMXSource sources[MAX_MERGE_SOURCES];
      DmxBuffer *buffer;
      map<UID, IPAddress> uid_map;
      Closure<void> *on_data;
      Closure<void> *on_discover;
      Closure<void> *on_flush;
      rdm_request_callback *on_rdm_request;
    };

  public:
    explicit ArtNetNode(const ola::network::Interface &interface,
                        const string &short_name,
                        const string &long_name,
                        ola::network::SelectServerInterface *ss,
                        uint8_t subnet_address = 0,
                        bool always_broadcast = false);
    virtual ~ArtNetNode();

    bool Start();
    bool Stop();

    // Various parameters to control the behaviour
    bool SetShortName(const string &name);
    string ShortName() const { return m_short_name; }
    bool SetLongName(const string &name);
    string LongName() const { return m_long_name; }

    bool SetSubnetAddress(uint8_t subnet_address);
    uint8_t SubnetAddress() const {
      return m_input_ports[0].universe_address >> 4;
    }

    bool SetPortUniverse(artnet_port_type type,
                         uint8_t port_id,
                         uint8_t universe_id);
    uint8_t GetPortUniverse(artnet_port_type type, uint8_t port_id);

    void SetBroadcastThreshold(unsigned int threshold) {
      m_broadcast_threshold = threshold;
    }

    bool SetMergeMode(uint8_t port_id, artnet_merge_mode merge_mode);


    // Poll, this should be called periodically if we're sending data.
    bool MaybeSendPoll();

    // The following apply to Input Ports (those which send data)
    bool SendDMX(uint8_t port_id, const ola::DmxBuffer &buffer);
    bool SendTodRequest(uint8_t port_id);
    bool ForceDiscovery(uint8_t port_id);
    bool SendRDMRequest(uint8_t port_id, const RDMRequest *request);
    bool SetInputPortRDMHandlers(
        uint8_t port_id,
        rdm_tod_callback *on_tod,
        rdm_response_callback *on_rdm_response);

    // The following apply to Output Ports (those which receive data);
    bool SetDMXHandler(uint8_t port_id,
                       DmxBuffer *buffer,
                       ola::Closure<void> *handler);
    bool SendTod(uint8_t port_id, const UIDSet &uid_set);
    bool SendRDMResponse(uint8_t port_id, const RDMResponse &response);
    bool SetOutputPortRDMHandlers(
        uint8_t port_id,
        ola::Closure<void> *on_discover,
        ola::Closure<void> *on_flush,
        rdm_request_callback *on_rdm_request);

    // socket management
    void SocketReady();

  private:
    bool m_running;
    bool m_send_reply_on_change;
    string m_short_name;
    string m_long_name;
    unsigned int m_broadcast_threshold;
    unsigned int m_unsolicited_replies;
    ola::network::SelectServerInterface *m_ss;
    bool m_always_broadcast;

    InputPort m_input_ports[ARTNET_MAX_PORTS];
    OutputPort m_output_ports[ARTNET_MAX_PORTS];
    ola::network::Interface m_interface;
    ola::network::UdpSocket *m_socket;
    ola::network::timeout_id m_discovery_timeout;

    ArtNetNode(const ArtNetNode&);
    ArtNetNode& operator=(const ArtNetNode&);
    bool SendPollReply(const IPAddress &destination);
    bool SendIPReply(const IPAddress &destination);
    void HandlePacket(const IPAddress &source_address,
                      const artnet_packet &packet,
                      unsigned int packet_size);
    void HandlePollPacket(const IPAddress &source_address,
                          const artnet_poll_t &packet,
                          unsigned int packet_size);
    void HandleReplyPacket(const IPAddress &source_address,
                           const artnet_reply_t &packet,
                           unsigned int packet_size);
    void HandleDataPacket(const IPAddress &source_address,
                          const artnet_dmx_t &packet,
                          unsigned int packet_size);
    void HandleTodRequest(const IPAddress &source_address,
                          const artnet_todrequest_t &packet,
                          unsigned int packet_size);
    void HandleTodData(const IPAddress &source_address,
                       const artnet_toddata_t &packet,
                       unsigned int packet_size);
    void HandleTodControl(const IPAddress &source_address,
                          const artnet_todcontrol_t &packet,
                          unsigned int packet_size);
    void HandleRdm(const IPAddress &source_address,
                   const artnet_rdm_t &packet,
                   unsigned int packet_size);
    void HandleRdmResponse(unsigned int port_id,
                           const RDMResponse *response);
    void HandleIPProgram(const IPAddress &source_address,
                         const artnet_ip_prog_t &packet,
                         unsigned int packet_size);
    void PopulatePacketHeader(artnet_packet *packet, uint16_t op_code);
    void IncrementUIDCounts(uint8_t port_id);
    bool SendPacket(const artnet_packet &packet,
                    unsigned int size,
                    const IPAddress &destination);
    void MaybeSendRDMRequest(uint8_t port_id);
    bool SendFirstRDMRequest(uint8_t port_id);
    void TimeoutRDMRequest(uint8_t port_id);
    void ClearPendingRDMRequest(uint8_t port_id);
    bool SendRDMCommand(const RDMCommand &command,
                        const IPAddress &destination,
                        uint8_t universe);
    void UpdatePortFromSource(OutputPort *port, const DMXSource &source);
    bool CheckPacketVersion(const IPAddress &source_address,
                            const string &packet_type,
                            uint16_t version);
    bool CheckPacketSize(const IPAddress &source_address,
                         const string &packet_type,
                         unsigned int actual_size,
                         unsigned int expected_size);
    bool CheckInputPortState(uint8_t port_id, const string &action);
    bool CheckOutputPortState(uint8_t port_id, const string &action);
    bool CheckPortState(uint8_t port_id, const string &action, bool is_output);
    void UpdatePortFromTodPacket(uint8_t port_id,
                                 const IPAddress &source_address,
                                 const artnet_toddata_t &packet,
                                 unsigned int packet_size);
    bool GrabDiscoveryLock(uint8_t port_id);
    void ReleaseDiscoveryLock(uint8_t port_id);
    void NotifyClientOfNewTod(uint8_t port_id);

    bool InitNetwork();

    static const char ARTNET_ID[];
    static const uint16_t ARTNET_PORT = 6454;
    static const uint16_t OEM_CODE = 0x0431;
    static const uint16_t ARTNET_VERSION = 14;
    static const unsigned int BROADCAST_THRESHOLD = 30;  // picked randomly...
    // after not receiving a PollReply after this many seconds we declare the
    // node as dead. This is set to 3x the POLL_INTERVAL in ArtNetDevice.
    static const uint8_t NODE_CODE = 0x00;
    static const uint16_t MAX_UIDS_PER_UNIVERSE = 0xffff;
    static const uint8_t RDM_VERSION = 0x01;  // v1.0 standard baby!
    static const uint8_t TOD_FLUSH_COMMAND = 0x01;
    static const unsigned int MERGE_TIMEOUT = 10;  // As per the spec
    // seconds after which a node is marked as inactive for the dmx merging
    static const unsigned int NODE_TIMEOUT = 31;
    // mseconds we wait for a TodData packet before declaring a node missing
    static const unsigned int RDM_TOD_TIMEOUT_MS = 10000;
    // Number of missed TODs before we decide a UID has gone
    static const unsigned int RDM_MISSED_TODDATA_LIMIT = 3;
    // The maximum number of requests we'll allow in the queue. This is a per
    // port (universe) limit.
    static const unsigned int RDM_REQUEST_QUEUE_LIMIT = 100;
    // How long to wait for a response to an RDM Request
    static const unsigned int RDM_REQUEST_TIMEOUT_MS = 2000;
};
}  // artnet
}  // plugin
}  // ola
#endif  // PLUGINS_ARTNET_ARTNETNODE_H_
