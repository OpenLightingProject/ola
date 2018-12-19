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
 * FtdiDmxThread.h
 * The FTDI usb chipset DMX plugin for ola
 * Copyright (C) 2011 Rui Barreiros
 *
 * Additional modifications to enable support for multiple outputs and
 * additional device ids did change the original structure.
 *
 * by E.S. Rosenberg a.k.a. Keeper of the Keys 5774/2014
 */

#ifndef PLUGINS_FTDIDMX_FTDIDMXTHREAD_H_
#define PLUGINS_FTDIDMX_FTDIDMXTHREAD_H_

#include <queue>
#include <utility>

#include "ola/DmxBuffer.h"
#include "ola/thread/Thread.h"
#include "ola/rdm/RDMCommand.h"


namespace ola {
namespace plugin {
namespace ftdidmx {

class FtdiDmxThread : public ola::thread::Thread {
 public:
    FtdiDmxThread(FtdiInterface *interface, unsigned int frequency);
    ~FtdiDmxThread();

    bool Stop();
    void *Run();
    bool WriteDMX(const DmxBuffer &buffer);
    void SendRDMRequest(ola::rdm::RDMRequest *request,
                        ola::rdm::RDMCallback *callback);

 private:
    enum TimerGranularity { UNKNOWN, GOOD, BAD };

    TimerGranularity m_granularity;
    FtdiInterface *m_interface;
    bool m_term;
    unsigned int m_frequency;

    DmxBuffer m_buffer;
    ola::thread::Mutex m_term_mutex;
    ola::thread::Mutex m_buffer_mutex;

    std::queue<std::pair<ola::rdm::RDMRequest *,
               ola::rdm::RDMCallback *>> m_RDMQueue;

    void CheckTimeGranularity();

    static const uint32_t DMX_MAB = 16;
    static const uint32_t DMX_BREAK = 110;
    static const uint32_t BAD_GRANULARITY_LIMIT = 3;
};
}  // namespace ftdidmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_FTDIDMX_FTDIDMXTHREAD_H_
