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
 * e133-receiver.cpp
 * Copyright (C) 2011 Simon Newton
 *
 * This creates a E1.33 receiver with one (emulated) RDM responder. The node is
 * registered in slp and the RDM responder responds to E1.33 commands.
 */

#ifndef TOOLS_E133_SIMPLEE133NODE_H_
#define TOOLS_E133_SIMPLEE133NODE_H_

#include <ola/BaseTypes.h>
#include <ola/base/Init.h>
#include <ola/acn/CID.h>
#include <ola/e133/SLPThread.h>
#include <ola/io/SelectServer.h>
#include <ola/io/StdinHandler.h>
#include <ola/network/NetworkUtils.h>
#include <ola/rdm/RDMCommand.h>
#include <ola/rdm/UID.h>

#include <memory>

#include "tools/e133/E133Device.h"
#include "tools/e133/EndpointManager.h"
#include "tools/e133/ManagementEndpoint.h"
#include "tools/e133/TCPConnectionStats.h"

using ola::acn::CID;
using ola::network::IPV4Address;
using ola::rdm::UID;
using std::auto_ptr;

/**
 * A very simple E1.33 node that registers itself using SLP and responds to
 * messages.
 */
class SimpleE133Node {
  public:
    struct Options {
      CID cid;
      IPV4Address ip_address;
      UID uid;
      uint16_t lifetime;

      Options(const CID &cid,
              const IPV4Address &ip,
              const UID &uid,
              uint16_t lifetime)
        : cid(cid),
          ip_address(ip),
          uid(uid),
          lifetime(lifetime) {
      }
    };

    explicit SimpleE133Node(const Options &options);
    ~SimpleE133Node();

    ola::io::SelectServer *SelectServer() { return &m_ss; }

    bool Init();
    void Run();
    void Stop() { m_ss.Terminate(); }

    // Ownership not passed.
    void AddEndpoint(uint16_t endpoint_id, E133Endpoint *endpoint);
    void RemoveEndpoint(uint16_t endpoint_id);

  private:
    ola::io::SelectServer m_ss;
    auto_ptr<ola::e133::BaseSLPThread> m_slp_thread;
    ola::io::StdinHandler m_stdin_handler;
    EndpointManager m_endpoint_manager;
    E133Device m_e133_device;
    ManagementEndpoint m_management_endpoint;
    const uint16_t m_lifetime;
    const UID m_uid;
    const IPV4Address m_ip_address;

    void RegisterCallback(bool ok);
    void DeRegisterCallback(bool ok);

    void Input(char c);
    void DumpTCPStats();
    void SendUnsolicited();

    SimpleE133Node(const SimpleE133Node&);
    SimpleE133Node operator=(const SimpleE133Node&);
};
#endif  // TOOLS_E133_SIMPLEE133NODE_H_
