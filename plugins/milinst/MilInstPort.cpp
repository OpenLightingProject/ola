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
 * MilInstPort.cpp
 * The MilInst plugin for ola
 * Copyright (C) 2013 Peter Newman
 */

#include "plugins/milinst/MilInstPort.h"

namespace ola {
namespace plugin {
namespace milinst {

bool MilInstOutputPort::WriteDMX(const DmxBuffer &buffer,
                                 OLA_UNUSED uint8_t priority) {
  return m_widget->SendDmx(buffer);
}
}  // namespace milinst
}  // namespace plugin
}  // namespace ola
