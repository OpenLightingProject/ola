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
 * SignalThread.cpp
 * A thread to handle signals.
 * Copyright (C) 2013 Simon Newton
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H

#include <errno.h>
#include <string.h>
#ifdef _WIN32
// On MinGW, pthread.h pulls in Windows.h, which in turn pollutes the global
// namespace. We define VC_EXTRALEAN and WIN32_LEAN_AND_MEAN to reduce this.
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#endif  // _WIN32
#include <pthread.h>
#include <signal.h>
#include <map>

#include "ola/Callback.h"
#include "ola/Logging.h"
#include "ola/stl/STLUtils.h"
#include "ola/thread/SignalThread.h"

namespace ola {
namespace thread {

#ifdef _WIN32
// Windows doesn't support sigwait and related functions, so we need to use
// this workaround. Keep the definition of g_signal_map in sync with
// SignalThread::SignalMap.
static std::map<int, SignalThread::SignalHandler*>* g_signal_map = NULL;
static void Win32SignalHandler(int signo) {
  OLA_INFO << "Received signal: " << signo;
  if (!g_signal_map) {
    OLA_WARN << "Signal handler called without signal map";
    return;
  }

  SignalThread::SignalHandler *handler = STLFindOrNull(*g_signal_map, signo);
  if (handler) {
    handler->Run();
  }
}
#endif  // _WIN32

SignalThread::SignalThread()
    : Thread(Thread::Options("signal-thread")) {}

SignalThread::~SignalThread() {
  ola::STLDeleteValues(&m_signal_handlers);
}

/**
 * @brief Install a signal handler for the given signal.
 *
 * This can't be called once the thread has stared.
 */
bool SignalThread::InstallSignalHandler(int signal, SignalHandler *handler) {
  if (BlockSignal(signal)) {
    ola::STLReplaceAndDelete(&m_signal_handlers, signal, handler);
    return true;
  }
  return false;
}

/**
 * @brief Entry point into the thread.
 */
void* SignalThread::Run() {
#ifndef _WIN32
  sigset_t signals;
  int signo;
#endif  // _WIN32

#ifdef _WIN32
  if (g_signal_map) {
    OLA_WARN << "Windows signal map was already set, it will be overwritten.";
  }
  g_signal_map = &m_signal_handlers;

  SignalMap::const_iterator iter = m_signal_handlers.begin();
  for (; iter != m_signal_handlers.end(); ++iter) {
    signal(iter->first, Win32SignalHandler);
  }
#endif  // _WIN32

  while (true) {
#ifndef _WIN32
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
#endif  // _WIN32
  }
  return NULL;
}

/**
 * @brief Add the signals we're interested in to the sigset.
 */
bool SignalThread::AddSignals(sigset_t *signals) {
#ifndef _WIN32
  SignalMap::const_iterator iter = m_signal_handlers.begin();
  for (; iter != m_signal_handlers.end(); ++iter) {
    if (sigaddset(signals, iter->first)) {
      OLA_WARN << "Failed to add " << strsignal(iter->first)
               << " to the signal set:" << strerror(errno);
      return false;
    }
  }
#else
  (void) signals;
#endif  // _WIN32
  return true;
}

/**
 * Block the signal
 */
bool SignalThread::BlockSignal(int signal) {
#ifdef _WIN32
  ::signal(signal, SIG_IGN);
#else
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
#endif  // _WIN32
  return true;
}
}  // namespace thread
}  // namespace ola
