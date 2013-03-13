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

#include <getopt.h>
#include <ola/Callback.h>
#include <ola/Logging.h>
#include <ola/StringUtils.h>
#include <ola/base/Init.h>
#include <ola/io/SelectServer.h>
#include <ola/network/IPV4Address.h>
#include <ola/network/Interface.h>
#include <ola/network/InterfacePicker.h>
#include <ola/rdm/UID.h>
#include <signal.h>
#include <sysexits.h>

#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "tools/e133/OLASLPThread.h"
#include "tools/e133/OpenSLPThread.h"
#include "tools/e133/SLPThread.h"

using ola::network::IPV4Address;
using ola::rdm::UID;
using std::auto_ptr;
using std::multimap;
using std::pair;
using std::string;
using std::vector;

// our command line options
typedef struct {
  bool use_openslp;
  string services;
  ola::log_level log_level;
  uint16_t lifetime;
  bool help;
} options;

// stupid globals for now
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


/*
 * Parse our cmd line options
 */
void ParseOptions(int argc, char *argv[], options *opts) {
  enum {
    OPENSLP_OPTION = 256,
  };

  static struct option long_options[] = {
      {"help", no_argument, 0, 'h'},
      {"log-level", required_argument, 0, 'l'},
      {"timeout", required_argument, 0, 't'},
      {"openslp", no_argument, 0, OPENSLP_OPTION},
      {0, 0, 0, 0}
    };

  opts->log_level = ola::OLA_LOG_WARN;
  opts->services = "";
  opts->lifetime = 60;
  opts->help = false;
  opts->use_openslp = false;

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
      case OPENSLP_OPTION:
        opts->use_openslp = true;
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
  "  -l, --log-level <level>  Set the logging level 0 .. 4.\n"
  "  -t, --timeout <seconds>  The value to use for the service lifetime\n"
  "  --openslp                 Use openslp rather than the OLA SLP server\n"
  << std::endl;
  exit(EX_USAGE);
}


/**
 * Process the list of services and build the list of properly formed service
 * names.
 */
void ProcessServices(const string &service_spec,
                     multimap<IPV4Address, UID> *services) {
  auto_ptr<ola::network::InterfacePicker> picker(
      ola::network::InterfacePicker::NewPicker());
  ola::network::Interface iface;

  if (!picker->ChooseInterface(&iface, "")) {
    OLA_WARN << "Failed to find interface";
    exit(EX_NOHOST);
  }

  vector<string> service_specs;
  ola::StringSplit(service_spec, service_specs, ",");
  vector<string>::const_iterator iter;

  for (iter = service_specs.begin(); iter != service_specs.end(); ++iter) {
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
        exit(EX_USAGE);
      }
      uid_str = ip_uid_pair[0];
    } else {
      OLA_FATAL << "Invalid service spec: " << service_spec;
      exit(EX_USAGE);
    }

    auto_ptr<UID> uid(UID::FromString(uid_str));
    if (!uid.get()) {
      OLA_FATAL << "Invalid UID: " << uid_str;
      exit(EX_USAGE);
    }
    services->insert(pair<IPV4Address, UID>(ipaddr, *uid));
  }
}


/**
 * main
 */
int main(int argc, char *argv[]) {
  ola::AppInit(argc, argv);
  options opts;
  ParseOptions(argc, argv, &opts);

  if (opts.help || opts.services == "")
    DisplayHelpAndExit(argv[0]);

  ola::InitLogging(opts.log_level, ola::OLA_LOG_STDERR);

  multimap<IPV4Address, UID> services;
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

  auto_ptr<BaseSLPThread> slp_thread;
  if (opts.use_openslp) {
    slp_thread.reset(new OpenSLPThread(&ss));
  } else {
    slp_thread.reset(new OLASLPThread(&ss));
  }

  if (!slp_thread->Init()) {
    OLA_WARN << "SlpThread Init() failed";
    exit(EX_UNAVAILABLE);
  }

  if (!slp_thread->Start()) {
    OLA_WARN << "SLPThread Start() failed";
    exit(EX_UNAVAILABLE);
  }
  multimap<IPV4Address, UID>::const_iterator iter;
  for (iter = services.begin(); iter != services.end(); ++iter) {
    slp_thread->RegisterDevice(
      ola::NewSingleCallback(&RegisterCallback),
      iter->first, iter->second, opts.lifetime);
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
