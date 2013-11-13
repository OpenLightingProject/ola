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
 * BonjourDiscoveryAgent.h
 * The Bonjour implementation of DiscoveryAgentInterface.
 * Copyright (C) 2013 Simon Newton
 */

#ifndef OLAD_BONJOURDISCOVERYAGENT_H_
#define OLAD_BONJOURDISCOVERYAGENT_H_

#include <dns_sd.h>

#include <ola/base/Macro.h>
#include <ola/io/Descriptor.h>
#include <ola/io/SelectServer.h>
#include <memory>
#include <string>
#include <vector>

#include "olad/DiscoveryAgent.h"

namespace ola {
namespace thread {
class CallbackThread;
}

/**
 * @class An implementation of DiscoveryAgentInterface that uses the Apple
 * dns_sd.h library.
 */
class BonjourDiscoveryAgent : public DiscoveryAgentInterface {
  public:
    BonjourDiscoveryAgent();
    ~BonjourDiscoveryAgent();

    bool Init();

    void RegisterService(const std::string &service_name,
                         const std::string &type,
                         uint16_t port,
                         const RegisterOptions &options);

  private:
    struct RegisterArgs : public RegisterOptions {
      std::string service_name;
      std::string type;
      uint16_t port;

      RegisterArgs(const std::string &service_name,
                   const std::string &type,
                   uint16_t port,
                   const RegisterOptions &options);
    };

    struct ServiceRef {
      // DNSServiceRef is just a pointer.
      DNSServiceRef service_ref;
      class DNSSDDescriptor *descriptor;
    };

    typedef vector<ServiceRef> ServiceRefs;

    ola::io::SelectServer m_ss;
    std::auto_ptr<thread::CallbackThread> m_thread;
    ServiceRefs m_refs;

    void InternalRegisterService(RegisterArgs *args);
    string BuildTxtRecord(const RegisterOptions::TxtData &txt_data);
    DISALLOW_COPY_AND_ASSIGN(BonjourDiscoveryAgent);
};
}  // namespace ola
#endif  // OLAD_BONJOURDISCOVERYAGENT_H_
