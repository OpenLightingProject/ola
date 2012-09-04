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
 * slp-server.cpp
 * Copyright (C) 2012 Simon Newton
 */

#include <stdio.h>
#include <getopt.h>
#include <signal.h>
#include <sysexits.h>

#include <ola/ExportMap.h>
#include <ola/Logging.h>
#include <ola/StringUtils.h>
#include <ola/base/Credentials.h>
#include <ola/base/Init.h>
#include <ola/network/InterfacePicker.h>
#include <ola/network/Socket.h>
#include <ola/network/SocketAddress.h>

#include <memory>
#include <string>

#include "tools/slp/SLPServer.h"

using ola::network::IPV4SocketAddress;
using ola::network::TCPAcceptingSocket;
using ola::network::UDPSocket;
using ola::network::IPV4SocketAddress;
using ola::slp::SLPServer;
using std::auto_ptr;
using std::cout;
using std::endl;
using std::string;


SLPServer *server = NULL;


struct SLPOptions {
  public:
    bool help;
    ola::log_level log_level;
    string preferred_ip_address;
    uint16_t slp_port;
    string setuid;
    string setgid;

    SLPOptions()
        : help(false),
          log_level(ola::OLA_LOG_WARN),
          slp_port(SLPServer::DEFAULT_SLP_PORT) {
    }
};


/*
 * Parse our command line options
 */
void ParseOptions(int argc, char *argv[],
                  SLPOptions *options,
                  SLPServer::SLPServerOptions *slp_options) {
  int enable_da = slp_options->enable_da;
  int enable_http = slp_options->enable_http;

  enum {
    SETUID_OPTION = 256,
    SETGID_OPTION = 257,
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
        options->slp_port = atoi(optarg);
        break;
      case SETUID_OPTION:
        options->setuid = optarg;
        break;
      case SETGID_OPTION:
        options->setgid = optarg;
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
  << endl;
  exit(0);
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
 * Add our variables to the export map
 */
void InitExportMap(ola::ExportMap *export_map, const SLPOptions &options) {
  ola::IntegerVariable *slp_port = export_map->GetIntegerVar("slp-port");
  slp_port->Set(options.slp_port);
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

/*
 * Startup the server.
 */
int main(int argc, char *argv[]) {
  SLPOptions options;
  SLPServer::SLPServerOptions slp_options;
  ola::ExportMap export_map;

  ParseOptions(argc, argv, &options, &slp_options);

  if (options.help)
    DisplayHelpAndExit(argv);

  ola::InitLogging(options.log_level, ola::OLA_LOG_STDERR);

  {
    // find an interface to use
    ola::network::Interface interface;
    auto_ptr<const ola::network::InterfacePicker> picker(
      ola::network::InterfacePicker::NewPicker());
    if (!picker->ChooseInterface(&interface, options.preferred_ip_address)) {
      OLA_INFO << "Failed to find an interface";
      exit(EX_UNAVAILABLE);
    }
    slp_options.ip_address = interface.ip_address;
  }

  auto_ptr<UDPSocket> udp_socket(SetupUDPSocket(options.slp_port));
  if (!udp_socket.get())
    exit(EX_UNAVAILABLE);

  auto_ptr<TCPAcceptingSocket> tcp_socket(
      SetupTCPSocket(slp_options.ip_address, options.slp_port));
  if (!tcp_socket.get())
    exit(EX_UNAVAILABLE);

  if (!DropPrivileges(options.setuid, options.setgid))
    exit(EX_UNAVAILABLE);

  ola::ServerInit(argc, argv, &export_map);
  InitExportMap(&export_map, options);

  SLPServer *server = new SLPServer(udp_socket.get(), tcp_socket.get(),
                                    slp_options, &export_map);
  if (!server->Init())
    exit(EX_UNAVAILABLE);

  cout << "---------------  Controls  ----------------\n";
  cout << " q - Quit\n";
  cout << "-------------------------------------------\n";
  ola::InstallSignal(SIGINT, InteruptSignal);
  server->Run();
  delete server;
}
