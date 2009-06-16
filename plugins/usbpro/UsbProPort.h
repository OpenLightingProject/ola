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
 * Copyright (C) 2006-2009 Simon Newton
 */

#ifndef USBPROPORT_H
#define USBPROPORT_H

#include <lla/DmxBuffer.h>
#include <llad/Port.h>
#include "UsbProDevice.h"

namespace lla {
namespace usbpro {

class UsbProPort: public lla::Port<UsbProDevice> {
  public:
    UsbProPort(UsbProDevice *parent, unsigned int id, const string &path):
      lla::Port<UsbProDevice>(parent, id),
      m_usb_device(parent),
      m_path(path) {};

    bool WriteDMX(const DmxBuffer &buffer);
    const DmxBuffer &ReadDMX() const;
    bool SetUniverse(Universe *uni);
    bool CanRead() const;
    bool CanWrite() const;
    string Description() const { return m_path; }

  private:
    UsbProDevice *m_usb_device;
    DmxBuffer m_empty_buffer;
    string m_path;
};

} // usbpro
} //lla

#endif
