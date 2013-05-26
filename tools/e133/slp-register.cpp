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
 * slp-register.cpp
 * Copyright (C) 2011 Simon Newton
 *
 */
#include <ola/Callback.h>
#include <ola/Logging.h>
#include <ola/StringUtils.h>
#include <ola/base/Flags.h>
#include <ola/base/Init.h>
#include <ola/base/SysExits.h>
#include <ola/e133/SLPThread.h>
#include <ola/io/SelectServer.h>
#include <ola/network/IPV4Address.h>
#include <ola/network/Interface.h>
#include <ola/network/InterfacePicker.h>
#include <ola/rdm/UID.h>
#include <signal.h>

#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <utility>

using ola::network::IPV4Address;
using ola::rdm::UID;
using std::auto_ptr;
using std::multimap;
using std::pair;
using std::string;
using std::vector;

DEFINE_s_uint16(lifetime, t, 60, "The value to use for the service lifetime");

// The SelectServer is global so we can access it from the signal handler.
ola::io::SelectServer ss;

/**
 * Called when a registration request completes.
 */
void RegisterCallback(bool ok) {
  if (ok) {
    OLA_INFO << "Registered E1.33 device";
  } else {
    OLA_WARN << "Failed to register E1.33 device";
  }
}


/**
 * Called when a de-registration request completes.
 */
void DeRegisterCallback(ola::io::SelectServer *ss,
                        unsigned int *registrations_active, bool ok) {
  if (ok) {
    OLA_INFO << "Registered E1.33 device";
  } else {
    OLA_WARN << "Failed to register E1.33 device";
  }
  if (--(*registrations_active) == 0)
    ss->Terminate();
}


/*
 * Terminate cleanly on interrupt.
 */
static void InteruptSignal(int signo) {
  ss.Terminate();
  (void) signo;
}


/**
 * Process the list of services and build the list of properly formed service
 * names.
 */
void ProcessService(const string &service_spec,
                    multimap<IPV4Address, UID> *services) {
  auto_ptr<ola::network::InterfacePicker> picker(
      ola::network::InterfacePicker::NewPicker());
  ola::network::Interface iface;

  if (!picker->ChooseInterface(&iface, "")) {
    OLA_WARN << "Failed to find interface";
    exit(ola::EXIT_NOHOST);
  }

  vector<string> ip_uid_pair;
  ola::StringSplit(service_spec, ip_uid_pair, "@");

  IPV4Address ipaddr;
  string uid_str;

  if (ip_uid_pair.size() == 1) {
    ipaddr = iface.ip_address;
    uid_str = ip_uid_pair[0];
  } else if (ip_uid_pair.size() == 2) {
    if (!IPV4Address::FromString(ip_uid_pair[1], &ipaddr)) {
      OLA_FATAL << "Invalid ip address: " << ip_uid_pair[1];
      exit(ola::EXIT_USAGE);
    }
    uid_str = ip_uid_pair[0];
  } else {
    OLA_FATAL << "Invalid service spec: " << service_spec;
    exit(ola::EXIT_USAGE);
  }

  auto_ptr<UID> uid(UID::FromString(uid_str));
  if (!uid.get()) {
    OLA_FATAL << "Invalid UID: " << uid_str;
    exit(ola::EXIT_USAGE);
  }
  services->insert(std::make_pair(ipaddr, *uid));
}


/**
 * main
 */
int main(int argc, char *argv[]) {
  ola::AppInit(argc, argv);
  ola::SetHelpString("[options] [services]",
    "Register one or more E1.33 services with SLP. [services] is\n"
    "a list of IP, UIDs in the form: uid[@ip], e.g. \n"
    "7a70:00000001 (default ip) or 7a70:00000001@192.168.1.1\n");
  ola::ParseFlags(&argc, argv);
  ola::InitLoggingFromFlags();

  if (argc < 2) {
    ola::DisplayUsage();
    exit(ola::EXIT_USAGE);
  }

  multimap<IPV4Address, UID> services;
  for (int i = 1; i < argc; i++)
    ProcessService(argv[i], &services);

  // signal handler
  ola::InstallSignal(SIGINT, InteruptSignal);

  auto_ptr<ola::e133::BaseSLPThread> slp_thread(
    ola::e133::SLPThreadFactory::NewSLPThread(&ss));

  if (!slp_thread->Init()) {
    OLA_WARN << "SLPThread Init() failed";
    exit(ola::EXIT_UNAVAILABLE);
  }

  if (!slp_thread->Start()) {
    OLA_WARN << "SLPThread Start() failed";
    exit(ola::EXIT_UNAVAILABLE);
  }
  multimap<IPV4Address, UID>::const_iterator iter;
  for (iter = services.begin(); iter != services.end(); ++iter) {
    slp_thread->RegisterDevice(
      ola::NewSingleCallback(&RegisterCallback),
      iter->first, iter->second, FLAGS_lifetime);
  }

  ss.Run();

  // start the de-registration process
  unsigned int registrations_active = services.size();
  for (iter = services.begin(); iter != services.end(); ++iter) {
    slp_thread->DeRegisterDevice(
        ola::NewSingleCallback(&DeRegisterCallback, &ss, &registrations_active),
        iter->first, iter->second);
  }
  ss.Run();

  slp_thread->Join(NULL);
}
