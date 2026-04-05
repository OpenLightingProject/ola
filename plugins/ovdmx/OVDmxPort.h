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
 * OVDmxPort.h
 * The OVDMX plugin for ola
 * Copyright (C) 2005 Simon Newton, 2017 Jan Ove Saltvedt
 */

#ifndef PLUGINS_OVDMX_OVDMXPORT_H_
#define PLUGINS_OVDMX_OVDMXPORT_H_

#include <string>
#include "ola/DmxBuffer.h"
#include "olad/Port.h"
#include "plugins/ovdmx/OVDmxDevice.h"
#include "plugins/ovdmx/OVDmxThread.h"

namespace ola {
namespace plugin {
namespace ovdmx {

class OVDmxOutputPort: public BasicOutputPort {
 public:
  OVDmxOutputPort(OVDmxDevice *parent,
                    unsigned int id,
                    const std::string &path)
        : BasicOutputPort(parent, id),
        m_thread(path),
        m_path(path) {
        m_thread.Start();
}

  ~OVDmxOutputPort() {
  }

  std::string Description() const { return "OVDMX at " + m_path; }

  bool WriteDMX(OLA_UNUSED const DmxBuffer &buffer,
                OLA_UNUSED uint8_t priority) {
    return m_thread.WriteDmx(buffer);
  }

 private:
  OVDmxThread m_thread;
  std::string m_path;
};
}  // namespace ovdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_OVDMX_OVDMXPORT_H_
