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
 * Dmx4LinuxPort.cpp
 * The DMX 4 Linux plugin
 * Copyright (C) 2006-2008 Simon Newton
 */

#include <string.h>
#include <errno.h>
#include <ola/BaseTypes.h>
#include <ola/Logging.h>

#include "plugins/dmx4linux/Dmx4LinuxPort.h"
#include "plugins/dmx4linux/Dmx4LinuxDevice.h"

namespace ola {
namespace plugin {
namespace dmx4linux {


/*
 * Write operation
 * @param buffer the DmxBuffer to write
 * @return true on success, false on failure
 */
bool Dmx4LinuxOutputPort::WriteDMX(const DmxBuffer &buffer) {
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


/*
 * Read operation
 * @return a DmxBufer with the data
 */
const DmxBuffer &Dmx4LinuxInputPort::ReadDMX() const {
  return m_read_buffer;
}


/*
 * Process new Data
 */
bool Dmx4LinuxInputPort::UpdateData(const uint8_t *in_buffer,
                                    unsigned int length) {
  DmxBuffer tmp_buffer = DmxBuffer(in_buffer, length);
  if (!(tmp_buffer == m_read_buffer)) {
    m_read_buffer.Set(tmp_buffer);
    DmxChanged();
  }
  return true;
}
}  // dmx4linux
}  // plugin
}  // ola
