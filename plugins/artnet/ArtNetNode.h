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
 * ArtNetNodeImpl.h
 * Header file for the ArtNetNodeImpl class
 * Copyright (C) 2005-2010 Simon Newton
 */

#ifndef PLUGINS_ARTNET_ARTNETNODE_H_
#define PLUGINS_ARTNET_ARTNETNODE_H_

#include <map>
#include <queue>
#include <string>
#include <utility>
#include <vector>

#include "ola/Callback.h"
#include "ola/Clock.h"
#include "ola/DmxBuffer.h"
#include "ola/network/Interface.h"
#include "ola/network/SelectServerInterface.h"
#include "ola/network/Socket.h"
#include "ola/rdm/QueueingRDMController.h"
#include "ola/rdm/RDMCommand.h"
#include "ola/rdm/RDMControllerInterface.h"
#include "ola/rdm/UIDSet.h"
#include "plugins/artnet/ArtNetPackets.h"

namespace ola {
namespace plugin {
namespace artnet {

using std::map;
using std::queue;
using std::string;
using ola::rdm::RDMCallback;
using ola::rdm::RDMCommand;
using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;
using ola::rdm::UID;
using ola::rdm::UIDSet;


// The directions are the opposite from what OLA uses
typedef enum {
  ARTNET_INPUT_PORT,  // sends ArtNet data
  ARTNET_OUTPUT_PORT,  // receives ArtNet data
} artnet_port_type;

typedef enum {
  ARTNET_MERGE_HTP,  // default
  ARTNET_MERGE_LTP,
} artnet_merge_mode;


// This can be passed to SetPortUniverse to disable ports
static const uint8_t ARTNET_DISABLE_PORT = 0xf0;

struct ltIpAddress {
  bool operator()(const struct in_addr &ip1, const struct in_addr &ip2) {
    return ip1.s_addr < ip2.s_addr;
  }
};


class ArtNetNodeImpl {
  public:
    ArtNetNodeImpl(const ola::network::Interface &interface,
                   ola::network::SelectServerInterface *ss,
                   bool always_broadcast = false,
                   ola::network::UdpSocketInterface *socket = NULL);
    virtual ~ArtNetNodeImpl();

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
    bool SendPoll();

    // The following apply to Input Ports (those which send data)
    bool SendDMX(uint8_t port_id, const ola::DmxBuffer &buffer);
    bool SendTodRequest(uint8_t port_id);
    bool ForceDiscovery(uint8_t port_id);
    void SendRDMRequest(uint8_t port_id,
                        const RDMRequest *request,
                        RDMCallback *on_complete);
    bool SetInputPortRDMHandlers(
        uint8_t port_id,
        ola::Callback1<void, const ola::rdm::UIDSet&> *on_tod);

    // The following apply to Output Ports (those which receive data);
    bool SetDMXHandler(uint8_t port_id,
                       DmxBuffer *buffer,
                       ola::Callback0<void> *handler);
    bool SendTod(uint8_t port_id, const UIDSet &uid_set);
    bool SetOutputPortRDMHandlers(
        uint8_t port_id,
        ola::Callback0<void> *on_discover,
        ola::Callback0<void> *on_flush,
        ola::Callback2<void, const RDMRequest*, RDMCallback*> *on_rdm_request);

    // socket management
    void SocketReady();

  private:
    struct GenericPort {
      uint8_t universe_address;
      uint8_t sequence_number;
      bool enabled;
    };

    // map a uid to a IP address and the number of times we've missed a
    // response.
    typedef map<UID, std::pair<struct in_addr, uint8_t> > uid_map;

    // Input ports are ones that send data using ArtNet
    struct InputPort: public GenericPort {
      map<struct in_addr, TimeStamp, ltIpAddress> subscribed_nodes;
      uid_map uids;  // used to keep track of the UIDs
      // Called when we have a new TOD
      ola::Callback1<void, const ola::rdm::UIDSet&> *on_tod;
      bool discovery_running;
      // the in-flight request and it's callback
      ola::rdm::RDMCallback *rdm_request_callback;
      const ola::rdm::RDMRequest *pending_request;
      struct in_addr rdm_ip_destination;

      // these control the sending of RDM requests.
      ola::network::timeout_id rdm_send_timeout;
    };

    enum { MAX_MERGE_SOURCES = 2 };

    struct DMXSource {
      DmxBuffer buffer;
      TimeStamp timestamp;
      struct in_addr address;
    };

    // Output Ports receive ArtNet data
    struct OutputPort: public GenericPort {
      artnet_merge_mode merge_mode;
      bool is_merging;
      DMXSource sources[MAX_MERGE_SOURCES];
      DmxBuffer *buffer;
      map<UID, struct in_addr> uid_map;
      Callback0<void> *on_data;
      Callback0<void> *on_discover;
      Callback0<void> *on_flush;
      ola::Callback2<void, const RDMRequest*, RDMCallback*> *on_rdm_request;
    };

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
    ola::network::UdpSocketInterface *m_socket;
    ola::network::timeout_id m_discovery_timeout;

    ArtNetNodeImpl(const ArtNetNodeImpl&);
    ArtNetNodeImpl& operator=(const ArtNetNodeImpl&);
    bool SendPollReply(const struct in_addr &destination);
    bool SendIPReply(const struct in_addr &destination);
    void HandlePacket(const struct in_addr &source_address,
                      const artnet_packet &packet,
                      unsigned int packet_size);
    void HandlePollPacket(const struct in_addr &source_address,
                          const artnet_poll_t &packet,
                          unsigned int packet_size);
    void HandleReplyPacket(const struct in_addr &source_address,
                           const artnet_reply_t &packet,
                           unsigned int packet_size);
    void HandleDataPacket(const struct in_addr &source_address,
                          const artnet_dmx_t &packet,
                          unsigned int packet_size);
    void HandleTodRequest(const struct in_addr &source_address,
                          const artnet_todrequest_t &packet,
                          unsigned int packet_size);
    void HandleTodData(const struct in_addr &source_address,
                       const artnet_toddata_t &packet,
                       unsigned int packet_size);
    void HandleTodControl(const struct in_addr &source_address,
                          const artnet_todcontrol_t &packet,
                          unsigned int packet_size);
    void HandleRdm(const struct in_addr &source_address,
                   const artnet_rdm_t &packet,
                   unsigned int packet_size);
    void RDMRequestCompletion(struct in_addr destination,
                              uint8_t port_id,
                              uint8_t universe_address,
                              ola::rdm::rdm_response_code code,
                              const RDMResponse *response,
                              const std::vector<std::string> &packets);
    void HandleRDMResponse(unsigned int port_id,
                           const string &rdm_data,
                           const struct in_addr &source_address);
    void HandleIPProgram(const struct in_addr &source_address,
                         const artnet_ip_prog_t &packet,
                         unsigned int packet_size);
    void PopulatePacketHeader(artnet_packet *packet, uint16_t op_code);
    void IncrementUIDCounts(uint8_t port_id);
    bool SendPacket(const artnet_packet &packet,
                    unsigned int size,
                    const struct in_addr &destination);
    void TimeoutRDMRequest(uint8_t port_id);
    bool SendRDMCommand(const RDMCommand &command,
                        const struct in_addr &destination,
                        uint8_t universe);
    void UpdatePortFromSource(OutputPort *port, const DMXSource &source);
    bool CheckPacketVersion(const struct in_addr &source_address,
                            const string &packet_type,
                            uint16_t version);
    bool CheckPacketSize(const struct in_addr &source_address,
                         const string &packet_type,
                         unsigned int actual_size,
                         unsigned int expected_size);
    bool CheckInputPortState(uint8_t port_id, const string &action);
    bool CheckOutputPortState(uint8_t port_id, const string &action);
    bool CheckPortState(uint8_t port_id, const string &action, bool is_output);
    void UpdatePortFromTodPacket(uint8_t port_id,
                                 const struct in_addr &source_address,
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




/**
 * This glues the ArtNetNodeImpl together with the QueueingRDMController.
 * The ArtNetNodeImpl takes a port id so we need this extra layer.
 */
class ArtNetNodeImplRDMWrapper: public ola::rdm::RDMControllerInterface {
  public:
    ArtNetNodeImplRDMWrapper(ArtNetNodeImpl *impl,
                             uint8_t port_id):
      m_impl(impl),
      m_port_id(port_id) {
    }
    ~ArtNetNodeImplRDMWrapper() {}

    void SendRDMRequest(const ola::rdm::RDMRequest *request,
                        ola::rdm::RDMCallback *on_complete) {
      m_impl->SendRDMRequest(m_port_id, request, on_complete);
    }

  private:
    ArtNetNodeImpl *m_impl;
    uint8_t m_port_id;
};


/**
 * The actual ArtNet Node
 */
class ArtNetNode {
  public:
    ArtNetNode(const ola::network::Interface &interface,
               ola::network::SelectServerInterface *ss,
               bool always_broadcast = false,
               unsigned int rdm_queue_size = 20,
               ola::network::UdpSocketInterface *socket = NULL);
    virtual ~ArtNetNode();

    bool Start() { return m_impl.Start(); }
    bool Stop() { return m_impl.Stop(); }

    // Various parameters to control the behaviour
    bool SetShortName(const string &name) { return m_impl.SetShortName(name); }
    string ShortName() const { return m_impl.ShortName(); }
    bool SetLongName(const string &name) { return m_impl.SetLongName(name); }
    string LongName() const { return m_impl.LongName(); }

    bool SetSubnetAddress(uint8_t subnet_address) {
      return m_impl.SetSubnetAddress(subnet_address);
    }
    uint8_t SubnetAddress() const {
      return m_impl.SubnetAddress();
    }

    bool SetPortUniverse(artnet_port_type type,
                         uint8_t port_id,
                         uint8_t universe_id) {
      return m_impl.SetPortUniverse(type, port_id, universe_id);
    }
    uint8_t GetPortUniverse(artnet_port_type type, uint8_t port_id) {
      return m_impl.GetPortUniverse(type, port_id);
    }

    void SetBroadcastThreshold(unsigned int threshold) {
      m_impl.SetBroadcastThreshold(threshold);
    }

    bool SetMergeMode(uint8_t port_id, artnet_merge_mode merge_mode) {
      return m_impl.SetMergeMode(port_id, merge_mode);
    }

    // Poll, this should be called periodically if we're sending data.
    bool SendPoll() {
      return m_impl.SendPoll();
    }

    // The following apply to Input Ports (those which send data)
    bool SendDMX(uint8_t port_id, const ola::DmxBuffer &buffer) {
      return m_impl.SendDMX(port_id, buffer);
    }
    bool SendTodRequest(uint8_t port_id) {
      return m_impl.SendTodRequest(port_id);
    }
    bool ForceDiscovery(uint8_t port_id) {
      return m_impl.ForceDiscovery(port_id);
    }
    void SendRDMRequest(uint8_t port_id,
                        const RDMRequest *request,
                        ola::rdm::RDMCallback *on_complete);
    bool SetInputPortRDMHandlers(
        uint8_t port_id,
        ola::Callback1<void, const ola::rdm::UIDSet&> *on_tod) {
      return m_impl.SetInputPortRDMHandlers(port_id, on_tod);
    }

    // The following apply to Output Ports (those which receive data);
    bool SetDMXHandler(uint8_t port_id,
                       DmxBuffer *buffer,
                       ola::Callback0<void> *handler) {
      return m_impl.SetDMXHandler(port_id, buffer, handler);
    }
    bool SendTod(uint8_t port_id, const UIDSet &uid_set) {
      return m_impl.SendTod(port_id, uid_set);
    }
    bool SetOutputPortRDMHandlers(
        uint8_t port_id,
        ola::Callback0<void> *on_discover,
        ola::Callback0<void> *on_flush,
        ola::Callback2<void, const RDMRequest*, RDMCallback*> *on_rdm_request) {
      return m_impl.SetOutputPortRDMHandlers(port_id, on_discover, on_flush,
          on_rdm_request);
  }

  private:
    ArtNetNodeImpl m_impl;
    ArtNetNodeImplRDMWrapper *m_wrappers[ARTNET_MAX_PORTS];
    ola::rdm::QueueingRDMController *m_controllers[ARTNET_MAX_PORTS];
};
}  // artnet
}  // plugin
}  // ola
#endif  // PLUGINS_ARTNET_ARTNETNODE_H_
