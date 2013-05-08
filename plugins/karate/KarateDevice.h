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
 *
 * karatedevice.h
 * Interface for the Open DMX device
 * Copyright (C) 2005  Simon Newton
 */

#ifndef PLUGINS_KARATE_KARATEDEVICE_H_
#define PLUGINS_KARATE_KARATEDEVICE_H_

#include <string>
#include "olad/Device.h"

namespace ola {
namespace plugin {
namespace karate {

using std::string;

class KarateDevice: public ola::Device {
  public:
    KarateDevice(ola::AbstractPlugin *owner,
                  const string &name,
                  const string &path,
                  unsigned int device_id);

    // we only support one widget for now
    string DeviceId() const { return m_device_id; }

  protected:
    bool StartHook();

  private:
    string m_path;
    string m_device_id;
};
}  // namespace karate
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_KARATE_KARATEDEVICE_H_
