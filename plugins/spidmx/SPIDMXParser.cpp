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
 * SPIDMXParser.cpp
 * This parses a SPI buffer into a DmxBuffer and notifies a callback when a
 * packet is received completely.
 * Copyright (C) 2017 Florian Edelmann
 */

#include <stdio.h>

#include "ola/Callback.h"
#include "ola/DmxBuffer.h"
#include "ola/Logging.h"
#include "plugins/spidmx/SPIDMXParser.h"

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
void SPIDMXParser::ParseDmx(uint8_t *buffer, uint64_t buffersize) {
  m_chunk = buffer;

  m_chunk_spi_bytecount = 0;

  ChangeState(WAIT_FOR_BREAK);

  while (m_chunk_spi_bytecount < buffersize) {
    switch (m_state) {
      case WAIT_FOR_BREAK:
        WaitForBreak();
        break;

      case IN_BREAK:
        InBreak();
        break;

      case WAIT_FOR_MAB:
        WaitForMab();
        break;

      case IN_MAB:
        InMab();
        break;

      case IN_STARTCODE:
        InStartcode();
        break;

      case IN_STARTCODE_STOPBITS:
        InStartcodeStopbits();
        break;

      case IN_DATA_STARTBIT:
        InDataStartbit();
        break;

      case IN_DATA_BITS:
        if (m_state_spi_bitcount < 7) {
          InDataBits();
        } else {
          InLastDataBit();
        }
        break;

      case IN_DATA_STOPBITS:
        InDataStopbits();
        break;

      default:
        // should not happen, but prevent an infinite loop anyways
        m_chunk_spi_bytecount++;
    }
  }

  if (m_state >= IN_DATA_STARTBIT) {
    PacketComplete();
  }
}

/**
 * Changes the current state and resets other depending variables.
 */
void SPIDMXParser::ChangeState(SPIDMXParser::dmx_state_t new_state) {
  OLA_DEBUG << "iteration: " << m_chunk_spi_bytecount
            << ", change state to " << m_state
            << ", data=" << m_chunk[m_chunk_spi_bytecount]
            << ", m_state_spi_bitcount=" << m_state_spi_bitcount;

  m_state = new_state;
  m_state_spi_bitcount = 0;

  if (m_state == WAIT_FOR_MAB) {
    m_channel_count = -1;
  }
}

/**
 * Helper function that returns the number of zeros if a falling edge is
 * detected in the given byte or -1 if the byte is not a falling edge.
 *
 * Note: A falling edge can be between two bytes.
 * Note 2: If -1 is returned, this byte can either contain random spikes or
 *         be all ones.
 */
int8_t SPIDMXParser::DetectFallingEdge(uint8_t byte) {
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
 * detected in the given byte or -1 if the byte is not a rising edge.
 *
 * Note: A rising edge can be between two bytes.
 * Note 2: If -1 is returned, this byte can either contain random spikes or
 *         be all zeros.
 */
int8_t SPIDMXParser::DetectRisingEdge(uint8_t byte) {
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
void SPIDMXParser::PacketComplete() {
  m_channel_count++;

  OLA_DEBUG << "DMX packet complete (" << m_channel_count << " channels).";

  if (m_callback) {
    m_callback->Run();
  }
}

/**
 * Stay in this state until we find a falling edge, then change to IN_BREAK.
 */
void SPIDMXParser::WaitForBreak() {
  int8_t zeros = DetectFallingEdge(m_chunk[m_chunk_spi_bytecount]);
  if (zeros > 0) {
    ChangeState(IN_BREAK);
    m_state_spi_bitcount = zeros;
  }
  m_chunk_spi_bytecount++;
}

/**
 * We have to find at least 88µs low = 165 SPI bits to be sure we have a break.
 * If so, change to WAIT_FOR_MAB, else stay here.
 */
void SPIDMXParser::InBreak() {
  if (m_chunk[m_chunk_spi_bytecount] == 0) {
    m_state_spi_bitcount += 8;

    // (88µs break / 4µs per DMX bit) * 7.5 SPI bits = 165
    if (m_state_spi_bitcount > 165) {
      ChangeState(WAIT_FOR_MAB);
    }
  } else {
    ChangeState(WAIT_FOR_BREAK);
  }
  m_chunk_spi_bytecount++;
}

/**
 * We are still in a break, so we have to either find a rising edge and change
 * to IN_MAB or stay here.
 */
void SPIDMXParser::WaitForMab() {
  uint8_t byte = m_chunk[m_chunk_spi_bytecount];
  if (byte != 0) {
    int8_t ones = DetectRisingEdge(byte);
    if (ones > 0) {
      ChangeState(IN_MAB);
      m_state_spi_bitcount = ones;
    } else {
      ChangeState(WAIT_FOR_BREAK);
    }
  }
  m_chunk_spi_bytecount++;
}

/**
 * A MAB must be at least 8µs = 15 SPI bits, so we could already find it in the
 * first handled byte. Then change to IN_STARTCODE, otherwise stay here. If we
 * get unexpected spikes, go back to WAIT_FOR_BREAK.
 */
void SPIDMXParser::InMab() {
  uint8_t byte = m_chunk[m_chunk_spi_bytecount];
  if (byte == 0xff) {
    m_state_spi_bitcount += 8;
  } else {
    int8_t zeros = DetectFallingEdge(byte);
    int8_t ones = 8 - zeros;

    // (8µs MAB / 4µs per DMX bit) * 7.5 SPI bits = 15
    if (zeros < 0 || m_state_spi_bitcount + ones <= 15) {
      ChangeState(WAIT_FOR_BREAK);
      m_chunk_spi_bytecount++;
      return;
    }

    ChangeState(IN_STARTCODE);
    m_state_spi_bitcount = zeros;
  }

  m_chunk_spi_bytecount++;
}

/**
 * The start code for DMX consists of 1 low start bit + 8 low "data" bits.
 * So we have to get between 67 and 77 low SPI bits to be sure that this is the
 * start of a valid DMX packet. If we find the right amount, change to
 * IN_STARTCODE_STOPBITS, else go back to WAIT_FOR_BREAK.
 */
void SPIDMXParser::InStartcode() {
  uint8_t byte = m_chunk[m_chunk_spi_bytecount];
  if (byte == 0x00) {
    m_state_spi_bitcount += 8;
  } else {
    int8_t ones = DetectRisingEdge(byte);
    int8_t zeros = 8 - ones;

    m_state_spi_bitcount += zeros;

    // (1 start bit + 8 NULL code bits) * 7.5 SPI bits >= 67
    // (1 start bit + 8 NULL code bits) * 8.5 SPI bits <= 77
    if (zeros < 0 || m_state_spi_bitcount <= 67 || m_state_spi_bitcount >= 77) {
      ChangeState(WAIT_FOR_BREAK);
      m_chunk_spi_bytecount++;
      return;
    }

    ChangeState(IN_STARTCODE_STOPBITS);
    m_state_spi_bitcount = ones;
  }

  m_chunk_spi_bytecount++;
}

/**
 * We have to find 2 high stop bits + optionally an arbitrarily long MBS. After
 * that, we are in the first slot's start bit (IN_DATA_STARTBIT). If the stop
 * bits are too short, go back to WAIT_FOR_BREAK.
 */
void SPIDMXParser::InStartcodeStopbits() {
  uint8_t byte = m_chunk[m_chunk_spi_bytecount];
  if (byte == 0xff) {
    m_state_spi_bitcount += 8;
  } else {
    int8_t zeros = DetectFallingEdge(byte);
    int8_t ones = 8 - zeros;

    // (8µs stop bits / 4µs per DMX bit) * 7.5 SPI bits = 15
    if (zeros < 0 || m_state_spi_bitcount + ones <= 15) {
      ChangeState(WAIT_FOR_BREAK);
      m_chunk_spi_bytecount++;
      return;
    }

    ChangeState(IN_DATA_STARTBIT);
    m_state_spi_bitcount = zeros;
  }

  m_chunk_spi_bytecount++;
}

/**
 * Now, we're close to the actual data. We always want to sample in the middle
 * of an SPI byte, so we have to calculate the sampling position. Additionally,
 * we could have to look at the last byte again. See the following table:
 *
 * d denotes the first DMX data bit
 * ^ is the desired sampling position
 * SP = sampling position
 * SBC = m_state_spi_bitcount
 *
 * SBC  last & current byte               new current byte
 * ---  -------------------               ----------------
 *
 *  8    00000000 dddddddd   -> backtrack:   00000000
 *          ^                                   ^      SP = 4
 *  7    10000000 0ddddddd   -> backtrack:   10000000
 *           ^                                   ^     SP = 3
 *  6    11000000 00dddddd   -> backtrack:   11000000
 *            ^                                   ^    SP = 2
 *  5    11100000 000ddddd   -> backtrack:   11100000
 *             ^                                   ^   SP = 1
 *  4    11110000 0000dddd   -> backtrack:   11110000
 *              ^                                   ^  SP = 0
 *  3    11111000 00000ddd   -> nop:         00000ddd
 *                ^                          ^         SP = 7
 *  2    11111100 000000dd   -> nop:         000000dd
 *                 ^                          ^        SP = 6
 *  1    11111110 0000000d   -> nop:         0000000d
 *                  ^                          ^       SP = 5
 */
void SPIDMXParser::InDataStartbit() {
  uint8_t byte = m_chunk[m_chunk_spi_bytecount];

  if (m_state_spi_bitcount >= 4) {
    // look at the last byte again and don't increase m_chunk_spi_bytecount
    byte = m_chunk[m_chunk_spi_bytecount - 1];
    m_sampling_position = m_state_spi_bitcount - 4;
  } else {
    // next byte will be handled in next step as usual
    m_chunk_spi_bytecount++;
    m_sampling_position = m_state_spi_bitcount + 8 - 4;
  }

  // start bit must be zero
  if ((byte & (1 << m_sampling_position))) {
    ChangeState(WAIT_FOR_BREAK);
  } else {
    m_current_dmx_value = 0x00;
    ChangeState(IN_DATA_BITS);
  }
}

/**
 * Handle bits 1 - 7 of a slot.
 *
 * Sample the current DMX bit at the calculated position and update the current
 * DMX value accordingly.
 * Note: m_state_spi_bitcount is abused in this state because it doesn't count
 * SPI bits but DMX bits here.
 */
void SPIDMXParser::InDataBits() {
  uint8_t byte = m_chunk[m_chunk_spi_bytecount];
  uint8_t read_bit = ((byte & (1 << m_sampling_position)) ? 1 : 0);
  m_current_dmx_value |= read_bit << m_state_spi_bitcount;

  m_state_spi_bitcount++;
  m_chunk_spi_bytecount++;
}

/**
 * In the slot's last (eight) bit, we have to synchronize again to be able to
 * find the stop bits. Change to IN_DATA_STOPBITS in every case.
 */
void SPIDMXParser::InLastDataBit() {
  uint8_t byte = m_chunk[m_chunk_spi_bytecount];
  uint8_t read_bit = ((byte & (1 << m_sampling_position)) ? 1 : 0);
  m_current_dmx_value |= read_bit << 7;

  ChangeState(IN_DATA_STOPBITS);
  // assume that bit after sample position belongs to stop bits
  if (m_sampling_position >= 4) {
    m_state_spi_bitcount = m_sampling_position;
  } else {
    m_state_spi_bitcount = m_sampling_position + 8;
    m_chunk_spi_bytecount++;  // assume next byte is 0xff
  }
  m_chunk_spi_bytecount++;
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
void SPIDMXParser::InDataStopbits() {
  uint8_t byte = m_chunk[m_chunk_spi_bytecount];
  if (byte == 0xff) {
    m_state_spi_bitcount += 8;
  } else if (byte == 0x00 && m_state_spi_bitcount <= 11
      && m_current_dmx_value == 0x00) {  // we are actually in a break
    // all later channels are definitely zero
    m_dmx_buffer->SetRangeToValue(m_channel_count + 1, 0x00,
                                  511 - m_channel_count);
    m_channel_count = 511;
    PacketComplete();

    ChangeState(IN_BREAK);
    m_state_spi_bitcount = 10 * 8;
  } else {
    int8_t zeros = DetectFallingEdge(byte);
    int8_t ones = 8 - zeros;

    // (8µs stop bits / 4µs per DMX bit) * 7.5 SPI bits = 15
    if (m_state_spi_bitcount + ones <= 15) {
      // stop bits were too short
      PacketComplete();

      ChangeState(WAIT_FOR_BREAK);
      m_chunk_spi_bytecount++;
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
        m_chunk_spi_bytecount++;
        return;
      }
    }

    m_channel_count++;  // mark channel receive as complete
    m_dmx_buffer->SetChannel(m_channel_count, m_current_dmx_value);

    if (m_channel_count == 511) {
      // last channel filled
      PacketComplete();
      ChangeState(IN_BREAK);
    } else {
      ChangeState(IN_DATA_STARTBIT);
    }
    m_state_spi_bitcount = zeros;
  }

  m_chunk_spi_bytecount++;
}

}  // namespace spidmx
}  // namespace plugin
}  // namespace ola
