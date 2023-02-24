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
 * PluginLoader.h
 * Interface for the PluginLoader classes
 * Copyright (C) 2005 Simon Newton
 */

#ifndef OLAD_PLUGINLOADER_H_
#define OLAD_PLUGINLOADER_H_

#include <vector>

#include "ola/base/Macro.h"

namespace ola {

/**
 * @brief The interface used to load plugins.
 */
class PluginLoader {
 public:
  PluginLoader() : m_plugin_adaptor(NULL) {}
  virtual ~PluginLoader() {}

  /**
   * @brief Set the PluginAdaptor to use for the plugins.
   * @param adaptor The PluginAdaptor, ownership is not transferred.
   */
  void SetPluginAdaptor(class PluginAdaptor *adaptor) {
    m_plugin_adaptor = adaptor;
  }

  /**
   * @brief Load the plugins.
   * @returns A vector with a list of the plugins which were loaded. The
   *   PluginLoader maintains ownership of each plugin.
   */
  virtual std::vector<class AbstractPlugin*> LoadPlugins() = 0;

  /**
   * @brief Unload all previously loaded plugins.
   *
   * After this call completes, any plugins returned by LoadPlugins() must not
   * be used.
   */
  virtual void UnloadPlugins() = 0;

 protected:
  class PluginAdaptor *m_plugin_adaptor;

 private:
  DISALLOW_COPY_AND_ASSIGN(PluginLoader);
};
}  // namespace ola
#endif  // OLAD_PLUGINLOADER_H_
