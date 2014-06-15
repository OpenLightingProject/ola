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
 * SchedulerInterface.h
 * The scheduler interface provides methods to schedule the execution of
 * callbacks at some point in the future.
 * Copyright (C) 2011 Simon Newton
 */

#ifndef INCLUDE_OLA_THREAD_SCHEDULERINTERFACE_H_
#define INCLUDE_OLA_THREAD_SCHEDULERINTERFACE_H_

#include <ola/Callback.h>
#include <ola/Clock.h>

namespace ola {
namespace thread {

typedef void* timeout_id;
static const timeout_id INVALID_TIMEOUT = NULL;


class SchedulerInterface {
 public :
  SchedulerInterface() {}
  virtual ~SchedulerInterface() {}

  virtual timeout_id RegisterRepeatingTimeout(
      unsigned int ms,
      Callback0<bool> *closure) = 0;
  virtual timeout_id RegisterRepeatingTimeout(
      const ola::TimeInterval &interval,
      Callback0<bool> *closure) = 0;

  virtual timeout_id RegisterSingleTimeout(
      unsigned int ms,
      SingleUseCallback0<void> *closure) = 0;
  virtual timeout_id RegisterSingleTimeout(
      const ola::TimeInterval &interval,
      SingleUseCallback0<void> *closure) = 0;

  virtual void RemoveTimeout(timeout_id id) = 0;
};
}  // namespace thread
}  // namespace ola
#endif  // INCLUDE_OLA_THREAD_SCHEDULERINTERFACE_H_
