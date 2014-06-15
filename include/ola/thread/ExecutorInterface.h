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
 * ExecutorInterface.h
 * The executor interface provides a single method:
 *   Execute() which takes a no-argument, void Callback and executes it at some
 *   point the in future. Scheduling details are left to the implementation.
 * Copyright (C) 2011 Simon Newton
 */

#ifndef INCLUDE_OLA_THREAD_EXECUTORINTERFACE_H_
#define INCLUDE_OLA_THREAD_EXECUTORINTERFACE_H_

#include <ola/Callback.h>

namespace ola {
namespace thread {

class ExecutorInterface {
 public :
  ExecutorInterface() {}
  virtual ~ExecutorInterface() {}

  virtual void Execute(ola::BaseCallback0<void> *closure) = 0;
};
}  // namespace thread
}  // namespace ola
#endif  // INCLUDE_OLA_THREAD_EXECUTORINTERFACE_H_
