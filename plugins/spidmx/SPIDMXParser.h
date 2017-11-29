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
 * This parses a SPI buffer into a DmxBuffer and notifies a callback when a
 * packet is received completely.
 * Copyright (C) 2017 Florian Edelmann
 */

#ifndef PLUGINS_SPIDMX_SPIDMXPARSER_H_
#define PLUGINS_SPIDMX_SPIDMXPARSER_H_

#include "ola/Callback.h"
#include "ola/DmxBuffer.h"
#include "ola/base/Macro.h"

namespace ola {
namespace plugin {
namespace spidmx {

class SPIDMXParser {
 public:
  SPIDMXParser(DmxBuffer *buffer, Callback0<void> *callback)
    : m_dmx_buffer(buffer),
      m_callback(callback),
      m_state(WAIT_FOR_BREAK),   // reset in ChangeState()
      m_chunk(NULL),             // reset in ParseDmx()
      m_chunk_spi_bytecount(0),  // first reset in ParseDmx()
      m_state_spi_bitcount(0),   // first reset in ChangeState()
      m_current_dmx_value(0),    // first reset in InDataStartbit()
      m_channel_count(0),        // first reset in ChangeState()
      m_sampling_position(0) {   // reset in InDataStartbit()
  }
  void ParseDmx(uint8_t *buffer, uint64_t chunksize);
  void SetCallback(Callback0<void> *callback) {
    m_callback = callback;
  }

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

  // helper functions
  int8_t DetectFallingEdge(uint8_t byte);
  int8_t DetectRisingEdge(uint8_t byte);

  void ChangeState(SPIDMXParser::dmx_state_t new_state);
  void PacketComplete();

  // handle one state each
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

  /** current state */
  SPIDMXParser::dmx_state_t m_state;

  /** the raw SPI data */
  uint8_t *m_chunk;

  /** the current SPI byte */
  uint64_t m_chunk_spi_bytecount;

  /**
   * Count how many SPI bits the current state is already active. Not used
   * in all states.
   */
  uint64_t m_state_spi_bitcount;

  /** a DmxBuffer that is filled and returned when ready */
  DmxBuffer *m_dmx_buffer;

  /** The current DMX channel value (gets assembled from multiple SPI bytes) */
  uint8_t m_current_dmx_value;

  /** The current DMX channel count */
  int16_t m_channel_count;

  /** The currently used bit offset to sample the DMX bits */
  uint8_t m_sampling_position;

  /** The callback to call when a packet end is detected or the chunk ends */
  Callback0<void> *m_callback;

  DISALLOW_COPY_AND_ASSIGN(SPIDMXParser);
};

}  // namespace spidmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_SPIDMX_SPIDMXPARSER_H_
