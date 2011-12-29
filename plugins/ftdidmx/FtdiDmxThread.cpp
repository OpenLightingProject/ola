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

#include "ola/Clock.h"
#include "ola/StringUtils.h"
#include "plugins/ftdidmx/FtdiDmxPlugin.h"
#include "plugins/ftdidmx/FtdiDmxThread.h"

namespace ola {
namespace plugin {
namespace ftdidmx {

void FtdiDmxThread::SetupPreferences()
{
  string value = m_preferences->GetValue(FtdiDmxPlugin::K_FREQUENCY);

  if (!ola::StringToInt(value, &m_frequency))
    ola::StringToInt(FtdiDmxPlugin::DEFAULT_FREQUENCY, &m_frequency);
}

void FtdiDmxThread::CheckTimeGranularity()
{
  TimeStamp ts1, ts2;
  Clock::CurrentTime(&ts1);
  usleep(1000);
  Clock::CurrentTime(&ts2);
  
  TimeInterval interval = ts2 - ts1;
  if(interval.InMilliSeconds() > 3) {
    m_granularity = Bad;
  } else {
    m_granularity = Good;
  }
}

bool FtdiDmxThread::Stop()
{
  m_isRunning = false;
  return Join();
}

bool FtdiDmxThread::WriteDMX(const DmxBuffer &buffer)
{
  m_buffer = buffer;
  return true;
}

void *FtdiDmxThread::Run()
{
  TimeStamp ts1, ts2;
  CheckTimeGranularity();
  int frameTime = (int) floor(((double)1000 / m_frequency) + (double)0.5);
  
  // Setup the device
  if(m_device->GetWidget()->Open() == false) OLA_WARN << "Error Opening device";
  if(m_device->GetWidget()->Reset() == false) OLA_WARN << "Error Resetting device";
  if(m_device->GetWidget()->SetBaudRate() == false) OLA_WARN << "Error Setting baudrate";
  if(m_device->GetWidget()->SetLineProperties() == false) OLA_WARN << "Error setting line properties";
  if(m_device->GetWidget()->SetFlowControl() == false) OLA_WARN << "Error setting flow control";
  if(m_device->GetWidget()->PurgeBuffers() == false) OLA_WARN << "Error purging buffers";
  if(m_device->GetWidget()->ClearRts() == false) OLA_WARN << "Error clearing rts";

  m_isRunning = true;
  while(m_isRunning == true)
  {
    Clock::CurrentTime(&ts1);
    
    if(m_device->GetWidget()->SetBreak(true) == false)
      goto framesleep;

    if(m_granularity == Good)
      usleep(DMX_BREAK);

    if(m_device->GetWidget()->SetBreak(false) == false)
      goto framesleep;
      
    if (m_granularity == Good)
      usleep(DMX_MAB);

    if (m_device->GetWidget()->Write(m_buffer) == false)
      goto framesleep;

  framesleep:
    // Sleep for the remainder of the DMX frame time
    Clock::CurrentTime(&ts2);
    TimeInterval elapsed = ts2 - ts1;

    if (m_granularity == Good)
    {
      while (elapsed.InMilliSeconds() < frameTime) 
      { 
	usleep(1000);
	Clock::CurrentTime(&ts2);
	elapsed = ts2 - ts1;
      }
    }
    else
    {
      while (elapsed.InMilliSeconds() < frameTime) 
      { 
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
