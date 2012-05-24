/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * FtdiDmxThread.cpp
 * The FTDI usb chipset DMX plugin for ola
 * Copyright (C) 2011 Rui Barreiros
 */

#include <math.h>
#include <string>

#include "ola/Clock.h"
#include "ola/Logging.h"
#include "ola/StringUtils.h"
#include "plugins/ftdidmx/FtdiWidget.h"
#include "plugins/ftdidmx/FtdiDmxThread.h"

namespace ola {
namespace plugin {
namespace ftdidmx {

FtdiDmxThread::FtdiDmxThread(FtdiWidget *widget, unsigned int frequency)
  : m_granularity(UNKNOWN),
    m_widget(widget),
    m_term(false),
    m_frequency(frequency) {
}

FtdiDmxThread::~FtdiDmxThread() {
  Stop();
}


/**
 * Stop this thread
 */
bool FtdiDmxThread::Stop() {
  {
    ola::thread::MutexLocker locker(&m_term_mutex);
    m_term = true;
  }
  return Join();
}


/**
 * Copy a DMXBuffer to the output thread
 */
bool FtdiDmxThread::WriteDMX(const DmxBuffer &buffer) {
  {
    ola::thread::MutexLocker locker(&m_buffer_mutex);
    m_buffer = buffer;
    return true;
  }
}


/**
 * The method called by the thread
 */
void *FtdiDmxThread::Run() {
  TimeStamp ts1, ts2;
  Clock clock;
  CheckTimeGranularity();
  int frameTime = static_cast<int>(floor(
    (static_cast<double>(1000) / m_frequency) + static_cast<double>(0.5)));

  // Setup the widget
  if(!m_widget->IsOpen())
    m_widget->SetupOutput();

  while (1) {
    {
      ola::thread::MutexLocker locker(&m_term_mutex);
      if (m_term)
        break;
    }

    clock.CurrentTime(&ts1);

    if (!m_widget->SetBreak(true))
      goto framesleep;

    if (m_granularity == GOOD)
      usleep(DMX_BREAK);

    if (!m_widget->SetBreak(false))
      goto framesleep;

    if (m_granularity == GOOD)
      usleep(DMX_MAB);

    if (!m_widget->Write(m_buffer))
      goto framesleep;

  framesleep:
    // Sleep for the remainder of the DMX frame time
    clock.CurrentTime(&ts2);
    TimeInterval elapsed = ts2 - ts1;

    if (m_granularity == GOOD) {
      while (elapsed.InMilliSeconds() < frameTime) {
        usleep(1000);
        clock.CurrentTime(&ts2);
        elapsed = ts2 - ts1;
      }
    } else {
      while (elapsed.InMilliSeconds() < frameTime) {
        clock.CurrentTime(&ts2);
        elapsed = ts2 - ts1;
      }
    }
  }
  return NULL;
}


/**
 * Check the granularity of usleep.
 */
void FtdiDmxThread::CheckTimeGranularity() {
  TimeStamp ts1, ts2;
  Clock clock;

  clock.CurrentTime(&ts1);
  usleep(1000);
  clock.CurrentTime(&ts2);

  TimeInterval interval = ts2 - ts1;
  m_granularity = interval.InMilliSeconds() > 3 ? BAD : GOOD;
  OLA_INFO << "Granularity for ftdi thread is " <<
    (m_granularity == GOOD ? "GOOD" : "BAD");
}
}  // ftdidmx
}  // plugin
}  // ola
