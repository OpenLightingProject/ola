/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * CallbackThread.h
 * A thread which executes a Callback.
 * Copyright (C) 2013 Simon Newton
 */

#ifndef INCLUDE_OLA_THREAD_CALLBACKTHREAD_H_
#define INCLUDE_OLA_THREAD_CALLBACKTHREAD_H_

#include <ola/Callback.h>
#include <ola/thread/Thread.h>

namespace ola {
namespace thread {

/**
 * @class A thread which executes a Callback.
 */
class CallbackThread : public Thread {
  public:
    typedef SingleUseCallback0<void> VoidThreadCallback;

    /**
     * Create a new CallbackThread.
     * @param callback the callback to run in the new thread.
     */
    explicit CallbackThread(VoidThreadCallback *callback)
        : m_callback(callback) {
    }

  protected:
    void *Run() {
      if (m_callback) {
        m_callback->Run();
      }
      return NULL;
    }

  private:
    VoidThreadCallback *m_callback;

    DISALLOW_COPY_AND_ASSIGN(Thread);
};
}  // namespace thread
}  // namespace ola
#endif  // INCLUDE_OLA_THREAD_CALLBACKTHREAD_H_
