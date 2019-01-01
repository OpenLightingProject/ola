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
 * E131Node.h
 * Header file for the E131Node class, this is the interface between OLA and
 * the E1.31 library.
 * Copyright (C) 2007 Simon Newton
 */

#ifndef LIBS_ACN_E131NODE_H_
#define LIBS_ACN_E131NODE_H_

#include <map>
#include <set>
#include <string>
#include <vector>
#include "ola/Callback.h"
#include "ola/Constants.h"
#include "ola/DmxBuffer.h"
#include "ola/acn/ACNPort.h"
#include "ola/acn/CID.h"
#include "ola/base/Macro.h"
#include "ola/io/SelectServerInterface.h"
#include "ola/thread/SchedulerInterface.h"
#include "ola/network/Interface.h"
#include "ola/network/Socket.h"
#include "libs/acn/DMPE131Inflator.h"
#include "libs/acn/E131DiscoveryInflator.h"
#include "libs/acn/E131Inflator.h"
#include "libs/acn/E131Sender.h"
#include "libs/acn/RootInflator.h"
#include "libs/acn/RootSender.h"
#include "libs/acn/UDPTransport.h"

namespace ola {
namespace acn {

class E131Node {
 public:
  /**
   * @brief Options for the E131Node.
   */
  struct Options {
   public:
    Options()
       : use_rev2(false),
         ignore_preview(true),
         enable_draft_discovery(false),
         dscp(0),
         port(ola::acn::ACN_PORT),
         source_name(ola::OLA_DEFAULT_INSTANCE_NAME) {
    }

    bool use_rev2;  /**< Use Revision 0.2 of the 2009 draft */
    bool ignore_preview;  /**< Ignore preview data */
    bool enable_draft_discovery;  /**< Enable 2014 draft discovery */
    uint8_t dscp;  /**< The DSCP value to tag packets with */
    uint16_t port; /**< The UDP port to use, defaults to ACN_PORT */
    std::string source_name; /**< The source name to use */
  };

  struct KnownController {
    acn::CID cid;
    ola::network::IPV4Address ip_address;
    std::string source_name;
    std::set<uint16_t> universes;
  };

  /**
   * @brief Create a new E1.31 node.
   * @param ss the SchedulerInterface to use.
   * @param interface the Interface to prefer to listen on
   * @param options the Options to use for the node.
   * @param cid the CID to use, if not provided we generate one.
   */
  E131Node(ola::thread::SchedulerInterface *ss,
           // stupid Windows, 'interface' seems to be a struct so we use iface
           // here.
           const ola::network::Interface &iface,
           const Options &options,
           const ola::acn::CID &cid = ola::acn::CID::Generate());
  ~E131Node();

  /**
   * @brief Start this node
   */
  bool Start();

  /**
   * @brief Stop this node
   */
  bool Stop();

  /**
   * @brief Set the name for a universe.
   * @param universe the id of the universe to send
   * @param source the new source name.
   */
  bool SetSourceName(uint16_t universe, const std::string &source);

  /**
   * @brief Signal that we will start sending on this particular universe.
   *   Without sending any DMX data.
   * @param universe to start sending on.
   */
  bool StartStream(uint16_t universe);

  /**
   * @brief Signal that we will no longer send on this particular universe.
   * @param universe to terminate sending on.
   * @param priority the priority to use in the stream terminated message.
   */
  bool TerminateStream(uint16_t universe,
                       uint8_t priority = DEFAULT_PRIORITY);

  /**
   * @brief Send some DMX data.
   * @param universe the id of the universe to send
   * @param buffer the DMX data.
   * @param priority the priority to use
   * @param preview set to true to turn on the preview bit
   * @return true if it was sent successfully, false otherwise
   */
  bool SendDMX(uint16_t universe,
               const ola::DmxBuffer &buffer,
               uint8_t priority = DEFAULT_PRIORITY,
               bool preview = false);

  /**
   * @brief Send some DMX data, allowing finer grained control of parameters.
   *
   * The method is provided for the testing framework. Don't use it in
   * production code!
   *
   * @param universe the id of the universe to send
   * @param buffer the DMX data
   * @param sequence_offset used to twiddle the sequence numbers, this doesn't
   * increment the sequence counter.
   * @param priority the priority to use
   * @param preview set to true to turn on the preview bit
   * @return true if it was sent successfully, false otherwise
   */
  bool SendDMXWithSequenceOffset(uint16_t universe,
                                 const ola::DmxBuffer &buffer,
                                 int8_t sequence_offset,
                                 uint8_t priority = DEFAULT_PRIORITY,
                                 bool preview = false);


  /**
   * @brief Signal termination of the stream for a universe.
   * @param universe the id of the universe to send
   * @param buffer the last DmxBuffer to send.
   * @param priority the priority to use, this doesn't actually make a
   * difference.
   *
   * This does not remove the universe from the list of active TX universes, so
   * it should only be used for testing purposes.
   */
  bool SendStreamTerminated(uint16_t universe,
                            const ola::DmxBuffer &buffer = DmxBuffer(),
                            uint8_t priority = DEFAULT_PRIORITY);

  /**
   * @brief Set the Callback to be run when we receive data for this universe.
   * @param universe the universe to register the handler for
   * @param buffer the DmxBuffer to copy the data to.
   * @param priority the priority to set.
   * @param handler the Callback to call when there is data for this universe.
   *   Ownership is transferred.
   */
  bool SetHandler(uint16_t universe, ola::DmxBuffer *buffer,
                  uint8_t *priority, ola::Callback0<void> *handler);

  /**
   * @brief Remove the handler for a particular universe.
   * @param universe the universe handler to remove
   * @return true if removed, false if it didn't exist
   */
  bool RemoveHandler(uint16_t universe);

  /**
   * @brief Return the Interface this node is using.
   */
  const ola::network::Interface &GetInterface() const { return m_interface; }

  /**
   * @brief Return the UDP socket this node is using.
   */
  ola::network::UDPSocket* GetSocket() { return &m_socket; }

  /**
   * @brief Return a list of known controllers.
   *
   * This will return an empty list unless enable_draft_discovery was set in
   * the node Options.
   */
  void GetKnownControllers(std::vector<KnownController> *controllers);

 private:
  struct tx_universe {
    std::string source;
    uint8_t sequence;
  };

  typedef std::map<uint16_t, tx_universe> ActiveTxUniverses;
  typedef std::map<acn::CID, class TrackedSource*> TrackedSources;

  ola::thread::SchedulerInterface *m_ss;
  ola::network::Interface m_interface;
  const Options m_options;
  const ola::acn::CID m_cid;

  ola::network::UDPSocket m_socket;
  // senders
  RootSender m_root_sender;
  E131Sender m_e131_sender;
  // inflators
  RootInflator m_root_inflator;
  E131Inflator m_e131_inflator;
  E131InflatorRev2 m_e131_rev2_inflator;
  DMPE131Inflator m_dmp_inflator;
  E131DiscoveryInflator m_discovery_inflator;

  IncomingUDPTransport m_incoming_udp_transport;
  ActiveTxUniverses m_tx_universes;
  uint8_t *m_send_buffer;

  // Discovery members
  ola::thread::timeout_id m_discovery_timeout;
  TrackedSources m_discovered_sources;

  tx_universe *SetupOutgoingSettings(uint16_t universe);

  bool PerformDiscoveryHousekeeping();
  void NewDiscoveryPage(const HeaderSet &headers,
                        const E131DiscoveryInflator::DiscoveryPage &page);
  void SendDiscoveryPage(const std::vector<uint16_t> &universes, uint8_t page,
                         uint8_t last_page, uint32_t sequence_number);

  static const uint16_t DEFAULT_PRIORITY = 100;
  static const uint16_t UNIVERSE_DISCOVERY_INTERVAL = 10000;  // milliseconds
  static const uint16_t DISCOVERY_UNIVERSE_ID = 64214;
  static const uint16_t DISCOVERY_PAGE_SIZE = 512;

  DISALLOW_COPY_AND_ASSIGN(E131Node);
};
}  // namespace acn
}  // namespace ola
#endif  // LIBS_ACN_E131NODE_H_
