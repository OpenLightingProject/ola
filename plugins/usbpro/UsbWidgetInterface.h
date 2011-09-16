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
 * UsbWidgetInterface.h
 * Read and Write to a USB Widget.
 * Copyright (C) 2011 Simon Newton
 */

#ifndef PLUGINS_USBPRO_USBWIDGETINTERFACE_H_
#define PLUGINS_USBPRO_USBWIDGETINTERFACE_H_

#include <stdint.h>
#include <ola/Callback.h>
#include <string>
#include "ola/network/Socket.h"

namespace ola {
namespace plugin {
namespace usbpro {


/*
 * The UsbWidgetInterface, this is an interface so we can mock it out for
 * testing.
 */
class UsbWidgetInterface {
  public:
    UsbWidgetInterface() {}
    virtual ~UsbWidgetInterface() {}

    virtual ola::network::ConnectedDescriptor *GetDescriptor() const = 0;

    virtual void SetOnRemove(ola::SingleUseCallback0<void> *on_close) = 0;
    virtual void CloseDescriptor() = 0;
};
}  // usbpro
}  // plugin
}  // ola
#endif  // PLUGINS_USBPRO_USBWIDGETINTERFACE_H_
