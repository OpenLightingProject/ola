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

#include <stdarg.h>
#include <stdio.h>

#include "ola/Callback.h"
#include "ola/DmxBuffer.h"
#include "ola/Logging.h"
#include "plugins/spidmx/SpiDmxParser.h"

namespace ola {
namespace plugin {
namespace spidmx {

/*
 * possible DMX frequencies are 245 - 255kbit/s
 *
 * with a sampling rate of 2MHz, this results in one DMX bit being
 * mapped to 8.163265306122449 - 7.843137254901961 SPI bits
 *
 * so if we calculate with 7.5 - 8.5 bit length, we should be fine
 */

/*
 * uint8_t *buffer a pointer to an 512-byte array
 */
SpiDmxParser::SpiDmxParser(DmxBuffer *buffer, Callback0<void> *callback) {
  m_dmx_buffer = buffer;
  m_callback.reset(callback);
}

void SpiDmxParser::Debug(const char *message, ...) {
  // Compose Arguments
  va_list arg;
  char buffer[2048];

  va_start(arg, message);
  vsnprintf(buffer, sizeof(buffer), message, arg);
  va_end(arg);

  OLA_DEBUG << buffer;
}

/*
 *
 */
void SpiDmxParser::ChangeState(SpiDmxParser::dmx_state_t new_state) {
  /*Debug(
    "iteration %ld: change state to %d, data=0x%02x, state_bitcount=%ld\n",
    chunk_bitcount,
    state,
    chunk[chunk_bitcount],
    state_bitcount
  );*/

  state = new_state;
  state_bitcount = 0;

  if (state == WAIT_FOR_MAB) {
    channel_count = -1;
  }
}

/*
 *
 */
int8_t SpiDmxParser::DetectFallingEdge(uint8_t byte) {
  switch (byte) {
    case 0b11111110:
      return 1;

    case 0b11111100:
      return 2;

    case 0b11111000:
      return 3;

    case 0b11110000:
      return 4;

    case 0b11100000:
      return 5;

    case 0b11000000:
      return 6;

    case 0b10000000:
      return 7;

    case 0b00000000:
      return 8;

    default:
      return -1;
  }
}

/*
 *
 */
int8_t SpiDmxParser::DetectRisingEdge(uint8_t byte) {
  switch (byte) {
    case 0b00000001:
      return 1;

    case 0b00000011:
      return 2;

    case 0b00000111:
      return 3;

    case 0b00001111:
      return 4;

    case 0b00011111:
      return 5;

    case 0b00111111:
      return 6;

    case 0b01111111:
      return 7;

    case 0b11111111:
      return 8;

    default:
      return -1;
  }
}

/*
 *
 */
void SpiDmxParser::ReceiveComplete() {
  int j;
  channel_count++;

  Debug("DMX packet complete (%d channels).", channel_count);
  for (j=0; j < channel_count; j++) {
    Debug("DMX channel %3d: %3d", j+1, m_dmx_buffer->Get(j));
  }

  if (m_callback.get()) {
    m_callback->Run();
  }
}

/*
 *
 */
void SpiDmxParser::WaitForBreak() {
  int8_t zeros = DetectFallingEdge(chunk[chunk_bitcount]);
  if (zeros > 0) {
    ChangeState(IN_BREAK);
    state_bitcount = zeros;
  }
  chunk_bitcount++;
}

/*
 *
 */
void SpiDmxParser::InBreak() {
  if (chunk[chunk_bitcount] == 0) {
    state_bitcount += 8;

    // (88µs break / 4µs per DMX bit) * 7.5 SPI bits = 165
    if (state_bitcount > 165) {
      ChangeState(WAIT_FOR_MAB);
    }
  }
  else {
    ChangeState(WAIT_FOR_BREAK);
  }
  chunk_bitcount++;
}

/*
 *
 */
void SpiDmxParser::WaitForMab() {
  uint8_t byte = chunk[chunk_bitcount];
  if (byte != 0) {
    int8_t ones = DetectRisingEdge(byte);
    if (ones > 0) {
      ChangeState(IN_MAB);
      state_bitcount = ones;
    }
    else {
      ChangeState(WAIT_FOR_BREAK);
    }
  }
  chunk_bitcount++;
}

/*
 *
 */
void SpiDmxParser::InMab() {
  uint8_t byte = chunk[chunk_bitcount];
  if (byte == 0xff) {
    state_bitcount += 8;
  }
  else {
    int8_t zeros = DetectFallingEdge(byte);
    int8_t ones = 8 - zeros;

    // (8µs MAB / 4µs per DMX bit) * 7.5 SPI bits = 15
    if (zeros < 0 || state_bitcount + ones <= 15) {
      ChangeState(WAIT_FOR_BREAK);
      chunk_bitcount++;
      return;
    }
    
    ChangeState(IN_STARTCODE);
    state_bitcount = zeros;
  }

  chunk_bitcount++;
}

/*
 *
 */
void SpiDmxParser::InStartcode() {
  uint8_t byte = chunk[chunk_bitcount];
  if (byte == 0x00) {
    state_bitcount += 8;
  }
  else {
    int8_t ones = DetectRisingEdge(byte);
    int8_t zeros = 8 - ones;

    state_bitcount += zeros;

    // (1 start bit + 8 NULL code bits) * 7.5 SPI bits >= 67
    // (1 start bit + 8 NULL code bits) * 8.5 SPI bits <= 77
    if (zeros < 0 || state_bitcount <= 67 || state_bitcount >= 77) {
      ChangeState(WAIT_FOR_BREAK);
      chunk_bitcount++;
      return;
    }
    
    ChangeState(IN_STARTCODE_STOPBITS);
    state_bitcount = ones;
  }

  chunk_bitcount++;
}

/*
 *
 */
void SpiDmxParser::InStartcodeStopbits() {
  uint8_t byte = chunk[chunk_bitcount];
  if (byte == 0xff) {
    state_bitcount += 8;
  }
  else {
    int8_t zeros = DetectFallingEdge(byte);
    int8_t ones = 8 - zeros;

    // (8µs stop bits / 4µs per DMX bit) * 7.5 SPI bits = 15
    if (zeros < 0 || state_bitcount + ones <= 15) {
      ChangeState(WAIT_FOR_BREAK);
      chunk_bitcount++;
      return;
    }
    
    ChangeState(IN_DATA_STARTBIT);
    state_bitcount = zeros;
  }

  chunk_bitcount++;
}

/*
 *
 */
void SpiDmxParser::InDataStartbit() {
  uint8_t byte = chunk[chunk_bitcount];
  /**
   * We always want to sample in the middle of a SPI byte.
   * x denotes the first DMX data bit, SP the sampling position
   *
   * last & current byte               new current byte
   *
   * 00000000 xxxxxxxx   -> backtrack:   00000000
   *                                        ^      SP = 4
   * 10000000 0xxxxxxx   -> backtrack:   10000000
   *                                         ^     SP = 3
   * 11000000 00xxxxxx   -> backtrack:   11000000
   *                                          ^    SP = 2
   * 11100000 000xxxxx   -> backtrack:   11100000
   *                                           ^   SP = 1
   * 11110000 0000xxxx   -> backtrack:   11110000
   *                                            ^  SP = 0
   * 11111000 00000xxx   -> nop:         00000xxx
   *                                     ^         SP = 7
   * 11111100 000000xx   -> nop:         000000xx
   *                                      ^        SP = 6
   * 11111110 0000000x   -> nop:         0000000x
   *                                       ^       SP = 5
   */
  if (state_bitcount >= 4) {
    // look at the last byte again and don't increase chunk_bitcount
    byte = chunk[chunk_bitcount - 1];
    sampling_position = state_bitcount - 4;
  }
  else {
    // next byte will be handled in next step as usual
    chunk_bitcount++;
    sampling_position = state_bitcount + 8 - 4;
  }

  // start bit must be zero
  if ((byte & (1 << sampling_position))) {
    ChangeState(WAIT_FOR_BREAK);
  }
  else {
    current_dmx_value = 0x00;
    ChangeState(IN_DATA_BITS);
  }
}

/*
 *
 */
void SpiDmxParser::InDataBits() {
  uint8_t byte = chunk[chunk_bitcount];
  uint8_t read_bit = ((byte & (1 << sampling_position)) ? 1 : 0);
  current_dmx_value |= read_bit << state_bitcount;

  state_bitcount++;
  chunk_bitcount++;
}

/*
 *
 */
void SpiDmxParser::InLastDataBit() {
  uint8_t byte = chunk[chunk_bitcount];
  uint8_t read_bit = ((byte & (1 << sampling_position)) ? 1 : 0);
  current_dmx_value |= read_bit << 7;

  ChangeState(IN_DATA_STOPBITS);
  // assume that bit after sample position belongs to stop bits
  //if (read_bit == 1) {
  if (sampling_position >= 4) {
    state_bitcount = sampling_position;
  }
  else {
    state_bitcount = sampling_position + 8;
    chunk_bitcount++; // assume next byte is 0xff
  }
  chunk_bitcount++;
}

/*
 *
 */
void SpiDmxParser::InDataStopbits() {
  uint8_t byte = chunk[chunk_bitcount];
  if (byte == 0xff) {
    state_bitcount += 8;
  }
  else if (byte == 0x00 && state_bitcount <= 11 && current_dmx_value == 0x00) {
    // we are actually in a break

    // all later channels are definitely zero
    m_dmx_buffer->SetRangeToValue(channel_count + 1, 0x00, 511 - channel_count);
    channel_count = 511;
    ReceiveComplete();

    ChangeState(IN_BREAK);
    state_bitcount = 10 * 8;
  }
  else {
    int8_t zeros = DetectFallingEdge(byte);
    int8_t ones = 8 - zeros;

    // (8µs stop bits / 4µs per DMX bit) * 7.5 SPI bits = 15
    if (state_bitcount + ones <= 15) {
      // stop bits were too short
      ReceiveComplete();

      ChangeState(WAIT_FOR_BREAK);
      chunk_bitcount++;
      return;
    }

    if (zeros < 0) {
      // detected not a falling edge -> try other direction

      ones = DetectRisingEdge(byte);
      zeros = 8 - ones;

      // do not accept anything but a 7-bit start bit (7 zeros)
      if (ones != 1) {
        ReceiveComplete();

        ChangeState(WAIT_FOR_BREAK);
        chunk_bitcount++;
        return;
      }
    }

    channel_count++; // mark channel receive as complete
    m_dmx_buffer->SetChannel(channel_count, current_dmx_value);
    Debug("                                 DMX channel %d: %d", channel_count+1, current_dmx_value);
    
    if (channel_count == 511) {
      // last channel filled
      ReceiveComplete();
      ChangeState(IN_BREAK);
    }
    else {
      ChangeState(IN_DATA_STARTBIT);
    }
    state_bitcount = zeros;
  }

  chunk_bitcount++;
}

/*
 * uint8_t *buffer - The buffer with SPI bytes to read from
 * uint64_t buffersize - Size of the buffer
 */
void SpiDmxParser::ParseDmx(uint8_t *buffer, uint64_t buffersize) {
  chunk = buffer;

  chunk_bitcount = 0;

  ChangeState(WAIT_FOR_BREAK);

  while (chunk_bitcount < buffersize) {
    switch (state) {
      case WAIT_FOR_BREAK:
        Debug("%6ld  0x%02x  wait for break (%d)", chunk_bitcount, chunk[chunk_bitcount], state);
        WaitForBreak();
        break;

      case IN_BREAK:
        Debug("%6ld  0x%02x  in break (%d)", chunk_bitcount, chunk[chunk_bitcount], state);
        InBreak();
        break;

      case WAIT_FOR_MAB:
        Debug("%6ld  0x%02x  wait for MAB (%d)", chunk_bitcount, chunk[chunk_bitcount], state);
        WaitForMab();
        break;

      case IN_MAB:
        Debug("%6ld  0x%02x  in MAB (%d)", chunk_bitcount, chunk[chunk_bitcount], state);
        InMab();
        break;

      case IN_STARTCODE:
        Debug("%6ld  0x%02x  in startcode (%d)", chunk_bitcount, chunk[chunk_bitcount], state);
        InStartcode();
        break;

      case IN_STARTCODE_STOPBITS:
        Debug("%6ld  0x%02x  in startcode stopbits (%d)", chunk_bitcount, chunk[chunk_bitcount], state);
        InStartcodeStopbits();
        break;

      case IN_DATA_STARTBIT:
        Debug("%6ld  0x%02x  in data startbit (%d)", chunk_bitcount, chunk[chunk_bitcount], state);
        InDataStartbit();
        break;

      case IN_DATA_BITS:
        Debug("%6ld  0x%02x  in data bit %ld (%d)", chunk_bitcount, chunk[chunk_bitcount], state_bitcount, state);
        if (state_bitcount < 7) {
          InDataBits();
        }
        else {
          InLastDataBit();
        }
        break;

      case IN_DATA_STOPBITS:
        Debug("%6ld  0x%02x  in data stopbits (%d), state_bitcount = %ld", chunk_bitcount, chunk[chunk_bitcount], state, state_bitcount);
        InDataStopbits();
        break;

      default:
        Debug("%6ld  0x%02x  default (%d)", chunk_bitcount, chunk[chunk_bitcount], state);
        chunk_bitcount++;
    }
  }

  if (state >= IN_DATA_STARTBIT) {
    ReceiveComplete();
  }
}

}  // namespace spidmx
}  // namespace plugin
}  // namespace ola