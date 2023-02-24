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
 * Mutex.h
 * A thread object.
 * Copyright (C) 2010 Simon Newton
 */

#ifndef INCLUDE_OLA_THREAD_MUTEX_H_
#define INCLUDE_OLA_THREAD_MUTEX_H_

#ifdef _WIN32
// On MinGW, pthread.h pulls in Windows.h, which in turn pollutes the global
// namespace. We define VC_EXTRALEAN and WIN32_LEAN_AND_MEAN to reduce this.
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#endif  // _WIN32
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

    void Release();

 private:
    Mutex *m_mutex;
    bool m_requires_unlock;

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
