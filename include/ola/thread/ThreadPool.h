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
 * ThreadPool.h
 * An executor which farms work out to a bunch of threads.
 * Copyright (C) 2011 Simon Newton
 */

#ifndef INCLUDE_OLA_THREAD_THREADPOOL_H_
#define INCLUDE_OLA_THREAD_THREADPOOL_H_

#include <ola/OlaThread.h>
#include <ola/Callback.h>
#include <ola/thread/ConsumerThread.h>
#include <queue>
#include <vector>

namespace ola {
namespace thread {

class ThreadPool {
  public :
    typedef ola::BaseCallback0<void>* Action;

    explicit ThreadPool(unsigned int thread_count)
        : m_thread_count(thread_count),
          m_shutdown(false) {
    }
    ~ThreadPool();
    bool Init();
    void JoinAll();
    void Execute(Action action);

  private:
    queue<Action> m_callback_queue;
    unsigned int m_thread_count;
    bool m_shutdown;
    ola::Mutex m_mutex;
    ola::ConditionVariable m_condition_var;
    std::vector<ConsumerThread*> m_threads;

    void JoinAllThreads();
};
}  // thread
}  // ola
#endif  // INCLUDE_OLA_THREAD_THREADPOOL_H_
