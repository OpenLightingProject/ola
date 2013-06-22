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
 * DMXSignalProcessor.h
 * Process a stream of bits and decode into DMX frames.
 * Copyright (C) 2013 Simon Newton
 */

#ifndef TOOLS_LOGIC_DMXSIGNALPROCESSOR_H_
#define TOOLS_LOGIC_DMXSIGNALPROCESSOR_H_

#include <ola/io/SelectServer.h>
#include <ola/thread/Mutex.h>

#include <ola/BaseTypes.h>
#include <ola/Callback.h>
#include <ola/network/NetworkUtils.h>

using ola::io::SelectServer;

using ola::thread::Mutex;
using ola::thread::MutexLocker;
using ola::NewSingleCallback;

/**
 * Process a DMX signal.
 *
 * See E1.11 for the details. It generally goes something like
 *  Mark (Idle) - High
 *  Break - Low
 *  Mark After Break - High
 * start bit (low)
 * LSB to MSB
 * 2 stop bits (high)
 */
class DMXSignalProcessor {
  public:
    typedef ola::Callback2<void, const uint8_t*, unsigned int> DataCallback;

    DMXSignalProcessor(DataCallback *callback, unsigned int sample_rate);

    // Reset the processor. Used if there is a gap in the stream.
    void Reset() {
      SetState(IDLE);
    }

    // Process more data.
    void Process(uint8_t *ptr, unsigned int size, uint8_t mask = 0xff);

  private:
    enum State {
      UNDEFINED,  // when the signal is low and we have no idea where we are.
      IDLE,
      BREAK,
      MAB,
      START_BIT,
    };

    DataCallback *m_callback;
    unsigned int m_sample_rate;
    State m_state;
    unsigned int m_ticks;
    double m_microseconds_per_tick;

    void ProcessBit(bool bit);

    void SetState(State state, unsigned int ticks = 1);
    bool DurationExceeds(double micro_seconds);
    double TicksAsMicroSeconds();

    static const unsigned int DMX_BITRATE = 250000;
    // These are all in microseconds.
    static const double MIN_BREAK_TIME = 88.0;
    static const double MIN_MAB_TIME = 1.0;
    static const double MAX_MAB_TIME = 1000000.0;
};
#endif  // TOOLS_LOGIC_DMXSIGNALPROCESSOR_H_
