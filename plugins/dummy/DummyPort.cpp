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
 * DummyPort.cpp
 * The Dummy Port for ola
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <ola/Logging.h>
#include "DummyPort.h"

namespace ola {
namespace plugin {

/*
 * Write operation
 * @param  data  pointer to the dmx data
 * @param  length  the length of the data
 */
bool DummyPort::WriteDMX(const DmxBuffer &buffer) {
  m_buffer = buffer;
  string data = buffer.Get();

  OLA_INFO << "Dummy port: got " << buffer.Size() << " bytes: " << std::hex <<
    "0x" << data.at(0) <<
    "0x" << data.at(1) <<
    "0x" << data.at(2) <<
    "0x" << data.at(3);
  return true;
}

} //plugin
} //ola
