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
 * Copyright (C) 2011 Simon Newton
 */

#ifndef INCLUDE_OLA_THREAD_EXECUTORINTERFACE_H_
#define INCLUDE_OLA_THREAD_EXECUTORINTERFACE_H_

#include <ola/Callback.h>

namespace ola {
namespace thread {

/**
 * @brief Defer execution of a \ref callbacks "callback".
 *
 * The executor interface provides a method to defer execution of a callback.
 * Often we want to break the call stack, say if we need to run a callback that
 * modify / delete a non-reentrant object in our call stack.
 */
class ExecutorInterface {
 public :
  ExecutorInterface() {}
  virtual ~ExecutorInterface() {}

  /**
   * @brief Execute the supplied callback at some point in the future.
   * @param callback the callback to run.
   *
   * This method guarantees the callback will not be run immediately.
   */
  virtual void Execute(ola::BaseCallback0<void> *callback) = 0;
};
}  // namespace thread
}  // namespace ola
#endif  // INCLUDE_OLA_THREAD_EXECUTORINTERFACE_H_
