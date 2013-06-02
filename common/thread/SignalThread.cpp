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
 * SignalThread.cpp
 * A thread to handle signals.
 * Copyright (C) 2013 Simon Newton
 */

#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <map>

#include "ola/Callback.h"
#include "ola/Logging.h"
#include "ola/stl/STLUtils.h"
#include "ola/thread/SignalThread.h"

namespace ola {
namespace thread {

SignalThread::~SignalThread() {
  ola::STLDeleteValues(&m_signal_handlers);
}

/**
 * Install a signal handler for the given signal. This can't be called once the
 * thread has stared.
 */
bool SignalThread::InstallSignalHandler(int signal, SignalHandler *handler) {
  if (BlockSignal(signal)) {
    ola::STLReplaceAndDelete(&m_signal_handlers, signal, handler);
    return true;
  }
  return false;
}

/**
 * Entry point into the thread.
 */
void* SignalThread::Run() {
  sigset_t signals;
  int signo;

  while (true) {
    sigemptyset(&signals);
    AddSignals(&signals);
    // Don't try to use sigpending here. It won't work on Mac.
    if (sigwait(&signals, &signo) != 0) {
      OLA_INFO << "sigwait error: " << strerror(errno);
      continue;
    }

    OLA_INFO << "Received signal: " << strsignal(signo);
    SignalHandler *handler = STLFindOrNull(m_signal_handlers, signo);
    if (handler) {
      handler->Run();
    }
  }
  return NULL;
}

/**
 * Add the signals we're interested in to the sigset.
 */
bool SignalThread::AddSignals(sigset_t *signals) {
  SignalMap::const_iterator iter = m_signal_handlers.begin();
  for (; iter != m_signal_handlers.end(); ++iter) {
    if (sigaddset(signals, iter->first)) {
      OLA_WARN << "Failed to add " << strsignal(iter->first)
               << " to the signal set:" << strerror(errno);
      return false;
    }
  }
  return true;
}

/**
 * Block the signal
 */
bool SignalThread::BlockSignal(int signal) {
  sigset_t signals;
  if (sigemptyset(&signals)) {
    OLA_WARN << "Failed to init signal set: " << strerror(errno);
    return false;
  }

  if (sigaddset(&signals, signal)) {
    OLA_WARN << "Failed to add " << strsignal(signal)
             << " to the signal set:" << strerror(errno);
    return false;
  }

  if (pthread_sigmask(SIG_BLOCK, &signals, NULL)) {
    OLA_WARN << "Failed to block signals: " << strerror(errno);
    return false;
  }
  return true;
}
}  // namespace thread
}  // namespace ola
