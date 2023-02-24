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
 * DMXSignalProcessor.cpp
 * Process a stream of bits and decode into DMX frames.
 * Copyright (C) 2013 Simon Newton
 *
 * See E1.11 for the details including timing. It generally goes something
 * like:
 *  Mark (Idle) - High
 *  Break - Low
 *  Mark After Break - High
 *  Start bit (low)
 *  LSB to MSB (8)
 *  2 stop bits (high)
 *  Mark between slots (high)
 *
 * There are a number of interesting cases which we need to handle:
 *
 * Variable bit length
 *
 * Start bit vs Break.
 *  After the stop bits comes an optional mark time between slots, that can
 *  range up to 1s. When the next falling edge occurs, it could either be a
 *  break (indicating the previous frame is now complete) or a start bit. If a
 *  rising edge occurs before 35.28 (9 * 3.92) us then it was a start-bit. If
 *  36.72 (9 * 4.08) useconds passes and there was no rising edge it's a break.
 *
 * The implementation is based on a state machine, with a couple of tweaks.
 */

#include <ola/Logging.h>
#include <vector>

#include "tools/logic/DMXSignalProcessor.h"

using std::vector;

const double DMXSignalProcessor::MIN_BREAK_TIME = 88.0;
const double DMXSignalProcessor::MIN_MAB_TIME = 8.0;
const double DMXSignalProcessor::MAX_MAB_TIME = 1000000.0;
const double DMXSignalProcessor::MIN_BIT_TIME = 3.75;
const double DMXSignalProcessor::MAX_BIT_TIME = 4.08;
const double DMXSignalProcessor::MIN_LAST_BIT_TIME = 2.64;
const double DMXSignalProcessor::MAX_MARK_BETWEEN_SLOTS = 1000000.0;

/**
 * Create a new DMXSignalProcessor which runs the specified callback when a new
 * frame is received.
 */
DMXSignalProcessor::DMXSignalProcessor(DataCallback *callback,
                                       unsigned int sample_rate)
    : m_callback(callback),
      m_sample_rate(sample_rate),
      m_microseconds_per_tick(1000000.0 / sample_rate),
      m_state(IDLE),
      m_ticks(0),
      m_may_be_in_break(false),
      m_ticks_in_break(0) {
  if (m_sample_rate % DMX_BITRATE) {
    OLA_WARN << "Sample rate is not a multiple of " << DMX_BITRATE;
  }
}

/*
 * Process the data stream. We pass in a uint8_t array rather than a bool
 * array, since it's the same size anyway. The mask is used to indicate how to
 * interpret the data.
 * @param ptr the data stream to process
 * @param size the number of samples in the stream
 * @param mask the value to be AND'ed with each sample to determine if the
 *   signal is high or low.
 */
void DMXSignalProcessor::Process(uint8_t *ptr, unsigned int size,
                                 uint8_t mask) {
  for (unsigned int i = 0 ; i < size; i++) {
    ProcessSample(ptr[i] & mask);
  }
}

/**
 * Process one bit of data through the state machine.
 */
void DMXSignalProcessor::ProcessSample(bool bit) {
  if (m_may_be_in_break && !bit) {
    // if we may be in a break, keep track of the time since the falling edge.
    m_ticks_in_break++;
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
          OLA_WARN << "Break too short, was " << TicksAsMicroSeconds()
                   << " us";
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
          OLA_WARN << "Mark too short, was " << TicksAsMicroSeconds() << "us";
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
          OLA_WARN << "Saw a low during a stop bit";
          SetState(UNDEFINED);
        }
      }
      break;
    case MARK_BETWEEN_SLOTS:
      // Wait for the falling edge, this could signal the next start bit, or a
      // new break.
      m_ticks++;
      if (bit) {
        if (DurationExceeds(MAX_MARK_BETWEEN_SLOTS)) {
          // ok, that was the end of the frame.
          HandleFrame();
          SetState(IDLE);
        }
      } else {
        m_may_be_in_break = true;
        // Assume it's a start bit for now, but flag that we may be in a break.
        SetState(START_BIT);
      }
      break;
    default:
      break;
  }
}

/**
 * Process a sample that makes up a bit of data.
 */
void DMXSignalProcessor::ProcessBit(bool bit) {
  if (bit) {
    // a high at this stage means this definitely isn't a break.
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
    // Because we force a transition into the next state (bit) after
    // MAX_BIT_TIME. The last bit may appear to be too short. This math is as
    // follows:
    //  min time for 9 bits = 9 * 3.92 = 35.28
    //  max time for 8 bits = 8 * 4.08 = 32.64
    //  difference = 2.64
    if ((m_state == BIT_8 && DurationExceeds(MIN_LAST_BIT_TIME)) ||
        DurationExceeds(MIN_BIT_TIME)) {
      SetState(static_cast<State>(m_state + 1));
    } else {
      OLA_WARN << "Bit " << m_state << " was too short, was "
               << TicksAsMicroSeconds() << "us";
      SetState(UNDEFINED);
    }
  }
}

/**
 * This is where we accumulate the bit values, before packing them into a byte.
 * This method does a couple of things:
 *  If there is no known value for the bit, it sets one.
 *  Return the value of the bit.
 */
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

/**
 * Pack the 8 bit values into a byte, and append it to the vector of bytes.
 */
void DMXSignalProcessor::AppendDataByte() {
  uint8_t byte = 0;
  for (unsigned int i = 0; i < 8; i++) {
    // LSB first
    byte |= (m_current_byte[i] << i);
  }
  OLA_INFO << "Byte " << m_dmx_data.size() << " is "
           << static_cast<int>(byte) << " ( 0x" << std::hex
           << static_cast<int>(byte) << " )";
  m_dmx_data.push_back(byte);
  m_bits_defined.assign(8, false);
  m_current_byte.clear();
}

/**
 * Called when we know the previous frame is complete. This invokes the
 * callback if there is one, and resets the vector.
 */
void DMXSignalProcessor::HandleFrame() {
  // OLA_INFO << "--------------- END OF FRAME ------------------";
  OLA_INFO << "Got frame of size " << m_dmx_data.size();
  if (m_callback && !m_dmx_data.empty()) {
    m_callback->Run(&m_dmx_data[0], m_dmx_data.size());
  }
  m_dmx_data.clear();
}

/**
 * Used to transition between states
 */
void DMXSignalProcessor::SetState(State state, unsigned int ticks) {
  OLA_INFO << "Transition to " << state << ", prev duration was "
           << TicksAsMicroSeconds();
  m_state = state;
  m_ticks = ticks;
  if (state == UNDEFINED) {
    // if we have a partial frame, we should send that up the stack
    HandleFrame();
  } else if (state == MAB) {
    m_dmx_data.clear();
  } else if (state == START_BIT) {
    // The reset should be done in AppendDataByte but do it again to be safe.
    m_bits_defined.assign(8, false);
    m_current_byte.clear();
  }
}

/*
 * Return true if the current number of ticks exceeds micro_seconds.
 * Due to sampling this can be wrong by +- m_microseconds_per_tick.
 */
bool DMXSignalProcessor::DurationExceeds(double micro_seconds) {
  return m_ticks * m_microseconds_per_tick >= micro_seconds;
}

/*
 * Return the current number of ticks in microseconds.
 */
double DMXSignalProcessor::TicksAsMicroSeconds() {
  return m_ticks * m_microseconds_per_tick;
}
