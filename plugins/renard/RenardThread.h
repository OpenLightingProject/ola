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
 * RenardThread.h
 * Thread for the renard device
 * Copyright (C) 2013 Hakan Lindestaf
 */

#ifndef PLUGINS_RENARD_RENARDTHREAD_H_
#define PLUGINS_RENARD_RENARDTHREAD_H_

#include <string>
#include "ola/DmxBuffer.h"
#include "ola/thread/Thread.h"

namespace ola {
namespace plugin {
namespace renard {

class RenardThread: public ola::thread::Thread {
  public:
    explicit RenardThread(const string &path);
    ~RenardThread() {}

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
}  // namespace renard
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_RENARD_RENARDTHREAD_H_
