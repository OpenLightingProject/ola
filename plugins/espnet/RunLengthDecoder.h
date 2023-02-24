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
 * RunLengthDecoder.h
 * Header file for the RunLengthDecoder class
 * Copyright (C) 2005 Simon Newton
 */

#ifndef PLUGINS_ESPNET_RUNLENGTHDECODER_H_
#define PLUGINS_ESPNET_RUNLENGTHDECODER_H_

#include <ola/DmxBuffer.h>

namespace ola {
namespace plugin {
namespace espnet {

class RunLengthDecoder {
 public:
  RunLengthDecoder() {}
  ~RunLengthDecoder() {}

  void Decode(DmxBuffer *dst,
              const uint8_t *data,
              unsigned int length);
 private:
  static const uint8_t ESCAPE_VALUE = 0xFD;
  static const uint8_t REPEAT_VALUE = 0xFE;
};
}  // namespace espnet
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_ESPNET_RUNLENGTHDECODER_H_
