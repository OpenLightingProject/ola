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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
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
#include <ola/io/Descriptor.h>
#include <ola/io/SelectServerInterface.h>
#include <ola/network/SocketAddress.h>
#include <stdint.h>
#include <map>
#include <string>
#include <vector>
#include "plugins/osc/OSCTarget.h"

namespace ola {
namespace plugin {
namespace osc {

using ola::ExportMap;
using ola::io::SelectServerInterface;
using ola::network::IPV4SocketAddress;
using std::map;
using std::string;
using std::vector;

/**
 * The OSCNode object handles sending and receiving DMX data using OSC.
 *
 * Sending:
 *   For sending, OSC Targets are assigned to groups. A group ID is just an
 *   arbitary integer used to identify the group. It's not sent in the OSC
 *   packets. For example:
 *
 *   OSCNode node(OSCNode::OSCNodeOptions(), ...);
 *   node.Init();
 *
 *   node.AddTarget(1, OSCTarget(...));
 *   node.AddTarget(1, OSCTarget(...));
 *   node.SendData(1, ...);
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
    // The options for the OSCNode object.
    struct OSCNodeOptions {
      uint16_t listen_port;  // UDP port to listen on

      OSCNodeOptions() : listen_port(DEFAULT_OSC_PORT) {}
    };

    // The callback run when we receive new DMX data
    typedef Callback1<void, const DmxBuffer&> DMXCallback;

    OSCNode(SelectServerInterface *ss,
            ExportMap *export_map,
            const OSCNodeOptions &options);
    ~OSCNode();

    bool Init();
    void Stop();

    // Sending methods
    void AddTarget(unsigned int group, const OSCTarget &target);
    bool RemoveTarget(unsigned int group, const OSCTarget &target);
    bool SendData(unsigned int group, const ola::DmxBuffer &data);

    // Receiving methods
    bool RegisterAddress(const string &osc_address, DMXCallback *callback);

    // Called by liblo
    void HandleDMXData(const string &osc_address, const DmxBuffer &data);

  private:
    struct NodeOSCTarget: public OSCTarget {
      public:
        lo_address liblo_address;

        explicit NodeOSCTarget(const OSCTarget &target)
          : OSCTarget(target) {
        }
    };
    typedef vector<NodeOSCTarget*> OSCTargetVector;
    typedef map<unsigned int, OSCTargetVector*> OSCTargetMap;
    typedef map<string, DMXCallback*> AddressCallbackMap;

    SelectServerInterface *m_ss;
    const uint16_t m_listen_port;
    ola::io::UnmanagedFileDescriptor *m_descriptor;
    lo_server m_osc_server;
    OSCTargetMap m_target_by_group;
    AddressCallbackMap m_address_callbacks;

    void DescriptorReady();

    static const uint16_t DEFAULT_OSC_PORT = 7770;
    static const char OSC_PORT_VARIABLE[];
};
}  // namespace osc
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_OSC_OSCNODE_H_
