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
 * OVDmxThread.h
 * Thread for the OVDMX device
 * Copyright (C) 2005 Simon Newton, 2017 Jan Ove Saltvedt
 */

#ifndef PLUGINS_OVDMX_OVDMXTHREAD_H_
#define PLUGINS_OVDMX_OVDMXTHREAD_H_

#include <string>
#include "ola/DmxBuffer.h"
#include "ola/thread/Thread.h"

namespace ola {
namespace plugin {
namespace ovdmx {

class OVDmxThread: public ola::thread::Thread {
 public:
    explicit OVDmxThread(const std::string &path);
    ~OVDmxThread() {}

    bool Stop();
    bool WriteDmx(const DmxBuffer &buffer);
    void *Run();

 private:
    void MakeRaw(int fd);
    int m_fd;
    std::string m_path;
    DmxBuffer m_buffer;
    bool m_term;
    ola::thread::Mutex m_mutex;
    ola::thread::Mutex m_term_mutex;
    ola::thread::ConditionVariable m_term_cond;

    static const int INVALID_FD = -1;
};
}  // namespace ovdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_OVDMX_OVDMXTHREAD_H_
