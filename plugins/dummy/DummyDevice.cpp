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
 * DummyDevice.cpp
 * A dummy device
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <vector>

#include "plugins/dummy/DummyDevice.h"
#include "plugins/dummy/DummyPort.h"

namespace ola {
namespace plugin {
namespace dummy {


/*
 * Start this device
 */
bool DummyDevice::StartHook() {
  DummyPort *port = new DummyPort(this, 0);

  if (!AddPort(port)) {
    delete port;
    return false;
  }
  return true;
}
}  // dummy
}  // plugin
}  // ola
