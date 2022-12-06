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
 * ArtNetNode.h
 * Header file for the ArtNetNodeImpl class
 * Copyright (C) 2005 Simon Newton
 */

#ifndef PLUGINS_ARTNET_ARTNETNODE_H_
#define PLUGINS_ARTNET_ARTNETNODE_H_

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "ola/Callback.h"
#include "ola/Clock.h"
#include "ola/DmxBuffer.h"
#include "ola/network/IPV4Address.h"
#include "ola/network/Interface.h"
#include "ola/io/SelectServerInterface.h"
#include "ola/network/Socket.h"
#include "ola/rdm/QueueingRDMController.h"
#include "ola/rdm/RDMCommand.h"
#include "ola/rdm/RDMFrame.h"
#include "ola/rdm/RDMControllerInterface.h"
#include "ola/rdm/UIDSet.h"
#include "ola/timecode/TimeCode.h"
#include "plugins/artnet/ArtNetPackets.h"

namespace ola {
namespace plugin {
namespace artnet {


// The directions are the opposite from what OLA uses
typedef enum {
  ARTNET_INPUT_PORT,  // sends Art-Net data
  ARTNET_OUTPUT_PORT,  // receives Art-Net data
} artnet_port_type;

typedef enum {
  ARTNET_MERGE_HTP,  // default
  ARTNET_MERGE_LTP,
} artnet_merge_mode;


// This can be passed to SetPortUniverse to disable ports
static const uint8_t ARTNET_DISABLE_PORT = 0xf0;

class ArtNetNodeOptions {
 public:
  ArtNetNodeOptions()
      : always_broadcast(false),
        use_limited_broadcast_address(false),
        rdm_queue_size(20),
        broadcast_threshold(30),
        input_port_count(4) {
  }

  bool always_broadcast;
  bool use_limited_broadcast_address;
  unsigned int rdm_queue_size;
  unsigned int broadcast_threshold;
  uint8_t input_port_count;
};


class ArtNetNodeImpl {
 public:
  /**
   * @brief Create a new node
   * @param iface the interface to use.
   * @param ss a pointer to a SelectServerInterface to use
   * @param options the ArtNetNodeOptions for the node.
   * @param socket a pointer to a UDPSocketInterface to use
   */
  ArtNetNodeImpl(const ola::network::Interface &iface,
                 ola::io::SelectServerInterface *ss,
                 const ArtNetNodeOptions &options,
                 ola::network::UDPSocketInterface *socket = NULL);

  /**
   * @brief Cleanup
   */
  virtual ~ArtNetNodeImpl();

  /**
   * @brief Start this node. The port modifying functions can be called before this.
   */
  bool Start();

  /**
   * @brief Stop this node
   */
  bool Stop();

  /**
   * @brief Start the configuration transaction.
   *
   * Configuration mode allows the caller to make changes without triggering an
   * ArtPoll or ArtPollReply per change. e.g.
   * @examplepara
   * @code
   *   node.EnterConfigurationMode()
   *   node.SetShortName()
   *   node.SetInputPortUniverse()
   *   node.SetOutputPortUniverse()
   *   // The poll / poll reply is sent here
   *   node.ExitConfigurationMode()
   * @endcode
   * @return false if there is already a transaction pending.
   */
  bool EnterConfigurationMode();

  /**
   * @brief End the configuration transaction.
   * @returns false if we weren't in a transaction.
   * @sa EnterConfigurationMode
   */
  bool ExitConfigurationMode();


  // Various parameters to control the behaviour
  /**
   * @brief Set the short name.
   * @param name the short node name
   */
  bool SetShortName(const std::string &name);
  std::string ShortName() const { return m_short_name; }

  /**
   * @brief Set the long name.
   * @param name the long node name
   */
  bool SetLongName(const std::string &name);
  std::string LongName() const { return m_long_name; }

  /**
   * @brief Set the net address for this node
   * @param net_address the Art-Net 'net' address
   */
  bool SetNetAddress(uint8_t net_address);
  uint8_t NetAddress() const { return m_net_address; }

  /**
   * @brief Set the subnet address for this node
   * @param subnet_address the Art-Net 'subnet' address, 4 bits.
   */
  bool SetSubnetAddress(uint8_t subnet_address);
  uint8_t SubnetAddress() const {
    return m_output_ports[0].universe_address >> 4;
  }

  /**
   * Get the number of input ports
   * @returns the number of input ports
   */
  uint8_t InputPortCount() const;

  /**
   * Set the universe address of an input port
   */
  bool SetInputPortUniverse(uint8_t port_id, uint8_t universe_id);

  /**
   * @brief Get an input port universe address
   *
   * Return the 8bit universe address for a port. This does not include the
   * Art-Net III net-address.
   * @param port_id a port id between 0 and ARTNET_MAX_PORTS - 1
   * @return The universe address for the port. Invalid port_ids return 0.
   */
  uint8_t GetInputPortUniverse(uint8_t port_id) const;

  /**
   * @brief Disable an input port.
   * @param port_id a port id between 0 and ARTNET_MAX_PORTS - 1
   */
  void DisableInputPort(uint8_t port_id);

  /**
   * @brief Check the state of an input port
   * @param port_id a port id between 0 and ARTNET_MAX_PORTS - 1
   * @return the state (enabled or disabled) of an input port. An invalid
   * port_id returns false.
   */
  bool InputPortState(uint8_t port_id) const;

  /**
   * @brief Set the universe for an output port.
   * @param port_id a port id between 0 and ARTNET_MAX_PORTS - 1
   * @param universe_id the new universe id.
   */
  bool SetOutputPortUniverse(uint8_t port_id, uint8_t universe_id);

  /**
   * Return the current universe address for an output port
   * @param port_id a port id between 0 and ARTNET_MAX_PORTS - 1
   * @return the universe address for the port
   */
  uint8_t GetOutputPortUniverse(uint8_t port_id);

  /**
   * @brief Disable an output port.
   * @param port_id a port id between 0 and ARTNET_MAX_PORTS - 1
   */
  void DisableOutputPort(uint8_t port_id);

  /**
   * @brief Check the state of an output port
   * @param port_id a port id between 0 and ARTNET_MAX_PORTS - 1
   * @return the state (enabled or disabled) of an output port. An invalid
   * port_id returns false.
   */
  bool OutputPortState(uint8_t port_id) const;

  void SetBroadcastThreshold(unsigned int threshold) {
    m_broadcast_threshold = threshold;
  }

  /**
   * @brief Set the merge mode for an output port
   * @param port_id a port id between 0 and ARTNET_MAX_PORTS - 1
   * @param merge_mode the artnet_merge_mode
   */
  bool SetMergeMode(uint8_t port_id, artnet_merge_mode merge_mode);

  /**
   * @brief Send an ArtPoll if any of the ports are sending data
   *
   * This should be called periodically if we're sending data.
   */
  bool SendPoll();

  // The following apply to Input Ports (those which send data)
  /**
   * @brief Send some DMX data
   * @param port_id port to send on
   * @param buffer the DMX data
   * @return true if it was sent successfully, false otherwise
   */
  bool SendDMX(uint8_t port_id, const ola::DmxBuffer &buffer);

  /**
   * @brief Flush the TOD and force a full discovery.
   *
   * The DiscoverableQueueingRDMController ensures this is only called one at a
   * time.
   * @param port_id port to discover on
   * @param callback the RDMDiscoveryCallback to run when discovery completes
   */
  void RunFullDiscovery(uint8_t port_id,
                        ola::rdm::RDMDiscoveryCallback *callback);

  /**
   * @brief Run an 'incremental' discovery. This just involves fetching the TOD from
   * all nodes.
   *
   * The DiscoverableQueueingRDMController ensures only one discovery process
   * is running per port at any time.
   * @param port_id port to send on
   * @param callback the RDMDiscoveryCallback to run when discovery completes
   */
  void RunIncrementalDiscovery(uint8_t port_id,
                               ola::rdm::RDMDiscoveryCallback *callback);

  /**
   * @brief Send an RDMRequest on this port
   *
   * This may defer the sending if there are
   * other outstanding messages in the queue.
   * @param port_id the id of the port to send the request on
   * @param request the RDMRequest object
   * @param on_complete the RDMCallback to run
   *
   * Because this is wrapped in the QueueingRDMController this will only be
   * called one-at-a-time (per port)
   */
  void SendRDMRequest(uint8_t port_id,
                      ola::rdm::RDMRequest *request,
                      ola::rdm::RDMCallback *on_complete);

  /**
   * @brief Set the RDM handlers for an Input port
   * @param port_id the id of the port to set the handlers for
   * @param on_tod the callback to be invoked when a ArtTod message is
   * received, and the RDM process isn't running.
   */
  bool SetUnsolicitedUIDSetHandler(
      uint8_t port_id,
      ola::Callback1<void, const ola::rdm::UIDSet&> *on_tod);

  /**
   * @brief Get the nodes listening to the universe a port is sending
   *
   * Populate the vector with a list of IP addresses that are known to be
   * listening for the universe that this port is sending
   * @param port_id the id of the port to fetch nodes for
   * @param[out] node_addresses a vector of nodes listening to the port
   */
  void GetSubscribedNodes(
      uint8_t port_id,
      std::vector<ola::network::IPV4Address> *node_addresses);

  // The following apply to Output Ports (those which receive data);
  /**
   * @brief Set the closure to be called when we receive data for this universe.
   * @param port_id the id of the port to register the handler for
   * @param buffer a pointer to the DmxBuffer
   * @param handler the Callback0 to call when there is data for this universe.
   * Ownership of the closure is transferred to the node.
   */
  bool SetDMXHandler(uint8_t port_id,
                     DmxBuffer *buffer,
                     ola::Callback0<void> *handler);

  /**
   * @brief Send an set of UIDs in one of more ArtTod packets
   * @param port_id the id of the port to send on
   * @param uid_set the UIDSet to send
   */
  bool SendTod(uint8_t port_id, const ola::rdm::UIDSet &uid_set);

  /**
   * @brief Set the RDM handlers for an Output port
   */
  bool SetOutputPortRDMHandlers(
      uint8_t port_id,
      ola::Callback0<void> *on_discover,
      ola::Callback0<void> *on_flush,
      ola::Callback2<void,
                     ola::rdm::RDMRequest*,
                     ola::rdm::RDMCallback*> *on_rdm_request);

  /**
   * @brief Send a timecode packet
   */
  bool SendTimeCode(const ola::timecode::TimeCode &timecode);

 private:
  class InputPort;
  typedef std::vector<InputPort*> InputPorts;

  // map a uid to a IP address and the number of times we've missed a
  // response.
  typedef std::map<ola::rdm::UID,
                   std::pair<ola::network::IPV4Address, uint8_t> > uid_map;

  enum { MAX_MERGE_SOURCES = 2 };

  struct DMXSource {
    DmxBuffer buffer;
    TimeStamp timestamp;
    ola::network::IPV4Address address;
  };

  // Output Ports receive Art-Net data
  struct OutputPort {
    uint8_t universe_address;
    uint8_t sequence_number;
    bool enabled;
    artnet_merge_mode merge_mode;
    bool is_merging;
    DMXSource sources[MAX_MERGE_SOURCES];
    DmxBuffer *buffer;
    std::map<ola::rdm::UID, ola::network::IPV4Address> uid_map;
    Callback0<void> *on_data;
    Callback0<void> *on_discover;
    Callback0<void> *on_flush;
    ola::Callback2<void,
                   ola::rdm::RDMRequest*,
                   ola::rdm::RDMCallback*> *on_rdm_request;
  };

  bool m_running;
  uint8_t m_net_address;  // this is the 'net' portion of the Art-Net address
  bool m_send_reply_on_change;
  std::string m_short_name;
  std::string m_long_name;
  unsigned int m_broadcast_threshold;
  unsigned int m_unsolicited_replies;
  ola::io::SelectServerInterface *m_ss;
  bool m_always_broadcast;
  bool m_use_limited_broadcast_address;

  // The following keep track of "Configuration mode"
  bool m_in_configuration_mode;
  bool m_artpoll_required;
  bool m_artpollreply_required;

  InputPorts m_input_ports;
  OutputPort m_output_ports[ARTNET_MAX_PORTS];
  ola::network::Interface m_interface;
  std::auto_ptr<ola::network::UDPSocketInterface> m_socket;

  /**
   * @brief Called when there is data on this socket
   */
  void SocketReady();

  /**
   * @brief Send an ArtPoll if we're both running and not in configuration mode.
   *
   * If we're in configuration mode this sets m_artpoll_required instead.
   */
  bool SendPollIfAllowed();

  /**
   * @brief Send an ArtPollReply if we're both running and m_send_reply_on_change is
   * true.
   *
   * If we're in configuration mode, this sets m_artpollreply_required
   * instead of sending.
   */
  bool SendPollReplyIfRequired();

  /**
   * @brief Send an ArtPollReply message
   */
  bool SendPollReply(const ola::network::IPV4Address &destination);

  /**
   * @brief Send an IPProgReply
   */
  bool SendIPReply(const ola::network::IPV4Address &destination);

  /**
   * @brief Handle an Art-Net packet
   */
  void HandlePacket(const ola::network::IPV4Address &source_address,
                    const artnet_packet &packet,
                    unsigned int packet_size);

  /**
   * @brief Handle an ArtPoll packet
   */
  void HandlePollPacket(const ola::network::IPV4Address &source_address,
                        const artnet_poll_t &packet,
                        unsigned int packet_size);

  /**
   * @brief Handle an ArtPollReply packet
   */
  void HandleReplyPacket(const ola::network::IPV4Address &source_address,
                         const artnet_reply_t &packet,
                         unsigned int packet_size);

  /**
   * @brief Handle a DMX Data packet, this takes care of the merging
   */
  void HandleDataPacket(const ola::network::IPV4Address &source_address,
                        const artnet_dmx_t &packet,
                        unsigned int packet_size);

  /**
   * @brief Handle a TOD Request packet
   */
  void HandleTodRequest(const ola::network::IPV4Address &source_address,
                        const artnet_todrequest_t &packet,
                        unsigned int packet_size);

  /**
   * @brief Handle a TOD Data packet
   */
  void HandleTodData(const ola::network::IPV4Address &source_address,
                     const artnet_toddata_t &packet,
                     unsigned int packet_size);

  /**
   * @brief Handle a TOD Control packet
   */
  void HandleTodControl(const ola::network::IPV4Address &source_address,
                        const artnet_todcontrol_t &packet,
                        unsigned int packet_size);

  /**
   * @brief Handle an RDM packet
   */
  void HandleRdm(const ola::network::IPV4Address &source_address,
                 const artnet_rdm_t &packet,
                 unsigned int packet_size);

  /**
   * @brief Handle the completion of a request for an Output port
   */
  void RDMRequestCompletion(ola::network::IPV4Address destination,
                            uint8_t port_id,
                            uint8_t universe_address,
                            ola::rdm::RDMReply *reply);

  /**
   * @brief Handle an RDM response.
   *
   * <rant>
   * Art-Net as a protocol is broken, the nodes don't buffer ACK_OVERFLOW
   * messages so if another GET/SET message arrives from *any* controller the
   * ACK_OVERFLOW session will be reset, possibly causing the controller to spin
   * in a loop.
   * </rant>
   */
  void HandleRDMResponse(InputPort *port,
                         const ola::rdm::RDMFrame &rdm_data,
                         const ola::network::IPV4Address &source_address);

  /**
   * @brief Handle an IP Program message.
   */
  void HandleIPProgram(const ola::network::IPV4Address &source_address,
                       const artnet_ip_prog_t &packet,
                       unsigned int packet_size);

  /**
   * @brief Fill in the header for a packet
   */
  void PopulatePacketHeader(artnet_packet *packet, uint16_t op_code);

  /**
   * @brief Send an Art-Net packet
   * @param packet the packet to send
   * @param size the size of the packet, excluding the header portion
   * @param destination where to send the packet to
   */
  bool SendPacket(const artnet_packet &packet,
                  unsigned int size,
                  const ola::network::IPV4Address &destination);

  /**
   * @brief Timeout a pending RDM request
   * @param port the id of the port to timeout.
   */
  void TimeoutRDMRequest(InputPort *port);

  /**
   * @brief Send a generic ArtRdm message
   */
  bool SendRDMCommand(const ola::rdm::RDMCommand &command,
                      const ola::network::IPV4Address &destination,
                      uint8_t universe);

  /**
   * @brief Update a port from a source, merging if necessary
   */
  void UpdatePortFromSource(OutputPort *port, const DMXSource &source);

  /**
   * @brief Check the version number of a incoming packet
   */
  bool CheckPacketVersion(const ola::network::IPV4Address &source_address,
                          const std::string &packet_type,
                          uint16_t version);

  /**
   * @brief Check the size of an incoming packet
   */
  bool CheckPacketSize(const ola::network::IPV4Address &source_address,
                       const std::string &packet_type,
                       unsigned int actual_size,
                       unsigned int expected_size);

  // methods for accessing Input & Output ports
  /**
   * @brief Lookup an InputPort by id, if the id is invalid, we return NULL.
   */
  InputPort *GetInputPort(uint8_t port_id, bool warn = true);

  /**
   * @brief A const version of GetInputPort();
   */
  const InputPort *GetInputPort(uint8_t port_id) const;

  /**
   * @brief Similar to GetInputPort, but this also confirms the port is enabled.
   */
  InputPort *GetEnabledInputPort(uint8_t port_id, const std::string &action);

  /**
   * @brief Lookup an OutputPort by id, if the id is invalid, we return NULL.
   */
  OutputPort *GetOutputPort(uint8_t port_id);

  /**
   * @brief A const version of GetOutputPort();
   */
  const OutputPort *GetOutputPort(uint8_t port_id) const;

  /**
   * @brief Similar to GetOutputPort, but this also confirms the port is enabled.
   */
  OutputPort *GetEnabledOutputPort(uint8_t port_id, const std::string &action);

  /**
   * @brief Update a port with a new TOD list
   */
  void UpdatePortFromTodPacket(InputPort *port,
                               const ola::network::IPV4Address &source_address,
                               const artnet_toddata_t &packet,
                               unsigned int packet_size);

  /**
   * @brief Called when the discovery process times out.
   */
  void ReleaseDiscoveryLock(InputPort *port);

  /**
   * @brief Start the discovery process, this puts the port into discovery mode and
   * sets up the callback.
   * @param port the InputPort to discover devices one
   * @param callback the RDMDiscoveryCallback to run
   */
  bool StartDiscoveryProcess(InputPort *port,
                             ola::rdm::RDMDiscoveryCallback *callback);

  /**
   * @brief Setup the networking components.
   */
  bool InitNetwork();

  static const char ARTNET_ID[];
  static const uint16_t ARTNET_PORT = 6454;
  static const uint16_t OEM_CODE = 0x0431;
  static const uint16_t ARTNET_VERSION = 14;
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
  static const unsigned int RDM_TOD_TIMEOUT_MS = 4000;
  // Number of missed TODs before we decide a UID has gone
  static const unsigned int RDM_MISSED_TODDATA_LIMIT = 3;
  // The maximum number of requests we'll allow in the queue. This is a per
  // port (universe) limit.
  static const unsigned int RDM_REQUEST_QUEUE_LIMIT = 100;
  // How long to wait for a response to an RDM Request
  static const unsigned int RDM_REQUEST_TIMEOUT_MS = 2000;

  DISALLOW_COPY_AND_ASSIGN(ArtNetNodeImpl);
};


/**
 * This glues the ArtNetNodeImpl together with the QueueingRDMController.
 * The ArtNetNodeImpl takes a port id so we need this extra layer.
 */
class ArtNetNodeImplRDMWrapper
    : public ola::rdm::DiscoverableRDMControllerInterface {
 public:
  ArtNetNodeImplRDMWrapper(ArtNetNodeImpl *impl, uint8_t port_id):
      m_impl(impl),
      m_port_id(port_id) {
  }
  ~ArtNetNodeImplRDMWrapper() {}

  void SendRDMRequest(ola::rdm::RDMRequest *request,
                      ola::rdm::RDMCallback *on_complete) {
    m_impl->SendRDMRequest(m_port_id, request, on_complete);
  }

  void RunFullDiscovery(ola::rdm::RDMDiscoveryCallback *callback) {
    m_impl->RunFullDiscovery(m_port_id, callback);
  }

  void RunIncrementalDiscovery(ola::rdm::RDMDiscoveryCallback *callback) {
    m_impl->RunIncrementalDiscovery(m_port_id, callback);
  }

 private:
  ArtNetNodeImpl *m_impl;
  uint8_t m_port_id;

  DISALLOW_COPY_AND_ASSIGN(ArtNetNodeImplRDMWrapper);
};


/**
 * @brief The actual Art-Net Node
 */
class ArtNetNode {
 public:
  ArtNetNode(const ola::network::Interface &iface,
             ola::io::SelectServerInterface *ss,
             const ArtNetNodeOptions &options,
             ola::network::UDPSocketInterface *socket = NULL);
  virtual ~ArtNetNode();

  bool Start() { return m_impl.Start(); }
  bool Stop() { return m_impl.Stop(); }

  bool EnterConfigurationMode() {
    return m_impl.EnterConfigurationMode();
  }
  bool ExitConfigurationMode() {
    return m_impl.ExitConfigurationMode();
  }

  // Various parameters to control the behaviour
  bool SetShortName(const std::string &name) {
    return m_impl.SetShortName(name);
  }

  std::string ShortName() const { return m_impl.ShortName(); }
  bool SetLongName(const std::string &name) {
    return m_impl.SetLongName(name);
  }

  std::string LongName() const { return m_impl.LongName(); }

  uint8_t NetAddress() const { return m_impl.NetAddress(); }
  bool SetNetAddress(uint8_t net_address) {
    return m_impl.SetNetAddress(net_address);
  }
  bool SetSubnetAddress(uint8_t subnet_address) {
    return m_impl.SetSubnetAddress(subnet_address);
  }
  uint8_t SubnetAddress() const {
    return m_impl.SubnetAddress();
  }

  uint8_t InputPortCount() const {
    return m_impl.InputPortCount();
  }

  bool SetInputPortUniverse(uint8_t port_id, uint8_t universe_id) {
    return m_impl.SetInputPortUniverse(port_id, universe_id);
  }
  uint8_t GetInputPortUniverse(uint8_t port_id) const {
    return m_impl.GetInputPortUniverse(port_id);
  }
  void DisableInputPort(uint8_t port_id) {
    m_impl.DisableInputPort(port_id);
  }
  bool InputPortState(uint8_t port_id) const {
    return m_impl.InputPortState(port_id);
  }

  bool SetOutputPortUniverse(uint8_t port_id, uint8_t universe_id) {
    return m_impl.SetOutputPortUniverse(port_id, universe_id);
  }
  uint8_t GetOutputPortUniverse(uint8_t port_id) {
    return m_impl.GetOutputPortUniverse(port_id);
  }
  void DisableOutputPort(uint8_t port_id) {
    m_impl.DisableOutputPort(port_id);
  }
  bool OutputPortState(uint8_t port_id) const {
    return m_impl.OutputPortState(port_id);
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

  /**
   * @brief Trigger full discovery for a port
   */
  void RunFullDiscovery(uint8_t port_id,
                        ola::rdm::RDMDiscoveryCallback *callback);

  /**
   * @brief Trigger incremental discovery for a port.
   */
  void RunIncrementalDiscovery(uint8_t port_id,
                               ola::rdm::RDMDiscoveryCallback *callback);

  /**
   * @brief Send a RDM request by passing it though the Queuing Controller
   */
  void SendRDMRequest(uint8_t port_id,
                      ola::rdm::RDMRequest *request,
                      ola::rdm::RDMCallback *on_complete);

  /*
   * @brief This handler is called if we receive ArtTod packets and a discovery
   * process isn't running.
   */
  bool SetUnsolicitedUIDSetHandler(
      uint8_t port_id,
      ola::Callback1<void, const ola::rdm::UIDSet&> *on_tod) {
    return m_impl.SetUnsolicitedUIDSetHandler(port_id, on_tod);
  }
  void GetSubscribedNodes(
      uint8_t port_id,
      std::vector<ola::network::IPV4Address> *node_addresses) {
    m_impl.GetSubscribedNodes(port_id, node_addresses);
  }

  // The following apply to Output Ports (those which receive data);
  bool SetDMXHandler(uint8_t port_id,
                     DmxBuffer *buffer,
                     ola::Callback0<void> *handler) {
    return m_impl.SetDMXHandler(port_id, buffer, handler);
  }
  bool SendTod(uint8_t port_id, const ola::rdm::UIDSet &uid_set) {
    return m_impl.SendTod(port_id, uid_set);
  }
  bool SetOutputPortRDMHandlers(
      uint8_t port_id,
      ola::Callback0<void> *on_discover,
      ola::Callback0<void> *on_flush,
      ola::Callback2<void,
                     ola::rdm::RDMRequest*,
                     ola::rdm::RDMCallback*> *on_rdm_request) {
    return m_impl.SetOutputPortRDMHandlers(port_id,
                                           on_discover,
                                           on_flush,
                                           on_rdm_request);
  }

  // Time Code methods
  bool SendTimeCode(const ola::timecode::TimeCode &timecode) {
    return m_impl.SendTimeCode(timecode);
  }

 private:
  ArtNetNodeImpl m_impl;
  std::vector<ArtNetNodeImplRDMWrapper*> m_wrappers;
  std::vector<ola::rdm::DiscoverableQueueingRDMController*> m_controllers;

  /**
   * @brief Check that the port_id is a valid input port.
   * @return true if the port id is valid, false otherwise
   */
  bool CheckInputPortId(uint8_t port_id);

  DISALLOW_COPY_AND_ASSIGN(ArtNetNode);
};
}  // namespace artnet
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_ARTNET_ARTNETNODE_H_
