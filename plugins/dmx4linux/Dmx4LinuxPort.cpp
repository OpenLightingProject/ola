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
 * Dmx4LinuxPort.cpp
 * The DMX 4 Linux plugin
 * Copyright (C) 2006 Simon Newton
 */

#include <string.h>
#include <errno.h>
#include <ola/Constants.h>
#include <ola/Logging.h>

#include "plugins/dmx4linux/Dmx4LinuxPort.h"
#include "plugins/dmx4linux/Dmx4LinuxDevice.h"

namespace ola {
namespace plugin {
namespace dmx4linux {


bool Dmx4LinuxOutputPort::WriteDMX(const DmxBuffer &buffer,
                                   uint8_t priority) {
  int offset = DMX_UNIVERSE_SIZE * m_d4l_universe;
  if (lseek(m_socket->WriteDescriptor(), offset, SEEK_SET) == offset) {
    ssize_t r = m_socket->Send(buffer.GetRaw(), buffer.Size());
    if ((uint) r != buffer.Size()) {
      OLA_WARN << "only wrote " << r << "/" << buffer.Size() << " bytes: " <<
        strerror(errno);
      return false;
    }
  } else {
    OLA_WARN << "failed to seek: " << strerror(errno);
    return false;
  }
  return true;
}

const DmxBuffer &Dmx4LinuxInputPort::ReadDMX() const {
  return m_read_buffer;
}

bool Dmx4LinuxInputPort::UpdateData(const uint8_t *in_buffer,
                                    unsigned int length) {
  DmxBuffer tmp_buffer = DmxBuffer(in_buffer, length);
  if (!(tmp_buffer == m_read_buffer)) {
    m_read_buffer.Set(tmp_buffer);
    DmxChanged();
  }
  return true;
}
}  // namespace dmx4linux
}  // namespace plugin
}  // namespace ola
