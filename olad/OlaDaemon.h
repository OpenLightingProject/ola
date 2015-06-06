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
 * OlaDaemon.h
 * The OLA Daemon class.
 * Copyright (C) 2005 Simon Newton
 */

#ifndef OLAD_OLADAEMON_H_
#define OLAD_OLADAEMON_H_

#include <memory>
#include <string>
#include <vector>
#include "ola/Constants.h"
#include "ola/ExportMap.h"
#include "ola/base/Macro.h"
#include "ola/io/SelectServer.h"
#include "ola/network/SocketAddress.h"
#include "olad/OlaServer.h"

namespace ola {

class OlaDaemon {
 public:
  /**
   * @brief Create a new OlaDaemon.
   * @param options The Options for the new OlaDaemon.
   * @param export_map an optional ExportMap.
   */
  OlaDaemon(const OlaServer::Options &options,
            ExportMap *export_map = NULL);

  /**
   * @brief Destroy this object.
   */
  ~OlaDaemon();

  /**
   * @brief Initialise the daemon.
   * @return true on success, false on failure.
   */
  bool Init();

  /**
   * @brief Shutdown the daemon.
   */
  void Shutdown();

  /**
   * @brief Run the daemon.
   */
  void Run();

  /**
   * @brief Return the socket address the RPC server is listening on.
   * @returns A socket address, which is empty if the server hasn't been
   *   initialized.
   */
  ola::network::GenericSocketAddress RPCAddress() const;

  /**
   * @brief Get the SelectServer the daemon is using.
   * @returns A SelectServer object.
   */
  ola::io::SelectServer* GetSelectServer() { return &m_ss; }

  /**
   * @brief Get the OlaServer the daemon is using.
   * @returns An OlaServer object.
   */
  OlaServer *GetOlaServer() const { return m_server.get(); }


  static const unsigned int DEFAULT_RPC_PORT = OLA_DEFAULT_PORT;

 private:
  const OlaServer::Options m_options;
  class ExportMap *m_export_map;
  ola::io::SelectServer m_ss;
  std::vector<class PluginLoader*> m_plugin_loaders;

  // Populated in Init()
  std::auto_ptr<class PreferencesFactory> m_preferences_factory;
  std::auto_ptr<OlaServer> m_server;

  std::string DefaultConfigDir();
  bool InitConfigDir(const std::string &path);

  static const char OLA_CONFIG_DIR[];
  static const char CONFIG_DIR_KEY[];
  static const char UID_KEY[];
  static const char USER_NAME_KEY[];
  static const char GID_KEY[];
  static const char GROUP_NAME_KEY[];

  DISALLOW_COPY_AND_ASSIGN(OlaDaemon);
};
}  // namespace ola
#endif  // OLAD_OLADAEMON_H_
