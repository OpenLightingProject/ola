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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * UsbProExtWidget.h
 * The DMXKing Ultra DMX Pro Widget.
 * This is similar to the Enttec Usb Pro, but it has two output ports.
 * Copyright (C) 2011 Simon Newton
 */

#ifndef PLUGINS_USBPRO_USBPROEXTWIDGET_H_
#define PLUGINS_USBPRO_USBPROEXTWIDGET_H_

#include <deque>
#include <string>
#include "ola/Callback.h"
#include "ola/DmxBuffer.h"
#include "plugins/usbpro/GenericUsbProWidget.h"

namespace ola {
namespace plugin {
namespace usbpro {

/*
 * An Ultra DMX Pro Widget
 */
class UsbProExtWidget: public GenericUsbProWidget {
 public:

	explicit UsbProExtWidget(
	  ola::io::ConnectedDescriptor *descriptor)
	    : GenericUsbProWidget(descriptor) {};

    ~UsbProExtWidget() {}
    void Stop() { GenericStop(); }

    bool SendDMX(uint8_t label, const DmxBuffer &data);

 protected:
    static uint8_t PortLabel (uint8_t port_id);
    static const uint8_t DMX_PORT_LABEL_BASE = 100;
};
}  // namespace usbpro
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBPRO_USBPROEXTWIDGET_H_
