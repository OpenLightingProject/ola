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
 * OpenDeckWidget.cpp
 * The OpenDeck Widget.
 * Copyright (C) 2022 Igor Petrovic
 */

#include <vector>

#include "ola/Constants.h"
#include "plugins/usbpro/OpenDeckWidget.h"

namespace ola {
namespace plugin {
namespace usbpro {

/**
 * OpenDeckWidget Constructor
 */
OpenDeckWidget::OpenDeckWidget(
  ola::io::ConnectedDescriptor *descriptor)
    : GenericUsbProWidget(descriptor) {
}


bool OpenDeckWidget::SendDMX(const DmxBuffer &data) {
  if (data != internal_buffer) {
    std::vector<uint8_t> send_buffer = {};

    for (size_t index = 0; index < data.Size(); index++) {
      if (data.Get(index) != internal_buffer.Get(index)) {
        send_buffer.push_back((index+1) & 0xFF);
        send_buffer.push_back((index+1) >> 8);
        send_buffer.push_back(data.Get(index));
      }
    }

    internal_buffer = data;

    if (send_buffer.size() >= MAX_DIFF_CHANNELS) {
      // Just send the full frame in this case
      struct {
        uint8_t start_code;
        uint8_t dmx[DMX_UNIVERSE_SIZE];
      } full_frame;

      full_frame.start_code = DMX512_START_CODE;
      unsigned int length = DMX_UNIVERSE_SIZE;
      data.Get(full_frame.dmx, &length);

      return SendMessage(DMX_LABEL,
                         reinterpret_cast<uint8_t*>(&full_frame),
                         length + sizeof(full_frame.start_code));
    } else {
      return SendMessage(DMX_SLOT_VALUE_DIFF_LABEL,
                         &send_buffer[0],
                         send_buffer.size());
    }
  } else {
    // Nothing new to send
    return true;
  }
}
}  // namespace usbpro
}  // namespace plugin
}  // namespace ola
