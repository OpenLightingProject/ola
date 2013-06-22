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

#include "tools/logic/DMXSignalProcessor.h"

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


DMXSignalProcessor::DMXSignalProcessor(DataCallback *callback,
                                       unsigned int sample_rate)
    : m_callback(callback),
      m_sample_rate(sample_rate),
      m_state(IDLE),
      m_ticks(0),
      m_microseconds_per_tick(1000000.0 / sample_rate) {
  if (m_sample_rate % DMX_BITRATE) {
    OLA_WARN << "Sample rate is not a multiple of " << DMX_BITRATE;
  }
}

    // Process more data.
void DMXSignalProcessor::Process(uint8_t *ptr, unsigned int size,
                                 uint8_t mask) {
  OLA_INFO << "processing " << size << " samples";
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
    ProcessBit(ptr[i] & mask);
  }
}

/**
 * Process one bit of data through the state machine.
 */
void DMXSignalProcessor::ProcessBit(bool bit) {
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
          OLA_INFO << "In start bit!";
          SetState(START_BIT);
        } else {
          OLA_INFO << "Mark too short, was "
                   << TicksAsMicroSeconds() << "us";
          SetState(UNDEFINED);
        }
      }
      break;
    case START_BIT:
      if (bit) {

      } else {

      }
      break;
  }
}

void DMXSignalProcessor::SetState(State state, unsigned int ticks) {
  m_state = state;
  m_ticks = ticks;
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
