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
 * UsbDmxPort.h
 * The UsbDmx plugin for ola
 * Copyright (C) 2010 Simon Newton
 */

#ifndef PLUGINS_USBDMX_USBDMXPORT_H_
#define PLUGINS_USBDMX_USBDMXPORT_H_

#include <string>
#include "ola/DmxBuffer.h"
#include "olad/Port.h"
#include "plugins/usbdmx/UsbDmxDevice.h"

namespace ola {
namespace plugin {
namespace usbdmx {

class UsbDmxOutputPort: public BasicOutputPort {
  public:
    UsbDmxOutputPort(UsbDmxDevice *parent, unsigned int id)
        : BasicOutputPort(parent, id),
          m_device(parent) {}

    bool WriteDMX(const DmxBuffer &buffer, uint8_t priority) {
      return m_device->SendDMX(buffer);
    }

    string Description() const { return ""; }

  private:
    UsbDmxDevice *m_device;
};
}  // usbdmx
}  // plugin
}  // ola
#endif  // PLUGINS_USBDMX_USBDMXPORT_H_
