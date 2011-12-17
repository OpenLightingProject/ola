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
 * Thread.cpp
 * A simple thread class
 * Copyright (C) 2010 Simon Newton
 */

#include <pthread.h>
#include "ola/Logging.h"
#include "ola/thread/Thread.h"

namespace ola {
namespace thread {

/*
 * Called by the new thread
 */
void *StartThread(void *d) {
  Thread *thread = static_cast<Thread*>(d);
  return thread->Run();
}


/*
 * Start this thread
 */
bool Thread::Start() {
  int ret = pthread_create(&m_thread_id,
                           NULL,
                           ola::thread::StartThread,
                           static_cast<void*>(this));
  if (ret) {
    OLA_WARN << "pthread create failed";
    return false;
  }
  m_running = true;
  return true;
}


/*
 * Join this thread
 */
bool Thread::Join(void *ptr) {
  if (m_running) {
    int ret = pthread_join(m_thread_id, &ptr);
    m_running = false;
    return 0 == ret;
  }
  return false;
}


/**
 * Construct a new mutex object
 */
Mutex::Mutex() {
  pthread_mutex_init(&m_mutex, NULL);
}


/**
 * Clean up
 */
Mutex::~Mutex() {
  pthread_mutex_destroy(&m_mutex);
}


/**
 * Lock this mutex
 */
void Mutex::Lock() {
  pthread_mutex_lock(&m_mutex);
}


/**
 * Try and lock this mutex
 * @return true if we got the lock, false otherwise
 */
bool Mutex::TryLock() {
  int i = pthread_mutex_trylock(&m_mutex);
  return i == 0;
}


/**
 * Unlock this mutex
 */
void Mutex::Unlock() {
  pthread_mutex_unlock(&m_mutex);
}


/**
 * Create a new MutexLocker and lock the mutex.
 */
MutexLocker::MutexLocker(Mutex *mutex)
    : m_mutex(mutex) {
  m_mutex->Lock();
}

/**
 * Destroy this MutexLocker and unlock the mutex
 */
MutexLocker::~MutexLocker() {
  m_mutex->Unlock();
}


/**
 * New ConditionVariable
 */
ConditionVariable::ConditionVariable() {
  pthread_cond_init(&m_condition, NULL);
}


/**
 * Clean up
 */
ConditionVariable::~ConditionVariable() {
  pthread_cond_destroy(&m_condition);
}


/**
 * Wait on a condition variable
 * @param mutex the mutex that is locked
 */
void ConditionVariable::Wait(Mutex *mutex) {
  pthread_cond_wait(&m_condition, &mutex->m_mutex);
}


/**
 * Timed Wait
 * @param mutex the mutex that is locked
 * @param wait_time the time to sleep
 * @returns true if we received a signal, false if the timeout expired.
 */
bool ConditionVariable::TimedWait(Mutex *mutex, struct timespec *wait_time) {
  int i = pthread_cond_timedwait(&m_condition, &mutex->m_mutex, wait_time);
  return i == 0;
}


/**
 * Wake up a single listener
 */
void ConditionVariable::Signal() {
  pthread_cond_signal(&m_condition);
}


/**
 * Wake up all listeners
 */
void ConditionVariable::Broadcast() {
  pthread_cond_broadcast(&m_condition);
}
}  // thread
}  // ola
