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
 * FtdiDmxThread.cpp
 * The FTDI usb chipset DMX plugin for ola
 * Copyright (C) 2011 Rui Barreiros
 *
 * Additional modifications to enable support for multiple outputs and
 * additional device ids did change the original structure.
 *
 * by E.S. Rosenberg a.k.a. Keeper of the Keys 5774/2014
 */

#include <math.h>
#include <unistd.h>

#include <string>

#include "ola/Clock.h"
#include "ola/Logging.h"
#include "ola/StringUtils.h"
#include "plugins/ftdidmx/FtdiWidget.h"
#include "plugins/ftdidmx/FtdiDmxThread.h"

namespace ola {
namespace plugin {
namespace ftdidmx {

FtdiDmxThread::FtdiDmxThread(FtdiInterface *interface, unsigned int frequency)
  : m_granularity(UNKNOWN),
    m_interface(interface),
    m_term(false),
    m_frequency(frequency) {
}

FtdiDmxThread::~FtdiDmxThread() {
  Stop();
}


/**
 * @brief Stop this thread
 */
bool FtdiDmxThread::Stop() {
  {
    ola::thread::MutexLocker locker(&m_term_mutex);
    m_term = true;
  }
  return Join();
}


/**
 * @brief Copy a DMXBuffer to the output thread
 */
bool FtdiDmxThread::WriteDMX(const DmxBuffer &buffer) {
  {
    ola::thread::MutexLocker locker(&m_buffer_mutex);
    m_buffer.Set(buffer);
    return true;
  }
}


/**
 * @brief The method called by the thread
 */
void *FtdiDmxThread::Run() {
  TimeStamp ts1, ts2, ts3;
  Clock clock;
  CheckTimeGranularity();
  DmxBuffer buffer;

  int frameTime = static_cast<int>(floor(
    (static_cast<double>(1000) / m_frequency) + static_cast<double>(0.5)));

  // Setup the interface
  if (!m_interface->IsOpen()) {
    m_interface->SetupOutput();
  }

  while (1) {
    {
      ola::thread::MutexLocker locker(&m_term_mutex);
      if (m_term) {
        break;
      }
    }

    {
      ola::thread::MutexLocker locker(&m_buffer_mutex);
      buffer.Set(m_buffer);
    }

    clock.CurrentMonotonicTime(&ts1);

    if (!m_interface->SetBreak(true)) {
      goto framesleep;
    }

    if (m_granularity == GOOD) {
      usleep(DMX_BREAK);
    }

    if (!m_interface->SetBreak(false)) {
      goto framesleep;
    }

    if (m_granularity == GOOD) {
      usleep(DMX_MAB);
    }

    if (!m_interface->Write(buffer)) {
      goto framesleep;
    }

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
      if (interval.InMilliSeconds() < BAD_GRANULARITY_LIMIT) {
        m_granularity = GOOD;
        OLA_INFO << "Switching from BAD to GOOD granularity for ftdi thread";
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
 * @brief Check the granularity of usleep.
 */
void FtdiDmxThread::CheckTimeGranularity() {
  TimeStamp ts1, ts2;
  Clock clock;

  clock.CurrentMonotonicTime(&ts1);
  usleep(1000);
  clock.CurrentMonotonicTime(&ts2);

  TimeInterval interval = ts2 - ts1;
  m_granularity = (interval.InMilliSeconds() > BAD_GRANULARITY_LIMIT) ?
      BAD : GOOD;
  OLA_INFO << "Granularity for FTDI thread is "
           << ((m_granularity == GOOD) ? "GOOD" : "BAD");
}
}  // namespace ftdidmx
}  // namespace plugin
}  // namespace ola
