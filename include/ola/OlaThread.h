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
 * OlaThread.h
 * A thread object.
 * Copyright (C) 2010 Simon Newton
 */

#ifndef INCLUDE_OLA_OLATHREAD_H_
#define INCLUDE_OLA_OLATHREAD_H_

#include <pthread.h>

namespace ola {

class OlaThread {
  public:
    OlaThread(): m_thread_id(), m_running(false) {}
    virtual ~OlaThread() {}

    virtual bool Start();
    virtual bool Join(void *ptr = NULL);
    bool IsRunning() const { return m_running; }
    virtual void *Run() = 0;

  private:
    pthread_t m_thread_id;
    bool m_running;
};
}  // ola
#endif  // INCLUDE_OLA_OLATHREAD_H_
