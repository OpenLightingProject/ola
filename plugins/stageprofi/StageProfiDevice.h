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
 * StageProfiDevice.h
 * Interface for the stageprofi device
 * Copyright (C) 2006 Simon Newton
 */

#ifndef PLUGINS_STAGEPROFI_STAGEPROFIDEVICE_H_
#define PLUGINS_STAGEPROFI_STAGEPROFIDEVICE_H_

#include <memory>
#include <string>
#include "ola/network/Socket.h"
#include "olad/Device.h"

namespace ola {

class AbstractPlugin;

namespace plugin {
namespace stageprofi {

class StageProfiDevice: public Device {
 public:
  /**
   * @brief Create a new StageProfi device.
   *
   * @param owner the plugin that owns this device
   * @param name the device name
   * @param dev_path Either the path to the USB Serial device or an IP Address.
   */
  StageProfiDevice(AbstractPlugin *owner,
                   const std::string &name,
                   const std::string &dev_path);

  ~StageProfiDevice();

  // I don't think this get us full stickiness because USB devices may
  // appear as different devices.
  std::string DeviceId() const { return m_path; }

  ola::io::ConnectedDescriptor *GetSocket() const;

 protected:
  bool StartHook();
  void PrePortStop();

 private:
  std::string m_path;
  std::auto_ptr<class StageProfiWidget> m_widget;
};
}  // namespace stageprofi
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_STAGEPROFI_STAGEPROFIDEVICE_H_
