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
 *
 * renarddevice.h
 * Interface for the Renard device
 * Copyright (C) 2013 Hakan Lindestaf
 */

#ifndef PLUGINS_RENARD_RENARDDEVICE_H_
#define PLUGINS_RENARD_RENARDDEVICE_H_

#include <string>
#include "olad/Device.h"

namespace ola {
namespace plugin {
namespace renard {

using std::string;

class RenardDevice: public ola::Device {
  public:
    RenardDevice(ola::AbstractPlugin *owner,
                  const string &name,
                  const string &path,
                  int fd,
                  unsigned int device_id);
    virtual ~RenardDevice();

    // we only support one widget for now
    string DeviceId() const { return m_device_id; }

  protected:
    bool StartHook();

  private:
    string m_path;
    string m_device_id;
    ola::io::ConnectedDescriptor *m_descriptor;
};
}  // namespace renard
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_RENARD_RENARDDEVICE_H_
