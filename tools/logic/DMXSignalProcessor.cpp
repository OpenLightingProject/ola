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
 * DMXSignalProcessor.cpp
 * Process a stream of bits and decode into DMX frames.
 * Copyright (C) 2013 Simon Newton
 */

#include <ola/Logging.h>
#include <vector>

#include "tools/logic/DMXSignalProcessor.h"

using std::vector;

/*
 * For debugging..
const string byte_to_binary(int x) {
  string s;
  for (int z = 0x80; z > 0; z >>= 1) {
    s += ((x & z) == z) ? "1" : "0";
  }
  return s;
}
*/

// wait 2us, then sample
// wait 4us then sample
// x 7
// then wait for rising edge
// wait 2 bit times,
// the move back to START_BIT phase


DMXSignalProcessor::DMXSignalProcessor(DataCallback *callback,
                                       unsigned int sample_rate)
    : m_callback(callback),
      m_sample_rate(sample_rate),
      m_state(IDLE),
      m_ticks(0),
      m_microseconds_per_tick(1000000.0 / sample_rate),
      m_may_be_in_break(false),
      m_ticks_in_break(0) {
  if (m_sample_rate % DMX_BITRATE) {
    OLA_WARN << "Sample rate is not a multiple of " << DMX_BITRATE;
  }
}

    // Process more data.
void DMXSignalProcessor::Process(uint8_t *ptr, unsigned int size,
                                 uint8_t mask) {
  // OLA_INFO << "processing " << size << " samples";
  /*
  stringstream str;
  for (unsigned int i = 0 ; i < 100; i++) {
    str << std::hex << byte_to_binary(ptr[i]) << " ";
    if (i % 8 == 7) {
      OLA_INFO << i << ": " << str.str();
      str.str("");
    }
  }
  */
  for (unsigned int i = 0 ; i < size; i++) {
    ProcessSample(ptr[i] & mask);
  }
}

/**
 * Process one bit of data through the state machine.
 */
void DMXSignalProcessor::ProcessSample(bool bit) {
  if (m_may_be_in_break) {
    if (!bit) {
      m_ticks_in_break++;
    }
  }

  switch (m_state) {
    case UNDEFINED:
      if (bit) {
        SetState(IDLE);
      }
      break;
    case IDLE:
      if (bit) {
        m_ticks++;
      } else {
        SetState(BREAK);
      }
      break;
    case BREAK:
      if (bit) {
        if (DurationExceeds(MIN_BREAK_TIME)) {
          SetState(MAB);
        } else {
          OLA_INFO << "Break too short, was "
                   << TicksAsMicroSeconds() << " us";
          SetState(IDLE);
        }
      } else {
        m_ticks++;
      }
      break;
    case MAB:
      if (bit) {
        m_ticks++;
        if (DurationExceeds(MAX_MAB_TIME)) {
          SetState(IDLE, m_ticks);
        }
      } else {
        if (DurationExceeds(MIN_MAB_TIME)) {
          // OLA_INFO << "In start bit!";
          SetState(START_BIT);
        } else {
          OLA_INFO << "Mark too short, was "
                   << TicksAsMicroSeconds() << "us";
          SetState(UNDEFINED);
        }
      }
      break;
    case START_BIT:
    case BIT_1:
    case BIT_2:
    case BIT_3:
    case BIT_4:
    case BIT_5:
    case BIT_6:
    case BIT_7:
    case BIT_8:
      ProcessBit(bit);
      break;
    case STOP_BITS:
      m_ticks++;
      if (bit) {
        if (DurationExceeds(2 * MIN_BIT_TIME)) {
          AppendDataByte();
          SetState(MARK_BETWEEN_SLOTS);
        }
      } else {
        if (m_may_be_in_break) {
          HandleFrame();
          SetState(BREAK, m_ticks_in_break);
        } else {
          OLA_INFO << "Saw a low during a stop bit";
          SetState(UNDEFINED);
        }
      }
      break;
    case MARK_BETWEEN_SLOTS:
      // Wait for the falling edge, this could signal the next start bit, or a
      // new break.
      if (!bit) {
        m_may_be_in_break = true;
        // Assume it's a start bit for now, but flag that we may be in a break.
        SetState(START_BIT);
      } else {
        if (DurationExceeds(MAX_MARK_BETWEEN_SLOTS)) {
          // ok that was the end of the frame
          HandleFrame();
          SetState(IDLE);
        }
      }
      break;
    default:
      break;
  }
}

void DMXSignalProcessor::ProcessBit(bool bit) {
  if (bit) {
    // a high means this definitely isn't a break.
    m_may_be_in_break = false;
  }

  bool current_bit = SetBitIfNotDefined(bit);

  /*
  OLA_INFO << "ticks: " << m_ticks << ", current bit " << current_bit
           << ", our bit " << bit;
  */
  m_ticks++;
  if (bit == current_bit) {
    if (DurationExceeds(MAX_BIT_TIME)) {
      SetState(static_cast<State>(m_state + 1));
    }
  } else {
    if (DurationExceeds(MIN_BIT_TIME)) {
      SetState(static_cast<State>(m_state + 1));
    } else {
      OLA_INFO << "Bit was too short, was "
               << TicksAsMicroSeconds() << "us";
      SetState(UNDEFINED);
    }
  }
}

bool DMXSignalProcessor::SetBitIfNotDefined(bool bit) {
  if (m_state == START_BIT) {
    return false;
  }

  int offset = m_state - BIT_1;
  if (!m_bits_defined[offset]) {
    // OLA_INFO << "Set bit " << offset << " to " << bit;
    m_current_byte.push_back(bit);
    m_bits_defined[offset] = true;
  }
  return m_current_byte[offset];
}

void DMXSignalProcessor::AppendDataByte() {
  uint8_t byte = 0;
  for (unsigned int i = 0; i < 8; i++) {
    // LSB first
    byte |= (m_current_byte[i] << i);
  }
  // OLA_INFO << "Byte " << m_dmx_data.size() << " is " << (int) byte;
  m_dmx_data.push_back(byte);
  m_bits_defined.assign(8, false);
  m_current_byte.clear();
}

void DMXSignalProcessor::HandleFrame() {
  // OLA_INFO << "--------------- END OF FRAME ------------------";
  OLA_INFO << "Got frame of size " << m_dmx_data.size();
  if (m_callback && !m_dmx_data.empty()) {
    m_callback->Run(&m_dmx_data[0], m_dmx_data.size());
  }
  m_dmx_data.clear();
}

void DMXSignalProcessor::SetState(State state, unsigned int ticks) {
  m_state = state;
  m_ticks = ticks;
  // OLA_INFO << "moving to state " << state;
  if (state == MAB) {
    m_dmx_data.clear();
  } else if (state == START_BIT) {
    // The reset should be done in AppendDataByte but do it again to be safe.
    m_bits_defined.assign(8, false);
    m_current_byte.clear();
  }
}

// Return true if the current number of ticks exceeds micro_seconds.
// Due to sampling this can be wrong by +- m_microseconds_per_tick.
bool DMXSignalProcessor::DurationExceeds(double micro_seconds) {
  return m_ticks * m_microseconds_per_tick > micro_seconds;
}

// Return the current number of ticks in microseconds.
double DMXSignalProcessor::TicksAsMicroSeconds() {
  return m_ticks * m_microseconds_per_tick;
}
