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
#include "ola/Clock.h"
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
  TimeStamp ts1, ts2, ts3;
  Clock clock;
  CheckTimeGranularity();
  DmxBuffer buffer;

  int frameTime = static_cast<int>(floor(
    (m_malft / static_cast<double>(1000)) + static_cast<double>(0.5)));

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

    clock.CurrentMonotonicTime(&ts1);

    if (!m_widget->SetBreak(true))
      goto framesleep;

    if (m_granularity == GOOD)
      usleep(m_breakt);

    if (!m_widget->SetBreak(false))
      goto framesleep;

    if (m_granularity == GOOD)
      usleep(DMX_MAB);

    if (!m_widget->Write(buffer))
      goto framesleep;

  framesleep:
    // Sleep for the remainder of the DMX frame time
    clock.CurrentMonotonicTime(&ts2);
    TimeInterval elapsed = ts2 - ts1;

    if (m_granularity == GOOD) {
      while (elapsed.InMilliSeconds() < frameTime) {
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
      while (elapsed.InMilliSeconds() < frameTime) {
        clock.CurrentMonotonicTime(&ts2);
        elapsed = ts2 - ts1;
      }
    }
  }
  return NULL;
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
