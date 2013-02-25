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
 * Dmx4LinuxDevice.h
 * Interface for the dmx4linux device
 * Copyright (C) 2006-2009 Simon Newton
 */

#ifndef PLUGINS_DMX4LINUX_DMX4LINUXDEVICE_H_
#define PLUGINS_DMX4LINUX_DMX4LINUXDEVICE_H_

#include <string>
#include "olad/Device.h"

namespace ola {
namespace plugin {
namespace dmx4linux {

class Dmx4LinuxDevice: public ola::Device {
  public:
    Dmx4LinuxDevice(class Dmx4LinuxPlugin *owner,
                    const string &name,
                    const string &device_id);
    string DeviceId() const { return m_device_id; }

  private:
    string m_device_id;
};
}  // dmx4linux
}  // plugin
}  // ola
#endif  // PLUGINS_DMX4LINUX_DMX4LINUXDEVICE_H_
