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
 * FtdiDmxThread.h
 * The FTDI usb chipset DMX plugin for ola
 * Copyright (C) 2011 Rui Barreiros
 */

#ifndef PLUGINS_FTDIDMX_FTDIDMXTHREAD_H_
#define PLUGINS_FTDIDMX_FTDIDMXTHREAD_H_

#define DMX_MAB 16
#define DMX_BREAK 110

#include "ola/DmxBuffer.h"
#include "olad/Preferences.h"
#include "ola/thread/Thread.h"
#include "plugins/ftdidmx/FtdiDmxDevice.h"

namespace ola {
namespace plugin {
namespace ftdidmx {
  
class FtdiDmxThread : public ola::thread::Thread {
  public:
    FtdiDmxThread(FtdiDmxDevice *device, Preferences *preferences) : m_device(device), m_preferences(preferences) { SetupPreferences(); }
    ~FtdiDmxThread() {}

    bool Stop();
    void *Run();
    bool WriteDMX(const DmxBuffer &buffer);
    
  private:
    void CheckTimeGranularity();
    void SetupPreferences();

    enum TimerGranularity { Unknown, Good, Bad };

    TimerGranularity m_granularity;
    FtdiDmxDevice *m_device;
    int unsigned m_frequency;
    bool m_isRunning;
    DmxBuffer m_buffer;
    Preferences *m_preferences;
};

}
}
}

#endif
