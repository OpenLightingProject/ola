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
 * DMXSignalProcessor.h
 * Process a stream of bits and decode into DMX frames.
 * Copyright (C) 2013 Simon Newton
 */

#ifndef LIBS_SNIFFER_DMXSIGNALPROCESSOR_H_
#define LIBS_SNIFFER_DMXSIGNALPROCESSOR_H_

#include <ola/Callback.h>
#include <ola/Constants.h>
#include <ola/network/NetworkUtils.h>

#include <vector>

/**
 * Process a DMX signal.
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
    void Process(uint8_t *ptr, unsigned int size, uint8_t mask = 0xff,
                 unsigned int width = 1);

 private:
    enum State {
      UNDEFINED,  // when the signal is low and we have no idea where we are.
      IDLE,
      BREAK,
      MAB,
      START_BIT,
      BIT_1,
      BIT_2,
      BIT_3,
      BIT_4,
      BIT_5,
      BIT_6,
      BIT_7,
      BIT_8,
      STOP_BITS,
      MARK_BETWEEN_SLOTS,
    };

    // Set once in the constructor
    DataCallback* const m_callback;
    const unsigned int m_sample_rate;
    const double m_microseconds_per_tick;

    // our current state.
    State m_state;
    // the number of ticks (samples) we've been in this state.
    unsigned int m_ticks;
    // sometimes we may not know if we're in a break or not, see the comments
    // in DMXSignalProcessor.cpp
    bool m_may_be_in_break;
    unsigned int m_ticks_in_break;

    // Used to accumulate the bits in the current byte.
    std::vector<bool> m_bits_defined;
    std::vector<bool> m_current_byte;

    // The bytes are stored here.
    std::vector<uint8_t> m_dmx_data;

    void ProcessSample(bool bit);
    void ProcessBit(bool bit);
    bool SetBitIfNotDefined(bool bit);
    void AppendDataByte();
    void HandleFrame();

    void SetState(State state, unsigned int ticks = 1);
    bool DurationExceeds(double micro_seconds);
    double TicksAsMicroSeconds();

    static const unsigned int DMX_BITRATE = 250000;
    // These are all in microseconds and are the receiver side limits.
    static const double MIN_BREAK_TIME;
    static const double MIN_MAB_TIME;
    static const double MAX_MAB_TIME;
    // The minimum bit time, based on a 4MHz sample rate.
    // TODO(simon): adjust this based on the sample rate.
    static const double MIN_BIT_TIME;
    static const double MAX_BIT_TIME;
    static const double MIN_LAST_BIT_TIME;
    static const double MAX_MARK_BETWEEN_SLOTS;
};
#endif  // LIBS_SNIFFER_DMXSIGNALPROCESSOR_H_
