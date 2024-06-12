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
 * UsbProExtWidget.cpp
 * The DMXKing Ultra DMX Pro Widget.
 * Copyright (C) 2011 Simon Newton
 */

#include "ola/Constants.h"
#include "plugins/usbpro/UsbProExtWidget.h"

namespace ola {
namespace plugin {
namespace usbpro {

uint8_t UsbProExtWidget::PortLabel (uint8_t port_id) {
	return ((DMX_PORT_LABEL_BASE + port_id) > 0xFF ) ?
			0xFF :
			(DMX_PORT_LABEL_BASE + port_id);
}


bool UsbProExtWidget::SendDMX(uint8_t port_id,
								const DmxBuffer &data) {
  struct {
    uint8_t start_code;
    uint8_t dmx[DMX_UNIVERSE_SIZE];
  } widget_dmx;

  widget_dmx.start_code = DMX512_START_CODE;
  unsigned int length = DMX_UNIVERSE_SIZE;
  data.Get(widget_dmx.dmx, &length);
  return SendMessage(PortLabel(port_id),
                     reinterpret_cast<uint8_t*>(&widget_dmx),
                     length + 1);
}
}  // namespace usbpro
}  // namespace plugin
}  // namespace ola
