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
 * DummyDevice.h
 * Interface for the dummy device
 * Copyright (C) 2005 Simon Newton
 */

#ifndef PLUGINS_DUMMY_DUMMYDEVICE_H_
#define PLUGINS_DUMMY_DUMMYDEVICE_H_

#include <string>
#include "olad/Device.h"
#include "plugins/dummy/DummyPort.h"

namespace ola {

class AbstractPlugin;

namespace plugin {
namespace dummy {

class DummyDevice: public Device {
 public:
  DummyDevice(
      AbstractPlugin *owner,
      const std::string &name,
      const DummyPort::Options &port_options)
      : Device(owner, name),
        m_port_options(port_options) {
  }

  std::string DeviceId() const { return "1"; }

 protected:
  const DummyPort::Options m_port_options;

  bool StartHook();
};
}  // namespace dummy
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_DUMMY_DUMMYDEVICE_H_
