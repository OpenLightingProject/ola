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
 * StageProfiPort.cpp
 * The StageProfi plugin for ola
 * Copyright (C) 2006-2009 Simon Newton
 */

#include <string.h>
#include "plugins/stageprofi/StageProfiPort.h"
#include "plugins/stageprofi/StageProfiDevice.h"

namespace ola {
namespace plugin {
namespace stageprofi {

/*
 * Write operation
 * @param the buffer to write
 * @return true on success, false on failure
 */
bool StageProfiOutputPort::WriteDMX(const DmxBuffer &buffer,
                                    uint8_t priority) {
  return m_widget->SendDmx(buffer);
  (void) priority;
}
}  // namespace stageprofi
}  // namespace plugin
}  // namespace ola
