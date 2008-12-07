/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * LlaDevice.cpp
 * Implementation of the LLA Client Device class
 * Copyright (C) 2005-2006 Simon Newton
 */

#include "LlaDevice.h"

namespace lla {

using std::string;
using std::vector;


int LlaDevice::AddPort(const LlaPort &port) {
 m_ports.push_back(port);
 return 0;
}


int LlaDevice::ClearPorts() {
 m_ports.clear();
 return 0;
}

} // lla
