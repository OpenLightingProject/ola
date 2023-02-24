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
 * KarateThread.h
 * Thread for the karate dmx device
 * Copyright (C) 2005 Simon Newton
 */

#ifndef PLUGINS_KARATE_KARATETHREAD_H_
#define PLUGINS_KARATE_KARATETHREAD_H_

#include <string>
#include "ola/DmxBuffer.h"
#include "ola/thread/Thread.h"

namespace ola {
namespace plugin {
namespace karate {

class KarateThread: public ola::thread::Thread {
 public:
    explicit KarateThread(const std::string &path);

    bool Stop();
    bool WriteDmx(const DmxBuffer &buffer);
    void *Run();

 private:
    std::string m_path;
    DmxBuffer m_buffer;
    bool m_term;
    ola::thread::Mutex m_mutex;
    ola::thread::Mutex m_term_mutex;
    ola::thread::ConditionVariable m_term_cond;
};
}  // namespace karate
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_KARATE_KARATETHREAD_H_
