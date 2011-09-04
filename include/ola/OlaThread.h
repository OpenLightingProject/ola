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
 * OlaThread.h
 * A thread object.
 * Copyright (C) 2010 Simon Newton
 */

#ifndef INCLUDE_OLA_OLATHREAD_H_
#define INCLUDE_OLA_OLATHREAD_H_

#include <pthread.h>

namespace ola {

typedef pthread_t ThreadId;

/**
 * A thread object to be subclassed.
 */
class OlaThread {
  public:
    OlaThread(): m_thread_id(), m_running(false) {}
    virtual ~OlaThread() {}

    virtual bool Start();
    virtual bool Join(void *ptr = NULL);
    bool IsRunning() const { return m_running; }

    // Sub classes implement this.
    virtual void *Run() = 0;
    ThreadId Id() const { return m_thread_id; }

    static inline ThreadId Self() { return pthread_self(); }

  private:
    pthread_t m_thread_id;
    bool m_running;
};


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

    Mutex(const Mutex&);
    Mutex& operator=(const Mutex&);
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

    MutexLocker(const MutexLocker&);
    MutexLocker& operator=(const MutexLocker&);
};


/**
 * A condition variable
 */
class ConditionVariable {
  public:
    ConditionVariable();
    ~ConditionVariable();

    void Wait(Mutex *mutex);
    bool TimedWait(Mutex *mutex, struct timespec *wait_time);

    void Signal();
    void Broadcast();

  private:
    pthread_cond_t m_condition;

    ConditionVariable(const ConditionVariable&);
    ConditionVariable& operator=(const ConditionVariable&);
};
}  // ola
#endif  // INCLUDE_OLA_OLATHREAD_H_
