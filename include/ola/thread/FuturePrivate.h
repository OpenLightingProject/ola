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
 * FuturePrivate.h
 * An experimental implementation of Futures.
 * Copyright (C) 2010 Simon Newton
 */

#ifndef INCLUDE_OLA_THREAD_FUTUREPRIVATE_H_
#define INCLUDE_OLA_THREAD_FUTUREPRIVATE_H_

#include <ola/Logging.h>
#include <ola/base/Macro.h>
#include <ola/thread/Mutex.h>

namespace ola {
namespace thread {

template <typename T>
class FutureImpl {
 public:
    FutureImpl()
        : m_ref_count(1),
          m_is_set(false),
          m_value() {
    }

    void Ref() {
      {
        MutexLocker l(&m_mutex);
        m_ref_count++;
      }
    };

    void DeRef() {
      unsigned int ref_count = 0;
      {
        MutexLocker l(&m_mutex);
        ref_count = --m_ref_count;
      }
      if (ref_count == 0) {
        delete this;
      }
    }

    bool IsComplete() const {
      MutexLocker l(&m_mutex);
      return m_is_set;
    }

    const T& Get() const {
      MutexLocker l(&m_mutex);
      if (m_is_set) {
        return m_value;
      }
      m_condition.Wait(&m_mutex);
      return m_value;
    }

    void Set(const T &t) {
      {
        MutexLocker l(&m_mutex);
        if (m_is_set) {
          OLA_FATAL << "Double call to FutureImpl::Set()";
          return;
        }
        m_is_set = true;
        m_value = t;
      }
      m_condition.Broadcast();
    }

 private:
    mutable Mutex m_mutex;
    mutable ConditionVariable m_condition;
    unsigned int m_ref_count;
    bool m_is_set;
    T m_value;

    DISALLOW_COPY_AND_ASSIGN(FutureImpl<T>);
};

/**
 * The void specialization
 */
template <>
class FutureImpl<void> {
 public:
    FutureImpl()
        : m_ref_count(1),
          m_is_set(false) {
    }

    void Ref() {
      {
        MutexLocker l(&m_mutex);
        m_ref_count++;
      }
    };

    void DeRef() {
      unsigned int ref_count = 0;
      {
        MutexLocker l(&m_mutex);
        ref_count = --m_ref_count;
      }
      if (ref_count == 0) {
        delete this;
      }
    }

    bool IsComplete() const {
      MutexLocker l(&m_mutex);
      return m_is_set;
    }

    void Get() const {
      MutexLocker l(&m_mutex);
      if (m_is_set) {
        return;
      }
      m_condition.Wait(&m_mutex);
    }

    void Set() {
      {
        MutexLocker l(&m_mutex);
        if (m_is_set) {
          OLA_FATAL << "Double call to FutureImpl::Set()";
          return;
        }
        m_is_set = true;
      }
      m_condition.Broadcast();
    }

 private:
    mutable Mutex m_mutex;
    mutable ConditionVariable m_condition;
    unsigned int m_ref_count;
    bool m_is_set;

    DISALLOW_COPY_AND_ASSIGN(FutureImpl<void>);
};

}  // namespace thread
}  // namespace ola
#endif  // INCLUDE_OLA_THREAD_FUTUREPRIVATE_H_
