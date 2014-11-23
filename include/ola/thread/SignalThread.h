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
 * SignalThread.h
 * A thread to handle signals.
 * Copyright (C) 2013 Simon Newton
 */

#ifndef INCLUDE_OLA_THREAD_SIGNALTHREAD_H_
#define INCLUDE_OLA_THREAD_SIGNALTHREAD_H_

#include <signal.h>
#include <ola/Callback.h>
#include <ola/base/Macro.h>
#include <ola/thread/Thread.h>
#include <map>

namespace ola {
namespace thread {

/**
 * Signals and threads don't play nicely together. Consider the following:
 *  - thread1,  acquires mutex M
 *  - signal triggers
 *  - thread1 runs the signal handler
 *  - signal handler attempts to aqcuire mutex M
 *  - deadlock...
 *
 * The recommended way to deal with this is to run a separate thread, whose
 * sole purpose is to wait for signals. See section 12.8 of APUE.
 *
 * To avoid this we do the following:
 *  - mask all signals that we define custom handlers for
 *  - spawn a thread which uses sigwait() to capture signals
 */
class SignalThread : public ola::thread::Thread {
 public:
    typedef ola::Callback0<void> SignalHandler;

    SignalThread();
    ~SignalThread();

    // This has to be called before Start(). You can't add signal handlers once
    // the thread is running.
    bool InstallSignalHandler(int signal, SignalHandler *handler);

 protected:
    void* Run();

 private:
    typedef std::map<int, SignalHandler*> SignalMap;

    SignalMap m_signal_handlers;

    bool AddSignals(sigset_t *signals);
    bool BlockSignal(int signal);

    DISALLOW_COPY_AND_ASSIGN(SignalThread);
};
}  // namespace thread
}  // namespace ola
#endif  // INCLUDE_OLA_THREAD_SIGNALTHREAD_H_
