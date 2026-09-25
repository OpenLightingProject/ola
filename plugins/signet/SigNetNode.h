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
 * SigNetNode.h
 * A Sig-Net orientated, C++ wrapper around libcoap.
 * Copyright (C) 2026 Peter Newman
 */

#ifndef PLUGINS_SIGNET_SIGNETNODE_H_
#define PLUGINS_SIGNET_SIGNETNODE_H_

// Required for libcoap
#define WITH_POSIX
// #include <coap/coap.h>
#include <coap2/coap.h>
#include <ola/DmxBuffer.h>
#include <ola/ExportMap.h>
#include <ola/base/Macro.h>
#include <ola/io/Descriptor.h>
#include <ola/io/SelectServerInterface.h>
#include <ola/network/Interface.h>
#include <ola/network/IPV4Address.h>
#include <ola/network/Socket.h>
#include <ola/network/SocketAddress.h>
#include <ola/rdm/UID.h>
#include <ola/util/SequenceNumber.h>
#include <stdint.h>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace ola {
namespace plugin {
namespace signet {

/**
 * The SigNetNode object handles sending and receiving DMX data using Sig-Net.
 *
 * Sending:
 *   For sending
 *
 * Receiving:
 *   To receive DMX data, register a Callback for a specific SigNet Address. For
 *   example:
 *
 *   SigNetNode node(SigNetNode::SigNetNodeOptions(), ...);
 *   node.Init();
 *
 *   node.RegisterAddress("/dmx/1", NewCallback(...));
 *   // run the SelectServer
 *
 *   // once it's time to stop, de-register this address
 *   node.RegisterAddress("/dmx/1", NULL);
 */
class SigNetNode {
 public:
  // The options for the SigNetNode object.
  struct SigNetNodeOptions {
    uint16_t listen_port;  // UDP port to listen on

    SigNetNodeOptions() : listen_port(DEFAULT_SIGNET_PORT) {}
  };

  // The callback run when we receive new DMX data.
  typedef Callback1<void, const DmxBuffer&> DMXCallback;

  static const std::vector<std::string> SIGNET_LEVEL_URI;

  static const uint8_t key[];

  static bool PopulateHMACData(
      const uint8_t *uri, const unsigned int uri_length,
      const uint8_t security_mode,
      const ola::rdm::UID sender_id_tuid, const uint16_t sender_id_endpoint,
      const uint16_t mfg_code,
      const uint32_t session_id, const uint32_t seq_num,
      const uint8_t *payload, const unsigned int payload_length,
      uint8_t *hmac_data, unsigned int *hmac_dat_length);

  static bool GenerateHMAC(
      const uint8_t *uri, const unsigned int uri_length,
      const uint8_t security_mode,
      const ola::rdm::UID sender_id_tuid, const uint16_t sender_id_endpoint,
      const uint16_t mfg_code,
      const uint32_t session_id, const uint32_t seq_num,
      const uint8_t *payload, const unsigned int payload_length,
      const uint8_t *key, const unsigned int key_length,
      uint8_t *hmac, unsigned int *hmac_length);

  static bool UniverseIP(uint16_t universe,
                         class ola::network::IPV4Address *addr);

  static bool UniverseURI(uint16_t universe, std::string *uri);

  SigNetNode(ola::io::SelectServerInterface *ss,
             ola::ExportMap *export_map,
             const ola::rdm::UID &uid,
             const SigNetNodeOptions &options);
  ~SigNetNode();

  bool Init();
  void Stop();

  /**
   * @brief Set the Callback to be run when we receive data for this universe.
   * @param universe the universe to register the handler for
   * @param buffer the DmxBuffer to copy the data to.
   * @param priority the priority to set.
   * @param handler the Callback to call when there is data for this universe.
   *   Ownership is transferred.
   */
  bool SetHandler(const uint16_t universe, ola::DmxBuffer *buffer,
                  uint8_t *priority,
                  DMXCallback *callback);

  /**
   * @brief Remove the handler for a particular universe.
   * @param universe the universe handler to remove
   * @return true if removed, false if it didn't exist
   */
  bool RemoveHandler(uint16_t universe);

  // Sending methods
  bool SendDMX(const uint16_t universe, const ola::DmxBuffer &data);

  // Called by the libcoap handlers.
  void SetUniverse(const uint16_t universe, const uint8_t *data,
                   const unsigned int size);

  // The port SigNet is listening on.
  uint16_t ListeningPort() const;

  /**
   * @brief Return the Interface this node is using.
   */
  const ola::network::Interface &GetInterface() const { return m_interface; }

  /**
   * @brief Return the UDP socket this node is using.
   */
  ola::network::UDPSocket* GetSocket() { return &m_socket; }

 private:
  struct SigNetOutputGroup {
//    SigNetTargetVector targets;
    DmxBuffer dmx;  // holds the last values.
  };

  struct SigNetInputGroup {
    explicit SigNetInputGroup(DMXCallback *callback) : callback(callback) {}

    DmxBuffer dmx;
    std::auto_ptr<DMXCallback> callback;
  };

  typedef std::map<unsigned int, SigNetOutputGroup*> OutputGroupMap;

  typedef std::map<unsigned int, SigNetInputGroup*> InputUniverseMap;

  ola::io::SelectServerInterface *m_ss;
  ola::rdm::UID m_uid;
  const uint16_t m_listen_port;
  ola::network::Interface m_interface;
  ola::network::UDPSocket m_socket;
  ola::SequenceNumber<uint32_t> m_seq_num;
  std::auto_ptr<ola::io::UnmanagedFileDescriptor> m_descriptor;
  coap_context_t *m_coap_context;
  coap_session_t *m_coap_session;
  OutputGroupMap m_output_map;
  InputUniverseMap m_input_map;

  void DescriptorReady();
  bool SendCoapMessage(const std::string uri,
                       const ola::network::IPV4Address dest,
                       const uint8_t *payload,
                       const unsigned int payload_length);

/*void SigNetUniverseHandler(coap_context_t *ctx,
              struct coap_resource_t *resource,
              const coap_endpoint_t *local_interface,
              coap_address_t *peer,
              coap_pdu_t *request,
              str *token,
              coap_pdu_t *response);*/

  static coap_context_t* GetCoapContext(const ola::network::IPV4Address ip,
                                        const uint16_t port);

  static const uint16_t DEFAULT_SIGNET_PORT = 5683;
  static const char SIGNET_PORT_VARIABLE[];
};
}  // namespace signet
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_SIGNET_SIGNETNODE_H_

