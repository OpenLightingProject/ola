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
 * Mutex.h
 * A thread object.
 * Copyright (C) 2010 Simon Newton
 */

#ifndef INCLUDE_OLA_THREAD_MUTEX_H_
#define INCLUDE_OLA_THREAD_MUTEX_H_

#include <pthread.h>
#include <ola/Clock.h>
#include <ola/base/Macro.h>

namespace ola {
namespace thread {


/**
 * A Mutex object
 */
class Mutex {
 public:
    friend class ConditionVariable;

    Mutex();
    ~Mutex();

    void Lock();
    bool TryLock();
    void Unlock();

 private:
    pthread_mutex_t m_mutex;

    DISALLOW_COPY_AND_ASSIGN(Mutex);
};


/**
 * A convenience class to lock mutexes. The mutex is unlocked when this object
 * is destroyed.
 */
class MutexLocker {
 public:
    explicit MutexLocker(Mutex *mutex);
    ~MutexLocker();

 private:
    Mutex *m_mutex;

    DISALLOW_COPY_AND_ASSIGN(MutexLocker);
};


/**
 * A condition variable
 */
class ConditionVariable {
 public:
    ConditionVariable();
    ~ConditionVariable();

    void Wait(Mutex *mutex);
    bool TimedWait(Mutex *mutex, const ola::TimeStamp &wake_up_time);

    void Signal();
    void Broadcast();

 private:
    pthread_cond_t m_condition;

    DISALLOW_COPY_AND_ASSIGN(ConditionVariable);
};
}  // namespace thread
}  // namespace ola
#endif  // INCLUDE_OLA_THREAD_MUTEX_H_
