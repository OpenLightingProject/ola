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
 * slp-server.cpp
 * Copyright (C) 2012 Simon Newton
 */

#include <stdio.h>
#include <getopt.h>
#include <signal.h>
#ifndef _WIN32
#include <sysexits.h>
#endif

#include <ola/ExportMap.h>
#include <ola/Logging.h>
#include <ola/StringUtils.h>
#include <ola/base/Credentials.h>
#include <ola/base/Init.h>
#include <ola/base/SysExits.h>
#include <ola/network/InterfacePicker.h>
#include <ola/network/Socket.h>
#include <ola/network/SocketAddress.h>

#include <memory>
#include <string>
#include <vector>

#include "slp/RegistrationFileParser.h"
#include "slp/SLPDaemon.h"
#include "slp/ServerCommon.h"
#include "slp/ServiceEntry.h"

using ola::network::IPV4SocketAddress;
using ola::network::IPV4SocketAddress;
using ola::network::TCPAcceptingSocket;
using ola::network::UDPSocket;
using ola::slp::RegistrationFileParser;
using ola::slp::SLPDaemon;
using ola::slp::ServiceEntries;
using std::auto_ptr;
using std::cout;
using std::endl;
using std::string;


SLPDaemon *server = NULL;


struct SLPOptions {
 public:
    bool help;
    ola::log_level log_level;
    string preferred_ip_address;
    string setuid;
    string setgid;
    string scopes;
    string registration_file;

    SLPOptions()
        : help(false),
          log_level(ola::OLA_LOG_WARN) {
    }
};


/*
 * Parse our command line options
 */
void ParseOptions(int argc, char *argv[],
                  SLPOptions *options,
                  SLPDaemon::SLPDaemonOptions *slp_options) {
  int enable_da = slp_options->enable_da;
  int enable_http = slp_options->enable_http;

  enum {
    SETUID_OPTION = 256,
    SETGID_OPTION = 257,
    SCOPE_OPTION  = 258,
    REGISTRATION_OPTION  = 259,
  };

  static struct option long_options[] = {
      {"help", no_argument, 0, 'h'},
      {"ip", required_argument, 0, 'i'},
      {"log-level", required_argument, 0, 'l'},
      {"slp-port", required_argument, 0, 'p'},
      {"no-da", no_argument, &enable_da, 0},
      {"no-http", no_argument, &enable_http, 0},
      {"setuid", required_argument, 0, SETUID_OPTION},
      {"setgid", required_argument, 0, SETGID_OPTION},
      {"scopes", required_argument, 0, SCOPE_OPTION},
      {"services", required_argument, 0, REGISTRATION_OPTION},
      {0, 0, 0, 0}
  };

  int option_index = 0;

  while (1) {
    int c = getopt_long(argc, argv, "hi:l:p:", long_options, &option_index);

    if (c == -1)
      break;

    switch (c) {
      case 'h':
        options->help = true;
        break;
      case 'i':
        options->preferred_ip_address = optarg;
        break;
      case 'l':
        switch (atoi(optarg)) {
          case 0:
            // nothing is written at this level
            // so this turns logging off
            options->log_level = ola::OLA_LOG_NONE;
            break;
          case 1:
            options->log_level = ola::OLA_LOG_FATAL;
            break;
          case 2:
            options->log_level = ola::OLA_LOG_WARN;
            break;
          case 3:
            options->log_level = ola::OLA_LOG_INFO;
            break;
          case 4:
            options->log_level = ola::OLA_LOG_DEBUG;
            break;
          default :
            break;
        }
        break;
      case 'p':
        slp_options->slp_port = atoi(optarg);
        break;
      case SETUID_OPTION:
        options->setuid = optarg;
        break;
      case SETGID_OPTION:
        options->setgid = optarg;
        break;
      case SCOPE_OPTION:
        options->scopes = optarg;
        break;
      case REGISTRATION_OPTION:
        options->registration_file = optarg;
        break;
      case '?':
        break;
      default:
       break;
    }
  }
  slp_options->enable_da = enable_da;
  slp_options->enable_http = enable_http;
}


/*
 * Display the help message
 */
void DisplayHelpAndExit(char *argv[]) {
  cout << "Usage: " << argv[0] << " [options]\n"
  "\n"
  "Run the SLP server.\n"
  "\n"
  "  -h, --help               Display this help message and exit.\n"
  "  -i, --ip                 The IP address to listen on.\n"
  "  -l, --log-level <level>  Set the logging level 0 .. 4.\n"
  "  -p, --slp-port           The SLP port to listen on (default 427).\n"
  "  --no-http                Don't run the http server\n"
  "  --no-da                  Disable DA functionality\n"
  "  --setuid <uid,user>      User to switch to after startup\n"
  "  --setgid <gid,group>     Group to switch to after startup\n"
  "  --scopes <scope-list>    Commas separated list of scopes\n"
  "  --services <file>        Services to pre-register\n"
  << endl;
  exit(0);
}


bool CheckSLPOptions(const SLPOptions &options,
                     SLPDaemon::SLPDaemonOptions *slp_options) {
  if (!options.scopes.empty()) {
    vector<string> scope_list;
    ola::StringSplit(options.scopes, scope_list, ",");
    vector<string>::const_iterator iter = scope_list.begin();
    for (; iter != scope_list.end(); ++iter)
      slp_options->scopes.insert(*iter);
  }
  return true;
}

/**
 * Create the UDP Socket and bind to the port. We do this outside the server so
 * we can't bind to ports < 1024.
 */
UDPSocket *SetupUDPSocket(uint16_t port) {
  UDPSocket *socket = new UDPSocket();

  // setup the UDP socket
  if (!socket->Init()) {
    OLA_WARN << "Failed to Init UDP Socket";
    return NULL;
  }

  if (!socket->Bind(IPV4SocketAddress(IPV4Address::WildCard(), port))) {
    OLA_WARN << "UDP Port failed to find";
    return NULL;
  }
  return socket;
}


/**
 * Create the TCP Socket and bind to the port. We do this outside the server so
 * we can't bind to ports < 1024.
 */
TCPAcceptingSocket *SetupTCPSocket(const IPV4Address ip, uint16_t port) {
  TCPAcceptingSocket *socket = new TCPAcceptingSocket(NULL);

  // setup the TCP socket
  if (!socket->Listen(IPV4SocketAddress(ip, port))) {
    OLA_WARN << "Failed to Init TCP Socket";
    return NULL;
  }
  return socket;
}


/**
 * Interupt handler
 */
static void InteruptSignal(int unused) {
  if (server)
    server->Stop();
  (void) unused;
}


/**
 * Drop Privileges if required.
 */
bool DropPrivileges(const string &setuid, const string &setgid) {
  if (!setuid.empty()) {
    ola::PasswdEntry passwd_entry;
    bool ok = false;
    uid_t uid;
    if (ola::StringToInt(setuid, &uid, true)) {
      ok = ola::GetPasswdUID(uid, &passwd_entry);
    } else {
      ok = ola::GetPasswdName(setuid, &passwd_entry);
    }
    if (!ok) {
      OLA_WARN << "Unknown UID or username: " << setuid;
      return false;
    }
    if (!ola::SetUID(passwd_entry.pw_uid)) {
      OLA_WARN << "Failed to setuid to: " << setuid;
      return false;
    }
  }

  if (!setgid.empty()) {
    ola::GroupEntry group_entry;
    bool ok = false;
    gid_t gid;
    if (ola::StringToInt(setgid, &gid, true)) {
      ok = ola::GetGroupGID(gid, &group_entry);
    } else {
      ok = ola::GetGroupName(setgid, &group_entry);
    }
    if (!ok) {
      OLA_WARN << "Unknown GID or group: " << setgid;
      return false;
    }
    if (!ola::SetGID(group_entry.gr_gid)) {
      OLA_WARN << "Failed to setgid to: " << setgid;
      return false;
    }
  }
  return true;
}


void PreRegisterServices(SLPDaemon *daemon, const string &file) {
  RegistrationFileParser parser;
  ServiceEntries services;
  bool ok = parser.ParseFile(file, &services);
  OLA_INFO << "parse file returned " << ok;
  ok = daemon->BulkLoad(services);
  OLA_INFO << "load returned " << ok;
}

/*
 * Startup the server.
 */
int main(int argc, char *argv[]) {
  SLPOptions options;
  SLPDaemon::SLPDaemonOptions slp_options;
  ola::ExportMap export_map;

  ParseOptions(argc, argv, &options, &slp_options);

  if (options.help)
    DisplayHelpAndExit(argv);

  ola::InitLogging(options.log_level, ola::OLA_LOG_STDERR);

  if (!CheckSLPOptions(options, &slp_options))
    DisplayHelpAndExit(argv);

  {
    // find an interface to use
    ola::network::Interface iface;
    auto_ptr<const ola::network::InterfacePicker> picker(
      ola::network::InterfacePicker::NewPicker());
    if (!picker->ChooseInterface(&iface, options.preferred_ip_address)) {
      OLA_INFO << "Failed to find an interface";
      exit(ola::EXIT_UNAVAILABLE);
    }
    slp_options.ip_address = iface.ip_address;
  }

  auto_ptr<UDPSocket> udp_socket(SetupUDPSocket(slp_options.slp_port));
  if (!udp_socket.get())
    exit(ola::EXIT_UNAVAILABLE);

  auto_ptr<TCPAcceptingSocket> tcp_socket(
      SetupTCPSocket(slp_options.ip_address, slp_options.slp_port));
  if (!tcp_socket.get())
    exit(ola::EXIT_UNAVAILABLE);

  if (!DropPrivileges(options.setuid, options.setgid))
    exit(ola::EXIT_UNAVAILABLE);

  ola::ServerInit(argc, argv, &export_map);

  SLPDaemon *daemon = new SLPDaemon(udp_socket.get(), tcp_socket.get(),
                                    slp_options, &export_map);
  if (!daemon->Init())
    exit(ola::EXIT_UNAVAILABLE);

  if (!options.registration_file.empty())
    PreRegisterServices(daemon, options.registration_file);

  cout << "---------------  Controls  ----------------\n";
  cout << " a - Start active DA discovery\n";
  cout << " d - Print Known DAs\n";
  cout << " p - Print Registrations\n";
  cout << " q - Quit\n";
  cout << "-------------------------------------------\n";
  ola::InstallSignal(SIGINT, InteruptSignal);
  daemon->Run();
  delete daemon;
}
