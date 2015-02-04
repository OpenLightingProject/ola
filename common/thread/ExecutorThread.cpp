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
 * ExecutorThread.cpp
 * Run callbacks in a separate thread.
 * Copyright (C) 2015 Simon Newton
 */

#include "ola/thread/ExecutorThread.h"

#include <ola/io/SelectServer.h>
#include <ola/thread/Future.h>
#include <ola/thread/Thread.h>

#include <memory>

namespace ola {
namespace thread {

using ola::io::SelectServer;
using ola::thread::Future;
using ola::thread::Thread;

namespace {
void SetFuture(Future<void>* f) {
  f->Set();
}
}  // namespace

void ExecutorThread::Execute(ola::BaseCallback0<void> *callback) {
  {
    MutexLocker locker(&m_mutex);
    m_callback_queue.push(callback);
  }
  m_condition_var.Signal();
}

ExecutorThread::~ExecutorThread() {
  RunRemaining();
}

void ExecutorThread::DrainCallbacks() {
  Future<void> f;
  Execute(NewSingleCallback<void>(SetFuture, &f));
  f.Get();
}

bool ExecutorThread::Start() {
  return m_thread.Start();
}

bool ExecutorThread::Stop() {
  {
    MutexLocker locker(&m_mutex);
    m_shutdown = true;
  }
  m_condition_var.Signal();
  bool ok = m_thread.Join();

  RunRemaining();
  return ok;
}

void ExecutorThread::RunRemaining() {
  while (!m_callback_queue.empty()) {
    BaseCallback0<void>* cb = m_callback_queue.front();
    m_callback_queue.pop();
    cb->Run();
  }
}

}  // namespace thread
}  // namespace ola
