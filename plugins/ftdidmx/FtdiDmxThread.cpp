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
#include "ola/StringUtils.h"
#include "plugins/ftdidmx/FtdiWidget.h"
#include "plugins/ftdidmx/FtdiDmxThread.h"

namespace ola {
namespace plugin {
namespace ftdidmx {

FtdiDmxThread::FtdiDmxThread(FtdiWidget *device, unsigned int freq)
  : m_device(device),
    m_term(false),
    m_frequency(freq) {
}

FtdiDmxThread::~FtdiDmxThread() {
  Stop();
}

void FtdiDmxThread::CheckTimeGranularity() {
  TimeStamp ts1, ts2;
  Clock::CurrentTime(&ts1);
  usleep(1000);
  Clock::CurrentTime(&ts2);

  TimeInterval interval = ts2 - ts1;
  if (interval.InMilliSeconds() > 3) {
    m_granularity = Bad;
  } else {
    m_granularity = Good;
  }
}

bool FtdiDmxThread::Stop() {
  {
    ola::thread::MutexLocker locker(&m_term_mutex);
    m_term = true;
  }
  return Join();
}

bool FtdiDmxThread::WriteDMX(const DmxBuffer &buffer) {
  {
    ola::thread::MutexLocker locker(&m_buffer_mutex);
    m_buffer = buffer;
    return true;
  }
}

void *FtdiDmxThread::Run() {
  TimeStamp ts1, ts2;
  CheckTimeGranularity();
  int frameTime = static_cast<int>(floor(
    (static_cast<double>(1000) / m_frequency) + static_cast<double>(0.5)));

  // Setup the device
  if (m_device->Open() == false)
    OLA_WARN << "Error Opening device";
  if (m_device->Reset() == false)
    OLA_WARN << "Error Resetting device";
  if (m_device->SetBaudRate() == false)
    OLA_WARN << "Error Setting baudrate";
  if (m_device->SetLineProperties() == false)
    OLA_WARN << "Error setting line properties";
  if (m_device->SetFlowControl() == false)
    OLA_WARN << "Error setting flow control";
  if (m_device->PurgeBuffers() == false)
    OLA_WARN << "Error purging buffers";
  if (m_device->ClearRts() == false)
    OLA_WARN << "Error clearing rts";

  while (1) {
    {
      ola::thread::MutexLocker locker(&m_term_mutex);
      if (m_term)
        break;
    }

    Clock::CurrentTime(&ts1);

    if (m_device->SetBreak(true) == false)
      goto framesleep;

    if (m_granularity == Good)
      usleep(DMX_BREAK);

    if (m_device->SetBreak(false) == false)
      goto framesleep;

    if (m_granularity == Good)
      usleep(DMX_MAB);

    if (m_device->Write(m_buffer) == false)
      goto framesleep;

  framesleep:
    // Sleep for the remainder of the DMX frame time
    Clock::CurrentTime(&ts2);
    TimeInterval elapsed = ts2 - ts1;

    if (m_granularity == Good) {
      while (elapsed.InMilliSeconds() < frameTime) {
        usleep(1000);
        Clock::CurrentTime(&ts2);
        elapsed = ts2 - ts1;
      }
    } else {
      while (elapsed.InMilliSeconds() < frameTime) {
        Clock::CurrentTime(&ts2);
        elapsed = ts2 - ts1;
      }
    }
  }

  return NULL;
}
}
}
}
