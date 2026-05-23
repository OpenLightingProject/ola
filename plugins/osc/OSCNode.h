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
 * OSCNode.h
 * A DMX orientated, C++ wrapper around liblo.
 * Copyright (C) 2012 Simon Newton
 */

#ifndef PLUGINS_OSC_OSCNODE_H_
#define PLUGINS_OSC_OSCNODE_H_

#include <lo/lo.h>
#include <ola/DmxBuffer.h>
#include <ola/ExportMap.h>
#include <ola/base/Macro.h>
#include <ola/io/Descriptor.h>
#include <ola/io/SelectServerInterface.h>
#include <ola/network/SocketAddress.h>
#include <stdint.h>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "plugins/osc/OSCTarget.h"

namespace ola {
namespace plugin {
namespace osc {

/**
 * The OSCNode object handles sending and receiving DMX data using OSC.
 *
 * Sending:
 *   For sending, OSC Targets are assigned to groups. A group ID is just an
 *   arbitrary integer used to identify the group. It's not sent in the OSC
 *   packets. For example:
 *
 *   OSCNode node(OSCNode::OSCNodeOptions(), ...);
 *   node.Init();
 *
 *   node.AddTarget(1, OSCTarget(...));
 *   node.AddTarget(1, OSCTarget(...));
 *   node.SendData(1, FORMAT_BLOB, dmx);
 *
 * Receiving:
 *   To receive DMX data, register a Callback for a specific OSC Address. For
 *   example:
 *
 *   OSCNode node(OSCNode::OSCNodeOptions(), ...);
 *   node.Init();
 *
 *   node.RegisterAddress("/dmx/1", NewCallback(...));
 *   // run the SelectServer
 *
 *   // once it's time to stop, de-register this address
 *   node.RegisterAddress("/dmx/1", NULL);
 */
class OSCNode {
 public:
  // The different data formats we can send in.
  enum DataFormat {
    FORMAT_BLOB,
    FORMAT_INT_ARRAY,
    FORMAT_INT_INDIVIDUAL,
    FORMAT_FLOAT_ARRAY,
    FORMAT_FLOAT_INDIVIDUAL,
  };

  // The options for the OSCNode object.
  struct OSCNodeOptions {
    uint16_t listen_port;  // UDP port to listen on

    OSCNodeOptions() : listen_port(DEFAULT_OSC_PORT) {}
  };

  // The callback run when we receive new DMX data.
  typedef Callback1<void, const DmxBuffer&> DMXCallback;

  OSCNode(ola::io::SelectServerInterface *ss,
          ola::ExportMap *export_map,
          const OSCNodeOptions &options);
  ~OSCNode();

  bool Init();
  void Stop();

  // Sending methods
  void AddTarget(unsigned int group, const OSCTarget &target);
  bool RemoveTarget(unsigned int group, const OSCTarget &target);
  bool SendData(unsigned int group, DataFormat data_format,
                const ola::DmxBuffer &data);

  // Receiving methods
  bool RegisterAddress(const std::string &osc_address, DMXCallback *callback);

  // Called by the liblo handlers.
  void SetUniverse(const std::string &osc_address, const uint8_t *data,
                   unsigned int size);
  void SetSlot(const std::string &osc_address, uint16_t slot, uint8_t value);

  // The port OSC is listening on.
  uint16_t ListeningPort() const;

 private:
  class NodeOSCTarget {
   public:
    explicit NodeOSCTarget(const OSCTarget &target);
    ~NodeOSCTarget();

    bool operator==(const NodeOSCTarget &other) const {
      return (socket_address == other.socket_address &&
              osc_address == other.osc_address);
    }

    bool operator==(const OSCTarget &other) const {
      return (socket_address == other.socket_address &&
              osc_address == other.osc_address);
    }

    ola::network::IPV4SocketAddress socket_address;
    std::string osc_address;
    lo_address liblo_address;

   private:
    DISALLOW_COPY_AND_ASSIGN(NodeOSCTarget);
  };

  typedef std::vector<NodeOSCTarget*> OSCTargetVector;

  struct OSCOutputGroup {
    OSCTargetVector targets;
    DmxBuffer dmx;  // holds the last values.
  };

  struct OSCInputGroup {
    explicit OSCInputGroup(DMXCallback *callback) : callback(callback) {}

    DmxBuffer dmx;
    std::unique_ptr<DMXCallback> callback;
  };

  typedef std::map<unsigned int, OSCOutputGroup*> OutputGroupMap;
  typedef std::map<std::string, OSCInputGroup*> InputUniverseMap;

  struct SlotMessage {
    unsigned int slot;
    lo_message message;
  };

  ola::io::SelectServerInterface *m_ss;
  const uint16_t m_listen_port;
  std::unique_ptr<ola::io::UnmanagedFileDescriptor> m_descriptor;
  lo_server m_osc_server;
  OutputGroupMap m_output_map;
  InputUniverseMap m_input_map;

  void DescriptorReady();
  bool SendBlob(const DmxBuffer &data, const OSCTargetVector &targets);
  bool SendIndividualFloats(const DmxBuffer &data,
                            OSCOutputGroup *group);
  bool SendIndividualInts(const DmxBuffer &data,
                          OSCOutputGroup *group);
  bool SendIntArray(const DmxBuffer &data,
                    const OSCTargetVector &targets);
  bool SendFloatArray(const DmxBuffer &data,
                      const OSCTargetVector &targets);
  bool SendMessageToTargets(lo_message message,
                            const OSCTargetVector &targets);
  bool SendIndividualMessages(const DmxBuffer &data,
                              OSCOutputGroup *group,
                              const std::string &osc_type);

  static const uint16_t DEFAULT_OSC_PORT = 7770;
  static const char OSC_PORT_VARIABLE[];
};
}  // namespace osc
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_OSC_OSCNODE_H_
