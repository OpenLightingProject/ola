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
 * OpenDmxThread.h
 * Thread for the open dmx device
 * Copyright (C) 2005-2009 Simon Newton
 */

#ifndef PLUGINS_OPENDMX_OPENDMXTHREAD_H_
#define PLUGINS_OPENDMX_OPENDMXTHREAD_H_

#include <string>
#include "ola/DmxBuffer.h"
#include "ola/thread/Thread.h"

namespace ola {
namespace plugin {
namespace opendmx {

class OpenDmxThread: public ola::thread::Thread {
  public:
    explicit OpenDmxThread(const string &path);
    ~OpenDmxThread() {}

    bool Stop();
    bool WriteDmx(const DmxBuffer &buffer);
    void *Run();

  private:
    int m_fd;
    string m_path;
    DmxBuffer m_buffer;
    bool m_term;
    ola::thread::Mutex m_mutex;
    ola::thread::Mutex m_term_mutex;
    ola::thread::ConditionVariable m_term_cond;

    static const int INVALID_FD = -1;
};
}  // opendmx
}  // plugin
}  // ola
#endif  // PLUGINS_OPENDMX_OPENDMXTHREAD_H_
