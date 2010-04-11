/*
 *  This dmxgram is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This dmxgram is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this dmxgram; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * VellemanDevice.cpp
 * The Velleman usb driver
 * Copyright (C) 2010 Simon Newton
 */

#include <string.h>
#include <sys/time.h>
#include <string>

#include "ola/Logging.h"
#include "plugins/usbdmx/VellemanDevice.h"
#include "plugins/usbdmx/VellemanOutputPort.h"

namespace ola {
namespace plugin {
namespace usbdmx {


/*
 * Start this device.
 */
bool VellemanDevice::StartHook() {
  VellemanOutputPort *output_port = new VellemanOutputPort(this,
                                                           0,
                                                           m_usb_device);
  if (!output_port->Start()) {
    delete output_port;
    return false;
  }
  AddPort(output_port);
  return true;
}
}  // usbdmx
}  // plugin
}  // ola
