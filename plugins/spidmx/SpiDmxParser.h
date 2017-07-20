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
 * SPIDMXParser.h
 * The SPI DMX plugin samples the Serial Peripheral Interface for DMX data.
 * Copyright (C) 2017 Florian Edelmann
 */

#ifndef PLUGINS_SPIDMX_SPIDMXPARSER_H_
#define PLUGINS_SPIDMX_SPIDMXPARSER_H_

#include <memory>
#include "ola/Callback.h"
#include "ola/DmxBuffer.h"
#include "ola/base/Macro.h"

namespace ola {
namespace plugin {
namespace spidmx {

class SpiDmxParser {
 public:
  SpiDmxParser(DmxBuffer *buffer, Callback0<void> *callback);
  void ParseDmx(uint8_t *buffer, uint64_t chunksize);

 private:
  typedef enum {
    WAIT_FOR_BREAK,
    IN_BREAK,
    WAIT_FOR_MAB,
    IN_MAB,
    IN_STARTCODE,
    IN_STARTCODE_STOPBITS,
    IN_DATA_STARTBIT,
    IN_DATA_BITS,
    IN_DATA_STOPBITS
  } dmx_state_t;

  int8_t DetectFallingEdge(uint8_t byte);
  int8_t DetectRisingEdge(uint8_t byte);

  void ChangeState(SpiDmxParser::dmx_state_t new_state);
  void WaitForBreak();
  void InBreak();
  void WaitForMab();
  void InMab();
  void InStartcode();
  void InStartcodeStopbits();
  void InDataStartbit();
  void InDataBits();
  void InLastDataBit();
  void InDataStopbits();
  void ReceiveComplete();

  void Debug(const char *message, ...);

  SpiDmxParser::dmx_state_t state;
  uint8_t *chunk;
  uint64_t chunk_bitcount;
  uint64_t state_bitcount;
  DmxBuffer *m_dmx_buffer;
  uint8_t current_dmx_value;
  int16_t channel_count;
  uint8_t sampling_position;
  std::auto_ptr<Callback0<void> > m_callback;

  DISALLOW_COPY_AND_ASSIGN(SpiDmxParser);
};

}  // namespace spidmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_SPIDMX_SPIDMXPARSER_H_