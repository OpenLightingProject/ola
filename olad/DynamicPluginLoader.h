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
 * DynamicPluginLoader.h
 * Interface for the DynamicPluginLoader class
 * Copyright (C) 2005 Simon Newton
 */

#ifndef OLAD_DYNAMICPLUGINLOADER_H_
#define OLAD_DYNAMICPLUGINLOADER_H_

#include <vector>
#include "ola/base/Macro.h"
#include "olad/PluginLoader.h"

namespace ola {

/**
 * @brief A PluginLoader which loads from shared (dynamic) libraries.
 */
class DynamicPluginLoader: public PluginLoader {
 public:
  DynamicPluginLoader() {}
  ~DynamicPluginLoader();

  std::vector<class AbstractPlugin*> LoadPlugins();

  void UnloadPlugins();

 private:
  void PopulatePlugins();

  std::vector<class AbstractPlugin*> m_plugins;

  DISALLOW_COPY_AND_ASSIGN(DynamicPluginLoader);
};
}  // namespace ola
#endif  // OLAD_DYNAMICPLUGINLOADER_H_
