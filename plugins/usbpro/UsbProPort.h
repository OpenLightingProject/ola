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
 * UsbProPort.h
 * The UsbPro plugin for lla
 * Copyright (C) 2006-2007 Simon Newton
 */

#ifndef USBPROPORT_H
#define USBPROPORT_H

#include <llad/Port.h>
#include "UsbProDevice.h"

namespace lla {
namespace plugin {

class UsbProPort: public lla::Port {
  public:
    UsbProPort(UsbProDevice *parent, int id):
      lla::Port(parent, id),
      m_usb_device(parent) {};

    int WriteDMX(uint8_t *data, unsigned int length);
    int ReadDMX(uint8_t *data, unsigned int length);
    int SetUniverse(Universe *uni);
    bool CanRead() const;
    bool CanWrite() const;

  private:
    UsbProDevice *m_usb_device;
};

} // plugin
} //lla

#endif
