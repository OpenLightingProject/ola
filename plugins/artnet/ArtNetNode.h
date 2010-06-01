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

#include <string>
#include <map>
#include <utility>
#include "ola/Clock.h"
#include "ola/Closure.h"
#include "ola/DmxBuffer.h"
#include "ola/rdm/RDMCommand.h"
#include "ola/rdm/UIDSet.h"
#include "ola/network/InterfacePicker.h"
#include "ola/network/Socket.h"
#include "plugins/artnet/ArtNetPackets.h"

namespace ola {
namespace plugin {
namespace artnet {

using std::string;
using std::map;
using ola::rdm::RDMCommand;


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

    /*
    typedef ola::Callback1<void, RDMCommand*, IPAddress>;
      ArtNetRDMCallback;
    */

  private:
    struct GenericPort {
      uint8_t universe_address;
      uint8_t sequence_number;
      bool enabled;
    };

    struct InputPort: public GenericPort {
      map<IPAddress, TimeStamp, ltIpAddress> subscribed_nodes;
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
      Closure<void> *on_data;
    };

  public:
    explicit ArtNetNode(const string &ip_address,
                        const string &short_name,
                        const string &long_name,
                        const TimeStamp *wake_up_time,
                        uint8_t subnet_address = 0);
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

    // send/receive DMX
    // The following apply to Input Ports (those which send data)
    bool SendDMX(uint8_t port_id, const ola::DmxBuffer &buffer);
    /*
    bool SendTodRequest(uint8_t port_id);
    bool ForceDiscovery(uint8_t port_id);
    // We need something to handle the discovery responses here
    bool SetInputPortRDMHandlers(uint8_t port_id,
                                 ArtNetRDMCallback *on_command);
    // TODO(simon): add some sort of node id here (should be the IP)
    bool SendRDM(uint8_t port_id,
                 RDMRequest *request);
    */

    // The following apply to Output Ports (those which receive data);
    bool SetDMXHandler(uint8_t port_id,
                       DmxBuffer *buffer,
                       ola::Closure<void> *handler);
    /*
    bool SendTod(unsigned int universe, const ola::rdm::UIDSet &uid_set);
    bool SetOutputPortRDMHandlers(
        uint8_t port_id,
        ola::Closure<void> *on_discover,
        ola::Closure<void> *on_flush,
        ola::Callback1<void, RDMRequest*> *on_command);
    */

    // socket management
    ola::network::UdpSocket* GetSocket() { return m_socket; }
    void SocketReady();


  private:

    bool m_running;
    bool m_send_reply_on_change;
    string m_short_name;
    string m_long_name;
    unsigned int m_broadcast_threshold;
    unsigned int m_unsolicited_replies;
    std::string m_preferred_ip;
    const TimeStamp *m_wake_time;

    // std::map<unsigned int, ola::Callback1<void, RDMCommand*>* >
    // m_rdm_handlers;
    InputPort m_input_ports[ARTNET_MAX_PORTS];
    OutputPort m_output_ports[ARTNET_MAX_PORTS];
    ola::network::Interface m_interface;
    ola::network::UdpSocket *m_socket;
    struct sockaddr_in m_destination;

    ArtNetNode(const ArtNetNode&);
    ArtNetNode& operator=(const ArtNetNode&);
    bool SendPollReply(struct sockaddr_in destination);
    void HandlePacket(IPAddress source_address,
                      const artnet_packet &packet,
                      unsigned int packet_size);
    void HandlePollPacket(IPAddress source_address,
                          const artnet_poll_t &packet,
                          unsigned int packet_size);
    void HandleReplyPacket(IPAddress source_address,
                           const artnet_reply_t &packet,
                           unsigned int packet_size);
    void HandleDataPacket(IPAddress source_address,
                          const artnet_dmx_t &packet,
                          unsigned int packet_size);
    void PopulatePacketHeader(artnet_packet *packet, uint16_t op_code);
    bool SendPacket(const artnet_packet &packet,
                    unsigned int size,
                    struct sockaddr_in destination);
    void UpdatePortFromSource(OutputPort *port, const DMXSource &source);
    bool InitNetwork();

    static const char ARTNET_ID[];
    static const uint16_t ARTNET_PORT = 6454;
    static const uint8_t ARTNET_VERSION = 14;
    static const unsigned int BROADCAST_THRESHOLD = 30;  // picked randomly...
    // after not receiving a PollReply after this many seconds we declare the
    // node as dead. This is set to 3x the POLL_INTERVAL in ArtNetDevice.
    static const unsigned int NODE_TIMEOUT = 31;  // in s
    static const unsigned int MERGE_TIMEOUT = 10;  // As per the spec
    static const uint16_t OEM_CODE = 0x0431;
    static const uint16_t ESTA_CODE = 0x7a70;
    static const uint8_t NODE_CODE = 0x00;
};
}  // artnet
}  // plugin
}  // ola
#endif  // PLUGINS_ARTNET_ARTNETNODE_H_
