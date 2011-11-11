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
 * slp-register.cpp
 * Copyright (C) 2011 Simon Newton
 *
 */

#include <getopt.h>
#include <ola/Callback.h>
#include <ola/Logging.h>
#include <ola/StringUtils.h>
#include <ola/network/IPV4Address.h>
#include <ola/network/Interface.h>
#include <ola/network/InterfacePicker.h>
#include <ola/network/SelectServer.h>
#include <ola/rdm/UID.h>
#include <signal.h>
#include <sysexits.h>

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "SlpThread.h"

using ola::network::IPV4Address;
using std::string;
using std::vector;

// our command line options
typedef struct {
  string services;
  ola::log_level log_level;
  unsigned short lifetime;
  bool help;
} options;


// stupid globals for now
SlpThread *thread;
ola::network::SelectServer ss;
unsigned int registrations_active;

/**
 * Called when a registration request completes.
 */
void RegisterCallback(bool ok) {
  OLA_INFO << "in register callback, state is " << ok;
}


/**
 * Called when a de-registration request completes.
 */
void DeRegisterCallback(ola::network::SelectServer *ss, bool ok) {
  OLA_INFO << "in deregister callback, state is " << ok;
  if (--registrations_active == 0)
    ss->Terminate();
}


/*
 * Terminate cleanly on interrupt.
 */
static void InteruptSignal(int signo) {
  ss.Terminate();
  (void) signo;
}


/*
 * Parse our cmd line options
 */
void ParseOptions(int argc, char *argv[], options *opts) {
  static struct option long_options[] = {
      {"help", no_argument, 0, 'h'},
      {"log-level", required_argument, 0, 'l'},
      {"timeout", required_argument, 0, 't'},
      {0, 0, 0, 0}
    };

  opts->log_level = ola::OLA_LOG_WARN;
  opts->services = "";
  opts->lifetime = 60;
  opts->help = false;

  int c;
  int option_index = 0;

  while (1) {
    c = getopt_long(argc, argv, "l:ht:", long_options, &option_index);

    if (c == -1)
      break;

    switch (c) {
      case 0:
        break;
      case 'h':
        opts->help = true;
        break;
      case 'l':
        switch (atoi(optarg)) {
          case 0:
            // nothing is written at this level
            // so this turns logging off
            opts->log_level = ola::OLA_LOG_NONE;
            break;
          case 1:
            opts->log_level = ola::OLA_LOG_FATAL;
            break;
          case 2:
            opts->log_level = ola::OLA_LOG_WARN;
            break;
          case 3:
            opts->log_level = ola::OLA_LOG_INFO;
            break;
          case 4:
            opts->log_level = ola::OLA_LOG_DEBUG;
            break;
          default :
            break;
        }
        break;
      case 't':
        opts->lifetime = atoi(optarg);
        break;
      case '?':
        break;
      default:
        break;
    }
  }

  if (optind + 1 == argc)
    opts->services = argv[optind];
}


/*
 * Display the help message
 */
void DisplayHelpAndExit(char arg[]) {
  std::cout << "Usage: " << arg << " [services]\n"
  "\n"
  "Register one or more E1.33 services with SLP. [services] is a comma\n"
  "separated list of IP, UIDs in the form: uid[@ip], e.g. \n"
  "7a70:00000001 (default ip) or 7a70:00000001@192.168.1.1\n"
  "\n"
  "  -h, --help               Display this help message and exit.\n"
  "  -l, --log-level <level>  Set the loggging level 0 .. 4.\n"
  "  -t, --timeout <seconds>  The value to use for the service lifetime\n"
  << std::endl;
  exit(EX_USAGE);
}


/**
 * Process a service spec in the form [ip].UID
 * If ip isn't supplied we use the default one.
 * Returns the canonical service name "ip:uid".
 * If the spec isn't valid we print a warning and exit.
 */
string ProcessServiceSpec(const string &service_spec,
                          const IPV4Address &default_address) {
  vector<string> ip_uid_pair;
  ola::StringSplit(service_spec, ip_uid_pair, "@");

  IPV4Address ipaddr;
  string uid_str;

  if (ip_uid_pair.size() == 1) {
    ipaddr = default_address;
    uid_str = ip_uid_pair[0];
  } else if (ip_uid_pair.size() == 2) {
    if (!IPV4Address::FromString(ip_uid_pair[1], &ipaddr)) {
      OLA_FATAL << "Invalid ip address: " << ip_uid_pair[1];
      exit(EX_USAGE);
    }
    uid_str = ip_uid_pair[0];
  } else {
    OLA_FATAL << "Invalid service spec: " << service_spec;
    exit(EX_USAGE);
  }

  std::auto_ptr<ola::rdm::UID> uid(ola::rdm::UID::FromString(uid_str));
  if (!uid.get()) {
    OLA_FATAL << "Invalid UID: " << uid_str;
    exit(EX_USAGE);
  }

  std::stringstream str;
  str << ipaddr << ":5568/" << std::setfill('0') << std::setw(4) << std::hex
    << uid->ManufacturerId() << std::setw(8) << uid->DeviceId();

  return str.str();
}


/**
 * Process the list of services and build the list of properly formed service
 * names.
 */
void ProcessServices(const string &service_spec, vector<string> *services) {
  std::auto_ptr<ola::network::InterfacePicker> picker(
      ola::network::InterfacePicker::NewPicker());
  ola::network::Interface iface;

  if (!picker->ChooseInterface(&iface, "")) {
    OLA_WARN << "Failed to find interface";
    exit(EX_NOHOST);
  }

  vector<string> service_specs;
  ola::StringSplit(service_spec, service_specs, ",");
  vector<string>::const_iterator iter;

  for (iter = service_specs.begin(); iter != service_specs.end(); ++iter)
    services->push_back(ProcessServiceSpec(*iter, iface.ip_address));
}


/**
 * main
 */
int main(int argc, char *argv[]) {
  options opts;
  ParseOptions(argc, argv, &opts);

  if (opts.help || opts.services == "")
    DisplayHelpAndExit(argv[0]);

  ola::InitLogging(opts.log_level, ola::OLA_LOG_STDERR);

  vector<string> services;
  ProcessServices(opts.services, &services);

  // signal handler
  struct sigaction act, oact;
  act.sa_handler = InteruptSignal;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;

  if (sigaction(SIGINT, &act, &oact) < 0) {
    OLA_WARN << "Failed to install signal SIGINT";
    return false;
  }

  // setup the Slpthread
  thread = new SlpThread(&ss);
  if (!thread->Init()) {
    OLA_WARN << "SlpThread Init() failed";
    delete thread;
    exit(EX_UNAVAILABLE);
  }

  thread->Start();
  vector<string>::const_iterator iter;
  for (iter = services.begin(); iter != services.end(); ++iter)
    thread->Register(
      ola::NewSingleCallback(&RegisterCallback), *iter, opts.lifetime);
  registrations_active = services.size();

  ss.Run();

  // start the de-registration process
  for (iter = services.begin(); iter != services.end(); ++iter) {
  thread->DeRegister(ola::NewSingleCallback(&DeRegisterCallback, &ss),
                     *iter);
  }
  ss.Run();

  thread->Join();
  delete thread;
}
