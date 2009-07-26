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

#ifndef DUMMYDEVICE_H
#define DUMMYDEVICE_H

#include <string>
#include <olad/Device.h>

namespace ola {

class AbstractPlugin;

namespace plugin {

using std::string;

class DummyDevice: public Device {
  public:
    DummyDevice(AbstractPlugin *owner, const string &name):
      Device(owner, name) {}
    bool Start();
    bool Stop();
    string DeviceId() const { return "1"; }
};

} //plugin
} // ola
#endif
