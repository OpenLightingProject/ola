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
 * SpiDmxParser.cpp
 * This parses a SPI buffer into a DmxBuffer and notifies a callback when a
 * packet is received completely.
 * Copyright (C) 2017 Florian Edelmann
 */

#include <stdio.h>

#include "ola/Callback.h"
#include "ola/DmxBuffer.h"
#include "ola/Logging.h"
#include "plugins/spidmx/SpiDmxParser.h"

namespace ola {
namespace plugin {
namespace spidmx {

/**
 * This class implements the DMX protocol on a very low level, so be sure to
 * fully understand the protocol before you tackle this code ;)
 *
 * Possible DMX frequencies are 245 - 255kbit/s.
 * With a sampling rate of 2MHz, this results in one DMX bit being
 * mapped to 8.163265306122449 - 7.843137254901961 SPI bits.
 * So if we calculate with 7.5 - 8.5 bit length, we should be fine.
 *
 * Abbreviations used in this class:
 *  - MAB: Mark after break
 *  - MBS: Mark between slots
 *  - MBB: Mark before break
 */


/**
 * Loop through the given SPI raw bytes and call in every iteration the
 * respective function that handles the byte in the current state.
 *
 * @param *buffer - The buffer with SPI bytes to read from
 * @param buffersize - Size of the buffer
 */
void SpiDmxParser::ParseDmx(uint8_t *buffer, uint64_t buffersize) {
  chunk = buffer;

  chunk_bitcount = 0;

  ChangeState(WAIT_FOR_BREAK);

  while (chunk_bitcount < buffersize) {
    switch (state) {
      case WAIT_FOR_BREAK:
        // printf("%6ld  0x%02x  wait for break (%d)\n", chunk_bitcount,
        //        chunk[chunk_bitcount], state);
        WaitForBreak();
        break;

      case IN_BREAK:
        // printf("%6ld  0x%02x  in break (%d)\n", chunk_bitcount,
        //        chunk[chunk_bitcount], state);
        InBreak();
        break;

      case WAIT_FOR_MAB:
        // printf("%6ld  0x%02x  wait for MAB (%d)\n", chunk_bitcount,
        //        chunk[chunk_bitcount], state);
        WaitForMab();
        break;

      case IN_MAB:
        // printf("%6ld  0x%02x  in MAB (%d)\n", chunk_bitcount,
        //        chunk[chunk_bitcount], state);
        InMab();
        break;

      case IN_STARTCODE:
        // printf("%6ld  0x%02x  in startcode (%d)\n", chunk_bitcount,
        //        chunk[chunk_bitcount], state);
        InStartcode();
        break;

      case IN_STARTCODE_STOPBITS:
        // printf("%6ld  0x%02x  in startcode stopbits (%d)\n", chunk_bitcount,
        //        chunk[chunk_bitcount], state);
        InStartcodeStopbits();
        break;

      case IN_DATA_STARTBIT:
        // printf("%6ld  0x%02x  in data startbit (%d)\n", chunk_bitcount,
        //        chunk[chunk_bitcount], state);
        InDataStartbit();
        break;

      case IN_DATA_BITS:
        // printf("%6ld  0x%02x  in data bit %ld (%d)\n", chunk_bitcount,
        //        chunk[chunk_bitcount], state_bitcount, state);
        if (state_bitcount < 7) {
          InDataBits();
        } else {
          InLastDataBit();
        }
        break;

      case IN_DATA_STOPBITS:
        // printf("%6ld  0x%02x  in data stopbits (%d), state_bitcount = %ld\n",
        //        chunk_bitcount, chunk[chunk_bitcount], state, state_bitcount);
        InDataStopbits();
        break;

      default:
        // printf("%6ld  0x%02x  default (%d)\n", chunk_bitcount,
        //        chunk[chunk_bitcount], state);
        chunk_bitcount++;
    }
  }

  if (state >= IN_DATA_STARTBIT) {
    PacketComplete();
  }
}

/**
 * Changes the current state and resets other depending variables.
 */
void SpiDmxParser::ChangeState(SpiDmxParser::dmx_state_t new_state) {
  OLA_DEBUG << "iteration: " << chunk_bitcount
            << ", change state to " << state
            << ", data=" << chunk[chunk_bitcount]
            << ", state_bitcount=" << state_bitcount;

  state = new_state;
  state_bitcount = 0;

  if (state == WAIT_FOR_MAB) {
    channel_count = -1;
  }
}

/**
 * Helper function that returns the number of zeros if a falling edge is
 * detected in the given byte or -1 if the byte is no falling edge.
 *
 * Note: A falling edge can be between two bytes.
 * Note 2: If -1 is returned, this byte can either contain random spikes or
 *         be all ones.
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

/**
 * Helper function that returns the number of ones if a rising edge is
 * detected in the given byte or -1 if the byte is no rising edge.
 *
 * Note: A rising edge can be between two bytes.
 * Note 2: If -1 is returned, this byte can either contain random spikes or
 *         be all zeros.
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

/**
 * Calls the callback to inform a registered InputPort about new data.
 * Warning: This does not reset the current state!
 */
void SpiDmxParser::PacketComplete() {
  channel_count++;

  OLA_DEBUG << "DMX packet complete (" << channel_count << " channels).";

  if (m_callback) {
    m_callback->Run();
  }
}

/**
 * Stay in this state until we find a falling edge, then change to IN_BREAK.
 */
void SpiDmxParser::WaitForBreak() {
  int8_t zeros = DetectFallingEdge(chunk[chunk_bitcount]);
  if (zeros > 0) {
    ChangeState(IN_BREAK);
    state_bitcount = zeros;
  }
  chunk_bitcount++;
}

/**
 * We have to find at least 88µs low = 165 SPI bits to be sure we have a break.
 * If so, change to WAIT_FOR_MAB, else stay here.
 */
void SpiDmxParser::InBreak() {
  if (chunk[chunk_bitcount] == 0) {
    state_bitcount += 8;

    // (88µs break / 4µs per DMX bit) * 7.5 SPI bits = 165
    if (state_bitcount > 165) {
      ChangeState(WAIT_FOR_MAB);
    }
  } else {
    ChangeState(WAIT_FOR_BREAK);
  }
  chunk_bitcount++;
}

/**
 * We are still in a break, so we have to either find a rising edge and change
 * to IN_MAB or stay here.
 */
void SpiDmxParser::WaitForMab() {
  uint8_t byte = chunk[chunk_bitcount];
  if (byte != 0) {
    int8_t ones = DetectRisingEdge(byte);
    if (ones > 0) {
      ChangeState(IN_MAB);
      state_bitcount = ones;
    } else {
      ChangeState(WAIT_FOR_BREAK);
    }
  }
  chunk_bitcount++;
}

/**
 * A MAB must be at least 8µs = 15 SPI bits, so we could alread find it in the
 * first handled byte. Then change to IN_STARTCODE, otherwise stay here. If we
 * get unexpected spikes, go back to WAIT_FOR_BREAK.
 */
void SpiDmxParser::InMab() {
  uint8_t byte = chunk[chunk_bitcount];
  if (byte == 0xff) {
    state_bitcount += 8;
  } else {
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

/**
 * The start code for DMX consists of 1 low start bit + 8 low "data" bits.
 * So we have to get between 67 and 77 low SPI bits to be sure that this is the
 * start of a valid DMX packet. If we find the right amount, change to
 * IN_STARTCODE_STOPBITS, else go back to WAIT_FOR_BREAK.
 */
void SpiDmxParser::InStartcode() {
  uint8_t byte = chunk[chunk_bitcount];
  if (byte == 0x00) {
    state_bitcount += 8;
  } else {
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

/**
 * We have to find 2 high stop bits + optionally an arbitrarily long MBS. After
 * that, we are in the first slot's start bit (IN_DATA_STARTBIT). If the stop
 * bits are too short, go back to WAIT_FOR_BREAK.
 */
void SpiDmxParser::InStartcodeStopbits() {
  uint8_t byte = chunk[chunk_bitcount];
  if (byte == 0xff) {
    state_bitcount += 8;
  } else {
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

/**
 * Now, we're close to the actual data. We always want to sample in the middle
 * of a SPI byte, so we have to calculate the sampling position. Additionally,
 * we could have to look at the last byte again. See the following table:
 * 
 * x denotes the first DMX data bit, SP the sampling position
 *
 * last & current byte               new current byte
 * -------------------               ----------------
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
void SpiDmxParser::InDataStartbit() {
  uint8_t byte = chunk[chunk_bitcount];

  if (state_bitcount >= 4) {
    // look at the last byte again and don't increase chunk_bitcount
    byte = chunk[chunk_bitcount - 1];
    sampling_position = state_bitcount - 4;
  } else {
    // next byte will be handled in next step as usual
    chunk_bitcount++;
    sampling_position = state_bitcount + 8 - 4;
  }

  // start bit must be zero
  if ((byte & (1 << sampling_position))) {
    ChangeState(WAIT_FOR_BREAK);
  } else {
    current_dmx_value = 0x00;
    ChangeState(IN_DATA_BITS);
  }
}

/**
 * Handle bits 1 - 7 of a slot.
 *
 * Sample the current DMX bit at the calculated position and update the current
 * DMX value accordingly.
 * Note: state_bitcount is abused in this state because it doesn't count SPI
 * bits but DMX bits here.
 */
void SpiDmxParser::InDataBits() {
  uint8_t byte = chunk[chunk_bitcount];
  uint8_t read_bit = ((byte & (1 << sampling_position)) ? 1 : 0);
  current_dmx_value |= read_bit << state_bitcount;

  state_bitcount++;
  chunk_bitcount++;
}

/**
 * In the slot's last (eight) bit, we have to synchronize again to be able to
 * find the stop bits. Change to IN_DATA_STOPBITS in every case.
 */
void SpiDmxParser::InLastDataBit() {
  uint8_t byte = chunk[chunk_bitcount];
  uint8_t read_bit = ((byte & (1 << sampling_position)) ? 1 : 0);
  current_dmx_value |= read_bit << 7;

  ChangeState(IN_DATA_STOPBITS);
  // assume that bit after sample position belongs to stop bits
  if (sampling_position >= 4) {
    state_bitcount = sampling_position;
  } else {
    state_bitcount = sampling_position + 8;
    chunk_bitcount++;  // assume next byte is 0xff
  }
  chunk_bitcount++;
}

/**
 * Here we have two different options:
 *
 * First, 2 stop bits + arbitrarily long MBS (or MBB, we can't know that yet).
 * Stay in this state until we find a falling edge (the following start bit is
 * exceptionally allowed to be only 7 SPI bits). Then save the current channel
 * and change to IN_DATA_STARTBIT (or to IN_BREAK if it was the last channel).
 *
 * Second, we could find that the expected stop bits are zero, in which case we
 * are actually in a break instead of a data slot. This means we have
 * successfully received the previous packet and can change directly to
 * IN_BREAK. However, 1 start bit + 8 data bits + the current stop bit = 10*8
 * SPI bits can already be counted to the break.
 */
void SpiDmxParser::InDataStopbits() {
  uint8_t byte = chunk[chunk_bitcount];
  if (byte == 0xff) {
    state_bitcount += 8;
  } else if (byte == 0x00 && state_bitcount <= 11
      && current_dmx_value == 0x00) {  // we are actually in a break
    // all later channels are definitely zero
    m_dmx_buffer->SetRangeToValue(channel_count + 1, 0x00, 511 - channel_count);
    channel_count = 511;
    PacketComplete();

    ChangeState(IN_BREAK);
    state_bitcount = 10 * 8;
  } else {
    int8_t zeros = DetectFallingEdge(byte);
    int8_t ones = 8 - zeros;

    // (8µs stop bits / 4µs per DMX bit) * 7.5 SPI bits = 15
    if (state_bitcount + ones <= 15) {
      // stop bits were too short
      PacketComplete();

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
        PacketComplete();

        ChangeState(WAIT_FOR_BREAK);
        chunk_bitcount++;
        return;
      }
    }

    channel_count++;  // mark channel receive as complete
    m_dmx_buffer->SetChannel(channel_count, current_dmx_value);

    if (channel_count == 511) {
      // last channel filled
      PacketComplete();
      ChangeState(IN_BREAK);
    } else {
      ChangeState(IN_DATA_STARTBIT);
    }
    state_bitcount = zeros;
  }

  chunk_bitcount++;
}

}  // namespace spidmx
}  // namespace plugin
}  // namespace ola
