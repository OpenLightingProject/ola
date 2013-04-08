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
 * UltraDMXProWidget.h
 * The DMXKing Ultra DMX Pro Widget.
 * Copyright (C) 2011 Simon Newton
 */

#include "ola/BaseTypes.h"
#include "plugins/usbpro/UltraDMXProWidget.h"

namespace ola {
namespace plugin {
namespace usbpro {

/**
 * UltraDMXProWidget Constructor
 */
UltraDMXProWidget::UltraDMXProWidget(
  ola::io::ConnectedDescriptor *descriptor)
    : GenericUsbProWidget(descriptor) {
}


bool UltraDMXProWidget::SendDMX(const DmxBuffer &buffer) {
  return SendDMXWithLabel(DMX_PRIMARY_PORT, buffer);
}


bool UltraDMXProWidget::SendSecondaryDMX(const DmxBuffer &buffer) {
  return SendDMXWithLabel(DMX_SECONDARY_PORT, buffer);
}


bool UltraDMXProWidget::SendDMXWithLabel(uint8_t label,
                                         const DmxBuffer &data) {
  struct {
    uint8_t start_code;
    uint8_t dmx[DMX_UNIVERSE_SIZE];
  } widget_dmx;

  widget_dmx.start_code = DMX512_START_CODE;
  unsigned int length = DMX_UNIVERSE_SIZE;
  data.Get(widget_dmx.dmx, &length);
  return SendMessage(label,
                     reinterpret_cast<uint8_t*>(&widget_dmx),
                     length + 1);
}
}  // usbpro
}  // plugin
}  // ola
