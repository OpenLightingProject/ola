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
 * ImgStageLineDevice.cpp
 * The img Stage Line DMX-1USB device
 * Copyright (C) 2014 Peter Newman
 */

#include <string.h>
#include <sys/time.h>

#include "ola/Logging.h"
#include "plugins/usbdmx/ImgStageLineDevice.h"
#include "plugins/usbdmx/ImgStageLineOutputPort.h"

namespace ola {
namespace plugin {
namespace usbdmx {

/*
 * Start this device.
 */
bool ImgStageLineDevice::StartHook() {
  ImgStageLineOutputPort *output_port =
      new ImgStageLineOutputPort(this, 0, m_usb_device);
  if (!output_port->Start()) {
    delete output_port;
    return false;
  }
  AddPort(output_port);
  return true;
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
