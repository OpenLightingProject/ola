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
 * PluginImplInterface.h
 * The interface for the various implementations of the USBDMX plugin.
 * Copyright (C) 2014 Simon Newton
 */

#ifndef PLUGINS_USBDMX_PLUGINIMPLINTERFACE_H_
#define PLUGINS_USBDMX_PLUGINIMPLINTERFACE_H_

#include <libusb.h>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include "ola/plugin_id.h"
#include "olad/Plugin.h"
#include "ola/io/Descriptor.h"

namespace ola {
namespace plugin {
namespace usbdmx {

/**
 * @brief The interface for an implementation of the USB DMX plugin.
 */
class PluginImplInterface {
 public:
  virtual ~PluginImplInterface() {}

  /**
   * @brief Start the implementation.
   * @returns true if successful, false otherwise.
   */
  virtual bool Start() = 0;

  /**
   * @brief Stop the implementation.
   * @returns true if successful, false otherwise.
   */
  virtual bool Stop() = 0;
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_PLUGINIMPLINTERFACE_H_
