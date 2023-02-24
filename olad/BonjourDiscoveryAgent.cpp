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
 * BonjourDiscoveryAgent.cpp
 * The Bonjour implementation of DiscoveryAgentInterface.
 * Copyright (C) 2013 Simon Newton
 */

#define __STDC_LIMIT_MACROS  // for UINT8_MAX & friends

#include "olad/BonjourDiscoveryAgent.h"

#include <dns_sd.h>
#include <stdint.h>
#include <ola/Callback.h>
#include <ola/Logging.h>
#include <ola/network/NetworkUtils.h>
#include <ola/thread/CallbackThread.h>

#include <string>

namespace ola {

using ola::network::HostToNetwork;
using ola::thread::Thread;
using std::auto_ptr;
using std::string;

static void RegisterCallback(OLA_UNUSED DNSServiceRef service,
                             OLA_UNUSED DNSServiceFlags flags,
                             DNSServiceErrorType error_code,
                             const char *name,
                             const char *type,
                             const char *domain,
                             OLA_UNUSED void *context) {
  if (error_code != kDNSServiceErr_NoError) {
    OLA_WARN << "DNSServiceRegister for " << name << "." << type << domain
             << " returned error " << error_code;
  } else {
    OLA_INFO << "Registered: " << name << "." << type << domain;
  }
}

class DNSSDDescriptor : public ola::io::ReadFileDescriptor {
 public:
  explicit DNSSDDescriptor(DNSServiceRef service_ref)
      : m_service_ref(service_ref) {
  }

  int ReadDescriptor() const {
    return DNSServiceRefSockFD(m_service_ref);
  }

  void PerformRead();

 private:
  DNSServiceRef m_service_ref;
};

void DNSSDDescriptor::PerformRead() {
  DNSServiceErrorType error = DNSServiceProcessResult(m_service_ref);
  if (error != kDNSServiceErr_NoError) {
    // TODO(simon): Consider de-registering from the ss here?
    OLA_FATAL << "DNSServiceProcessResult returned " << error;
  }
}

BonjourDiscoveryAgent::RegisterArgs::RegisterArgs(
    const string &service_name,
    const string &type,
    uint16_t port,
    const RegisterOptions &options)
    : RegisterOptions(options),
      service_name(service_name),
      type(type),
      port(port) {
}

BonjourDiscoveryAgent::BonjourDiscoveryAgent()
    : m_thread(new ola::thread::CallbackThread(
          NewSingleCallback(this, &BonjourDiscoveryAgent::RunThread),
          Thread::Options("bonjour"))) {
}

BonjourDiscoveryAgent::~BonjourDiscoveryAgent() {
  m_ss.Terminate();
  m_thread->Join();
  m_thread.reset();

  ServiceRefs::iterator iter = m_refs.begin();
  for (; iter != m_refs.end(); ++iter) {
    m_ss.RemoveReadDescriptor(iter->descriptor);
    delete iter->descriptor;
    DNSServiceRefDeallocate(iter->service_ref);
  }
}

bool BonjourDiscoveryAgent::Init() {
  m_thread->Start();
  return true;
}

void BonjourDiscoveryAgent::RegisterService(const string &service_name,
                                            const string &type,
                                            uint16_t port,
                                            const RegisterOptions &options) {
  RegisterArgs *args = new RegisterArgs(service_name, type, port, options);
  m_ss.Execute(NewSingleCallback(
      this,
      &BonjourDiscoveryAgent::InternalRegisterService, args));
}


void BonjourDiscoveryAgent::InternalRegisterService(RegisterArgs *args_ptr) {
  auto_ptr<RegisterArgs> args(args_ptr);

  OLA_INFO << "Adding " << args->service_name << ", " << args->type;

  ServiceRef ref;
  const string txt_data = BuildTxtRecord(args->txt_data);

  DNSServiceErrorType error = DNSServiceRegister(
      &ref.service_ref,
      0, args->if_index, args->service_name.c_str(), args->type.c_str(),
      args->domain.c_str(),
      NULL,  // use default host name
      HostToNetwork(args->port),
      txt_data.size(), txt_data.c_str(),
      &RegisterCallback,  // call back function
      NULL);  // no context

  if (error != kDNSServiceErr_NoError) {
    OLA_WARN << "DNSServiceRegister returned " << error;
    return;
  }

  ref.descriptor = new DNSSDDescriptor(ref.service_ref);

  m_ss.AddReadDescriptor(ref.descriptor);
  m_refs.push_back(ref);
}

string BonjourDiscoveryAgent::BuildTxtRecord(
    const RegisterOptions::TxtData &txt_data) {
  RegisterOptions::TxtData::const_iterator iter = txt_data.begin();
  string output;
  for (; iter != txt_data.end(); ++iter) {
    unsigned int pair_size = iter->first.size() + iter->second.size() + 1;
    if (pair_size > UINT8_MAX) {
      OLA_WARN << "Discovery data of " << iter->first << ": " << iter->second
               << " exceed " << static_cast<int>(UINT8_MAX)
               << " bytes. Data skipped";
      continue;
    }
    output.append(1, static_cast<char>(pair_size));
    output.append(iter->first);
    output.append("=");
    output.append(iter->second);
  }
  return output;
}

void BonjourDiscoveryAgent::RunThread() {
  m_ss.Run();
  m_ss.DrainCallbacks();
}
}  // namespace ola
