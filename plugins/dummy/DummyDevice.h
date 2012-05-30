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
 * DummyDevice.h
 * Interface for the dummy device
 * Copyright (C) 2005-2008 Simon Newton
 */

#ifndef PLUGINS_DUMMY_DUMMYDEVICE_H_
#define PLUGINS_DUMMY_DUMMYDEVICE_H_

#include <string>
#include "olad/Device.h"

namespace ola {

class AbstractPlugin;

namespace plugin {
namespace dummy {

using std::string;

class DummyDevice: public Device {
  public:
    DummyDevice(
        AbstractPlugin *owner,
        const string &name,
        uint16_t number_of_devices,
        uint16_t number_of_subdevices):
      Device(owner, name),
      m_number_of_devices(number_of_devices),
      m_number_of_subdevices(number_of_subdevices) {
    }
    string DeviceId() const { return "1"; }

  protected:
    bool StartHook();
    uint16_t m_number_of_devices;
    uint16_t m_number_of_subdevices;
};
}  // dummy
}  // plugin
}  // ola
#endif  // PLUGINS_DUMMY_DUMMYDEVICE_H_
