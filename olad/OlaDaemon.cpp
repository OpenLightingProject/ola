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
 * OlaDaemon.cpp
 * This is the main ola daemon
 * Copyright (C) 2005-2008 Simon Newton
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#ifdef _WIN32
#include <Shlobj.h>
#endif
#include <string>

#include "ola/ExportMap.h"
#include "ola/Logging.h"
#include "ola/base/Credentials.h"
#include "ola/base/Flags.h"
#include "ola/file/Util.h"
#include "ola/network/SocketAddress.h"
#include "ola/stl/STLUtils.h"

#include "olad/DynamicPluginLoader.h"
#include "olad/OlaDaemon.h"
#include "olad/OlaServerServiceImpl.h"
#include "olad/PluginLoader.h"
#include "olad/Preferences.h"

DEFINE_s_string(config_dir, c, "",
                "The path to the config directory, Defaults to ~/.ola/ " \
                "on *nix and %LOCALAPPDATA%\\.ola\\ on Windows.");
DEFINE_s_uint16(rpc_port, r, ola::OlaDaemon::DEFAULT_RPC_PORT,
                "The port to listen for RPCs on. Defaults to 9010.");

namespace ola {

using ola::io::SelectServer;
using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;
using ola::network::TCPAcceptingSocket;
using ola::thread::MutexLocker;
using std::auto_ptr;
using std::string;

const char OlaDaemon::K_RPC_PORT_VAR[] = "rpc-port";
const char OlaDaemon::OLA_CONFIG_DIR[] = ".ola";

/*
 * Create a new OlaDaemon
 */
OlaDaemon::OlaDaemon(const OlaServer::Options &options,
                     ExportMap *export_map)
    : m_options(options),
      m_export_map(export_map),
      m_ss(m_export_map) {
  if (m_export_map) {
    IntegerVariable *var = m_export_map->GetIntegerVar(K_RPC_PORT_VAR);
    var->Set(FLAGS_rpc_port);
  }
}


/*
 * Destroy this object
 *
 */
OlaDaemon::~OlaDaemon() {
  Shutdown();
}

/*
 * Initialise this object
 * @return true on success, false on failure
 */
bool OlaDaemon::Init() {
  if (m_server.get()) {
    return false;
  }

  string config_dir = FLAGS_config_dir;
  if (config_dir.empty()) {
    const string default_dir = DefaultConfigDir();
    if (default_dir.empty()) {
      OLA_FATAL << "Unable to determine home directory";
      return false;
    } else {
      config_dir = default_dir;
    }
  }
  // ignore the return code so this isn't fatal
  // in macports the home directory isn't writeable
  InitConfigDir(config_dir);
  OLA_INFO << "Using configs in " << config_dir;
  auto_ptr<PreferencesFactory> preferences_factory(
      new FileBackedPreferencesFactory(config_dir));

  auto_ptr<SelectServer> ss(new SelectServer(m_export_map));
  auto_ptr<OlaClientServiceFactory> service_factory(
      new OlaClientServiceFactory());

  auto_ptr<TCPAcceptingSocket> accepting_socket(
      new TCPAcceptingSocket(NULL));  // factory is added later

  if (!accepting_socket->Listen(
        IPV4SocketAddress(IPV4Address::Loopback(), FLAGS_rpc_port))) {
    OLA_FATAL << "Could not listen on the RPC port, you probably have " <<
      "another instance of olad running";
    return false;
  }

  // Order is important here as we won't load the same plugin twice.
  m_plugin_loaders.push_back(new DynamicPluginLoader());

  auto_ptr<OlaServer> server(
      new OlaServer(service_factory.get(), m_plugin_loaders,
                    preferences_factory.get(), &m_ss, m_options,
                    accepting_socket.get(), m_export_map));

  bool ok = server->Init();
  if (ok) {
    // setup all the members;
    m_preferences_factory.reset(preferences_factory.release());
    m_service_factory.reset(service_factory.release());
    m_accepting_socket.reset(accepting_socket.release());
    m_server.reset(server.release());
  } else {
    STLDeleteElements(&m_plugin_loaders);
  }
  return ok;
}

/*
 * Shutdown the daemon
 */
void OlaDaemon::Shutdown() {
  m_server.reset();
  m_service_factory.reset();
  m_preferences_factory.reset();
  m_accepting_socket.reset();
  STLDeleteElements(&m_plugin_loaders);
}

/*
 * Run the daemon
 */
void OlaDaemon::Run() {
  m_ss.Run();
}

/**
 * Return the address the RPC server is listening on
 */
ola::network::GenericSocketAddress OlaDaemon::RPCAddress() const {
  if (m_accepting_socket.get()) {
    return m_accepting_socket->GetLocalAddress();
  } else {
    return ola::network::GenericSocketAddress();
  }
}

/**
 * Return the home directory for the current user
 */
string OlaDaemon::DefaultConfigDir() {
  if (SupportsUIDs()) {
    PasswdEntry passwd_entry;
    uid_t uid;
    if (!GetUID(&uid))
      return "";
    if (!GetPasswdUID(uid, &passwd_entry))
      return "";

    return passwd_entry.pw_dir + ola::file::PATH_SEPARATOR + OLA_CONFIG_DIR;
  } else {
#ifdef _WIN32
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path))) {
      return string(path) + ola::file::PATH_SEPARATOR + OLA_CONFIG_DIR;
    } else {
      return "";
    }
#else
    return "";
#endif
  }
}

/**
 * Create the config dir if it doesn't exist. This doesn't create parent
 * directories.
 */
bool OlaDaemon::InitConfigDir(const string &path) {
  if (chdir(path.c_str())) {
    // try and create it
#ifdef _WIN32
    if (mkdir(path.c_str())) {
#else
    if (mkdir(path.c_str(), 0755)) {
#endif
      OLA_FATAL << "Couldn't mkdir " << path;
      return false;
    }

    if (chdir(path.c_str())) {
      OLA_FATAL << path << " doesn't exist";
      return false;
    }
  }
  return true;
}
}  // namespace ola
