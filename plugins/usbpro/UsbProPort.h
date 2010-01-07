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
 * The UsbPro plugin for ola
 * Copyright (C) 2006-2009 Simon Newton
 */

#ifndef PLUGINS_USBPRO_USBPROPORT_H_
#define PLUGINS_USBPRO_USBPROPORT_H_

#include <string>
#include "ola/DmxBuffer.h"
#include "olad/Port.h"
#include "plugins/usbpro/UsbProDevice.h"

namespace ola {
namespace plugin {
namespace usbpro {

class UsbProInputPort: public InputPort {
  public:
    UsbProInputPort(UsbProDevice *parent, unsigned int id, const string &path)
        : InputPort(parent, id),
          m_path(path),
          m_device(parent) {}

    const DmxBuffer &ReadDMX() const;
    string Description() const { return m_path; }

  private:
    string m_path;
    UsbProDevice *m_device;
};


class UsbProOutputPort: public OutputPort {
  public:
    UsbProOutputPort(UsbProDevice *parent, unsigned int id, const string &path)
        : OutputPort(parent, id),
          m_path(path),
          m_device(parent) {}

    bool WriteDMX(const DmxBuffer &buffer, uint8_t priority);
    void PostSetUniverse(Universe *new_universe, Universe *old_universe);
    string Description() const { return m_path; }

  private:
    string m_path;
    UsbProDevice *m_device;
};
}  // usbpro
}  // plugin
}  // ola
#endif  // PLUGINS_USBPRO_USBPROPORT_H_
