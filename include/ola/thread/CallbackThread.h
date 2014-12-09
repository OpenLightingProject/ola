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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * CallbackThread.h
 * A thread which executes a Callback.
 * Copyright (C) 2013 Simon Newton
 */

#ifndef INCLUDE_OLA_THREAD_CALLBACKTHREAD_H_
#define INCLUDE_OLA_THREAD_CALLBACKTHREAD_H_

#include <ola/Callback.h>
#include <ola/base/Macro.h>
#include <ola/thread/Thread.h>

#include <string>

namespace ola {
namespace thread {

/**
 * @brief A thread which executes a Callback.
 */
class CallbackThread : public Thread {
 public:
    typedef SingleUseCallback0<void> VoidThreadCallback;

    /**
     * Create a new CallbackThread.
     * @param callback the callback to run in the new thread.
     * @param options the thread's options.
     */
    explicit CallbackThread(VoidThreadCallback *callback,
                            const Options &options = Options())
        : Thread(options),
          m_callback(callback) {
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

    DISALLOW_COPY_AND_ASSIGN(CallbackThread);
};
}  // namespace thread
}  // namespace ola
#endif  // INCLUDE_OLA_THREAD_CALLBACKTHREAD_H_
