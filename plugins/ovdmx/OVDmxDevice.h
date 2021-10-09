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
 * OVDmxDevice.h
 * The OVDMX plugin for ola
 * Copyright (C) 2005 Simon Newton, 2017 Jan Ove Saltvedt
 */

#ifndef PLUGINS_OVDMX_OVDMXDEVICE_H_
#define PLUGINS_OVDMX_OVDMXDEVICE_H_

#include <string>
#include "olad/Device.h"

namespace ola {
namespace plugin {
namespace ovdmx {

class OVDmxDevice: public ola::Device {
 public:
    OVDmxDevice(ola::AbstractPlugin *owner,
                  const std::string &name,
                  const std::string &path,
                  unsigned int device_id);

    // we only support one widget for now
    std::string DeviceId() const { return m_device_id; }

 protected:
    bool StartHook();

 private:
    std::string m_path;
    std::string m_device_id;
};
}  // namespace ovdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_OVDMX_OVDMXDEVICE_H_
