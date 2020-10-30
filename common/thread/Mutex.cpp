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
 * Mutex.cpp
 * Mutex and ConditionVariable
 * Copyright (C) 2010 Simon Newton
 */

#include <pthread.h>
#include "ola/thread/Mutex.h"

namespace ola {
namespace thread {

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
    : m_mutex(mutex),
      m_requires_unlock(true) {
  m_mutex->Lock();
}

/**
 * Destroy this MutexLocker and unlock the mutex
 */
MutexLocker::~MutexLocker() {
  Release();
}

void MutexLocker::Release() {
  if (m_requires_unlock) {
    m_mutex->Unlock();
    m_requires_unlock = false;
  }
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
 * @param wake_up_time the time to wake up. This must be an absolute i.e. real
 * time.
 * @returns true if we received a signal, false if the timeout expired.
 */
bool ConditionVariable::TimedWait(Mutex *mutex, const TimeStamp &wake_up_time) {
  struct timespec ts = {
    wake_up_time.Seconds(),
    wake_up_time.MicroSeconds() * ONE_THOUSAND
  };
  int i = pthread_cond_timedwait(&m_condition, &mutex->m_mutex, &ts);
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
}  // namespace thread
}  // namespace ola
