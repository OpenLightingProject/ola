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
 * OlaDaemon.cpp
 * This is the main ola daemon
 * Copyright (C) 2005 Simon Newton
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#ifdef _WIN32
#include <Shlobj.h>
#endif  // _WIN32
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
                "The path to the config directory, defaults to ~/.ola/ " \
                "on *nix and %LOCALAPPDATA%\\.ola\\ on Windows.");

namespace ola {

using ola::io::SelectServer;
using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;
using ola::network::TCPAcceptingSocket;
using ola::thread::MutexLocker;
using std::auto_ptr;
using std::string;

const char OlaDaemon::OLA_CONFIG_DIR[] = ".ola";
const char OlaDaemon::CONFIG_DIR_KEY[] = "config-dir";
const char OlaDaemon::UID_KEY[] = "uid";
const char OlaDaemon::GID_KEY[] = "gid";
const char OlaDaemon::USER_NAME_KEY[] = "user";
const char OlaDaemon::GROUP_NAME_KEY[] = "group";

OlaDaemon::OlaDaemon(const OlaServer::Options &options,
                     ExportMap *export_map)
    : m_options(options),
      m_export_map(export_map),
      m_ss(m_export_map) {
  if (m_export_map) {
    uid_t uid;
    if (GetUID(&uid)) {
      m_export_map->GetIntegerVar(UID_KEY)->Set(uid);
      PasswdEntry passwd;
      if (GetPasswdUID(uid, &passwd)) {
        m_export_map->GetStringVar(USER_NAME_KEY)->Set(passwd.pw_name);
      }
    }

    gid_t gid;
    if (GetGID(&gid)) {
      m_export_map->GetIntegerVar(GID_KEY)->Set(gid);
      GroupEntry group;
      if (GetGroupGID(gid, &group)) {
        m_export_map->GetStringVar(GROUP_NAME_KEY)->Set(group.gr_name);
      }
    }
  }
}


OlaDaemon::~OlaDaemon() {
  Shutdown();
}

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

  // Ignore the return code so this isn't fatal
  // in macports the home directory isn't writeable
  InitConfigDir(config_dir);
  OLA_INFO << "Using configs in " << config_dir;
  if (m_export_map) {
    m_export_map->GetStringVar(CONFIG_DIR_KEY)->Set(config_dir);
  }
  auto_ptr<PreferencesFactory> preferences_factory(
      new FileBackedPreferencesFactory(config_dir));

  // Order is important here as we won't load the same plugin twice.
  m_plugin_loaders.push_back(new DynamicPluginLoader());

  auto_ptr<OlaServer> server(
      new OlaServer(m_plugin_loaders,
                    preferences_factory.get(), &m_ss, m_options,
                    NULL, m_export_map));

  bool ok = server->Init();
  if (ok) {
    // Set the members
    m_preferences_factory.reset(preferences_factory.release());
    m_server.reset(server.release());
  } else {
    STLDeleteElements(&m_plugin_loaders);
  }
  return ok;
}

void OlaDaemon::Shutdown() {
  m_server.reset();
  m_preferences_factory.reset();
  STLDeleteElements(&m_plugin_loaders);
}

void OlaDaemon::Run() {
  m_ss.Run();
}

ola::network::GenericSocketAddress OlaDaemon::RPCAddress() const {
  if (m_server.get()) {
    return m_server->LocalRPCAddress();
  } else {
    return ola::network::GenericSocketAddress();
  }
}

/*
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
#endif  // _WIN32
  }
}

/*
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
#endif  // _WIN32
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
