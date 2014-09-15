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
 * Future.h
 * An experimental implementation of Futures.
 * Copyright (C) 2010 Simon Newton
 */

#ifndef INCLUDE_OLA_THREAD_FUTURE_H_
#define INCLUDE_OLA_THREAD_FUTURE_H_

#include <ola/thread/FuturePrivate.h>

namespace ola {
namespace thread {

/**
 * A Future object
 */
template <typename T>
class Future {
 public:
    Future() : m_impl(new FutureImpl<T>()) {}

    ~Future() {
      m_impl->DeRef();
    }

    Future(const Future &other) : m_impl(other.m_impl) {
      other.m_impl->Ref();
    }

    Future& operator=(const Future<T> &other) {
      if (this != &other) {
        m_impl->DeRef();
        other.m_impl->Ref();
        m_impl = other.m_impl;
      }
      return *this;
    }

    bool IsComplete() const {
      return m_impl->IsComplete();
    }

    const T& Get() {
      return m_impl->Get();
    }

    void Set(const T &t) {
      m_impl->Set(t);
    }

 private:
    class FutureImpl<T> *m_impl;
};

/**
 * The void specialization.
 */
template <>
class Future<void> {
 public:
    Future() : m_impl(new FutureImpl<void>()) {}

    ~Future() {
      m_impl->DeRef();
    }

    Future(const Future &other) : m_impl(other.m_impl) {
      other.m_impl->Ref();
    }

    Future& operator=(const Future<void> &other) {
      if (this != &other) {
        m_impl->DeRef();
        other.m_impl->Ref();
        m_impl = other.m_impl;
      }
      return *this;
    }

    bool IsComplete() const {
      return m_impl->IsComplete();
    }

    void Get() {
      m_impl->Get();
    }

    void Set() {
      m_impl->Set();
    }

 private:
    class FutureImpl<void> *m_impl;
};
}  // namespace thread
}  // namespace ola
#endif  // INCLUDE_OLA_THREAD_FUTURE_H_
