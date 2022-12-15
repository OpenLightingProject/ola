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
 * UartDmxThread.cpp
 * The DMX through a UART plugin for ola
 * Copyright (C) 2011 Rui Barreiros
 * Copyright (C) 2014 Richard Ash
 */

#include <math.h>
#include <unistd.h>
#include <string>
#include <algorithm>
#include "ola/Clock.h"
#include "ola/Constants.h"
#include "ola/Logging.h"
#include "ola/StringUtils.h"
#include "plugins/uartdmx/UartWidget.h"
#include "plugins/uartdmx/UartDmxThread.h"

namespace ola {
namespace plugin {
namespace uartdmx {

UartDmxThread::UartDmxThread(UartWidget *widget, unsigned int breakt,
                             unsigned int malft)
  : m_granularity(UNKNOWN),
    m_widget(widget),
    m_term(false),
    m_breakt(breakt),
    m_malft(malft) {
}

UartDmxThread::~UartDmxThread() {
  Stop();
}


/**
 * Stop this thread
 */
bool UartDmxThread::Stop() {
  {
    ola::thread::MutexLocker locker(&m_term_mutex);
    m_term = true;
  }
  return Join();
}


/**
 * Copy a DMXBuffer to the output thread
 */
bool UartDmxThread::WriteDMX(const DmxBuffer &buffer) {
  ola::thread::MutexLocker locker(&m_buffer_mutex);
  m_buffer.Set(buffer);
  return true;
}


/**
 * The method called by the thread
 */
void *UartDmxThread::Run() {
  CheckTimeGranularity();
  DmxBuffer buffer;

  // basic frame time (without break): MAB + (time per bit * bits per slot * (slots per universe + 1)) + MALFT
  // the +1 at the slots_per_universe is for the dmx start code
  // the basic_frame_time is in microseconds
  int basic_frame_time = DMX_MAB + (4 * 11 * (DMX_UNIVERSE_SIZE + 1)) + m_malft;

  // the maximum time between breaks should be 1 second, and the minimum time 1204
  // the capping of max and min values is done to honor the standard
  int basic_frame_time_capped = std::max(std::min(basic_frame_time, 1000000), 1204);

  // converts the frame time into milliseconds for later use
  m_frameTime = static_cast<int>(floor(basic_frame_time_capped / static_cast<double>(1000)));

  // Setup the widget
  if (!m_widget->IsOpen())
    m_widget->SetupOutput();

  while (1) {
    {
      ola::thread::MutexLocker locker(&m_term_mutex);
      if (m_term)
        break;
    }

    {
      ola::thread::MutexLocker locker(&m_buffer_mutex);
      buffer.Set(m_buffer);
    }

    writeDmxData(buffer);
  }
  return NULL;
}

/**
 * Write the DMX data to the actual UART interface.
 */
void UartDmxThread::writeDmxData(const DmxBuffer &buffer) {
  TimeStamp ts1;
  Clock clock;

  // ensures that ts1 has a valid value for the framesleep if break condition cannot be locked/released
  clock.CurrentMonotonicTime(&ts1);

  if (!m_widget->SetBreak(true)) {
    frameSleep(ts1);
    return;
  }

  if (m_granularity == GOOD)
    usleep(m_breakt);

  if (!m_widget->SetBreak(false)) {
    frameSleep(ts1);
    return;
  }

  // stores the time stamp for the elapsed time calculation during the framesleep
  clock.CurrentMonotonicTime(&ts1);

  if (m_granularity == GOOD)
    usleep(DMX_MAB);

  m_widget->Write(buffer);
  
  frameSleep(ts1);
}

/**
 * Sleeps for the rest of the frame time and tries to recover the granularity if needed. 
 */
void UartDmxThread::frameSleep(const TimeStamp &ts1) {
  TimeStamp ts2, ts3;
  Clock clock;

  // Sleep for the remainder of the DMX frame time
  clock.CurrentMonotonicTime(&ts2);
  TimeInterval elapsed = ts2 - ts1;

  if (m_granularity == GOOD) {
    while (elapsed.InMilliSeconds() < m_frameTime) {
      usleep(1000);
      clock.CurrentMonotonicTime(&ts2);
      elapsed = ts2 - ts1;
    }
  } else {
    // See if we can drop out of bad mode.
    usleep(1000);
    clock.CurrentMonotonicTime(&ts3);
    TimeInterval interval = ts3 - ts2;
    if (interval.InMilliSeconds() <= BAD_GRANULARITY_LIMIT) {
      m_granularity = GOOD;
      OLA_INFO << "Switching from BAD to GOOD granularity for UART thread";
    }

    elapsed = ts3 - ts1;
    while (elapsed.InMilliSeconds() < m_frameTime) {
      clock.CurrentMonotonicTime(&ts2);
      elapsed = ts2 - ts1;
    }
  }
}

/**
 * Check the granularity of usleep.
 */
void UartDmxThread::CheckTimeGranularity() {
  TimeStamp ts1, ts2;
  Clock clock;

  clock.CurrentMonotonicTime(&ts1);
  usleep(1000);
  clock.CurrentMonotonicTime(&ts2);

  TimeInterval interval = ts2 - ts1;
  m_granularity = (interval.InMilliSeconds() > BAD_GRANULARITY_LIMIT) ?
      BAD : GOOD;
  OLA_INFO << "Granularity for UART thread is "
           << (m_granularity == GOOD ? "GOOD" : "BAD");
}
}  // namespace uartdmx
}  // namespace plugin
}  // namespace ola
