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
 * UsbDevice.h
 * Interface for the usb devices
 * Copyright (C) 2010 Simon Newton
 */

#ifndef PLUGINS_USBPRO_USBDEVICE_H_
#define PLUGINS_USBPRO_USBDEVICE_H_

#include <string>
#include "ola/Closure.h"
#include "olad/Device.h"
#include "plugins/usbpro/UsbWidget.h"

namespace ola {
namespace plugin {
namespace usbpro {

/*
 * A USB device
 */
class UsbDevice: public ola::Device {
  public:
    UsbDevice(ola::AbstractPlugin *owner,
              const string &name,
              UsbWidget *widget):
      Device(owner, name),
      m_widget(widget) {}

    virtual ~UsbDevice() {
      delete m_widget;
    }

    void SetOnRemove(ola::SingleUseClosure *on_close) {
      m_widget->SetOnRemove(on_close);
    }

  protected:
    UsbWidget *m_widget;
};
}  // usbpro
}  // plugin
}  // ola
#endif  // PLUGINS_USBPRO_USBDEVICE_H_
