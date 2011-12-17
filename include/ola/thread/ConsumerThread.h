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
 * ConsumerThread.h
 * An thread which consumes Callbacks from a queue and runs them.
 * Copyright (C) 2011 Simon Newton
 */

#ifndef INCLUDE_OLA_THREAD_CONSUMERTHREAD_H_
#define INCLUDE_OLA_THREAD_CONSUMERTHREAD_H_

#include <ola/Callback.h>
#include <ola/OlaThread.h>
#include <queue>

namespace ola {
namespace thread {

using std::queue;

/**
 * A thread which waits on a queue, and when actions (callbacks) become
 * available, it pulls them from the queue and executes them.
 */
class ConsumerThread: public ola::OlaThread {
  public :
    typedef BaseCallback0<void>* Action;
    /**
     * @param callback_queue the queue to pull actions from
     * @param shutdown a bool which is set to true if this thread is to finish.
     * @param mutex the Mutex object which protects the queue and shutdown
     *   variable.
     * @param condition_var the ConditionVariable to wait on. This signals when
     *   the queue is non-empty, or shutdown changes to true.
     */
    ConsumerThread(queue<Action> *callback_queue,
                   const bool *shutdown,
                   Mutex *mutex,
                   ConditionVariable *condition_var)
        : OlaThread(),
          m_callback_queue(callback_queue),
          m_shutdown(shutdown),
          m_mutex(mutex),
          m_condition_var(condition_var) {
    }
    ~ConsumerThread() {}
    void *Run();

  private:
    queue<Action> *m_callback_queue;
    const bool *m_shutdown;
    Mutex *m_mutex;
    ConditionVariable *m_condition_var;

    void EmptyQueue();
};
}  // thread
}  // ola
#endif  // INCLUDE_OLA_THREAD_CONSUMERTHREAD_H_
