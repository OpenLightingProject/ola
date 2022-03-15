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
#include "ola/Logging.h"
#include "ola/util/Utils.h"
#include "plugins/usbpro/OpenDeckWidget.h"

namespace ola {
namespace plugin {
namespace usbpro {

using ola::utils::SplitUInt16;

/**
 * OpenDeckWidget Constructor
 */
OpenDeckWidget::OpenDeckWidget(
  ola::io::ConnectedDescriptor *descriptor)
    : GenericUsbProWidget(descriptor) {
}


bool OpenDeckWidget::SendDMX(const DmxBuffer &data) {
  if (data != internal_buffer) {
    std::vector<uint8_t> send_buffer;
    uint16_t changed_values = 0;

    for (size_t index = 0; index < data.Size(); index++) {
      if (data.Get(index) != internal_buffer.Get(index)) {
        // In diff mode a packet is composed of 2 bytes of
        // channel to update and 1 byte of actual channel
        // value.
        uint8_t high;
        uint8_t low;

        SplitUInt16((index + 1), &high, &low);

        send_buffer.push_back(low);
        send_buffer.push_back(high);
        send_buffer.push_back(data.Get(index));

        if (++changed_values >= MAX_DIFF_CHANNELS) {
          // Just send the full frame in this case
          struct {
            uint8_t start_code;
            uint8_t dmx[DMX_UNIVERSE_SIZE];
          } full_frame;

          full_frame.start_code = DMX512_START_CODE;
          unsigned int length = DMX_UNIVERSE_SIZE;
          data.Get(full_frame.dmx, &length);
          internal_buffer = data;

          return SendMessage(DMX_LABEL,
                             reinterpret_cast<uint8_t*>(&full_frame),
                             length + sizeof(full_frame.start_code));
        }
      }
    }

    internal_buffer = data;
    return SendMessage(DMX_SLOT_VALUE_DIFF_LABEL,
                       &send_buffer[0],
                       send_buffer.size());
  } else {
    OLA_DEBUG << "Data unchanged - not sending data to device";
    return true;
  }
}
}  // namespace usbpro
}  // namespace plugin
}  // namespace ola
