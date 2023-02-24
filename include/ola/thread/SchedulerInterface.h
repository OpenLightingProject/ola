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

/**
 * @brief A timeout handle which can later be used to cancel a timeout.
 */
typedef void* timeout_id;

/**
 * @brief An invalid / uninitialized timeout_id.
 */
static const timeout_id INVALID_TIMEOUT = NULL;


/**
 * @brief Allows \ref callbacks to be scheduled to run after a specified
 *   interval.
 */
class SchedulerInterface {
 public :
  SchedulerInterface() {}
  virtual ~SchedulerInterface() {}

  /**
   * @brief Execute a callback periodically.
   * @param period the number of milliseconds between each execution of the
   *   callback.
   * @param callback the callback to run. Ownership is transferred.
   * @returns a timeout_id which can be used later to cancel the timeout.
   * @deprecated Use the version that takes a TimeInterval instead.
   *
   * Returning false from the callback will cause it to be cancelled.
   */
  virtual timeout_id RegisterRepeatingTimeout(
      unsigned int period,
      Callback0<bool> *callback) = 0;

  /**
   * @brief Execute a callback periodically.
   * @param period the time interval between each execution of the callback.
   * @param callback the callback to run. Ownership is transferred.
   * @returns a timeout_id which can be used later to cancel the timeout.
   *
   * Returning false from the callback will cause it to be cancelled.
   */
  virtual timeout_id RegisterRepeatingTimeout(
      const ola::TimeInterval &period,
      Callback0<bool> *callback) = 0;

  /**
   * @brief Execute a callback after a certain time interval.
   * @param delay the number of milliseconds before the callback is executed.
   * @param callback the callback to run. Ownership is transferred.
   * @returns a timeout_id which can be used later to cancel the timeout.
   * @deprecated Use the version that takes a TimeInterval instead.
   */
  virtual timeout_id RegisterSingleTimeout(
      unsigned int delay,
      SingleUseCallback0<void> *callback) = 0;

  /**
   * @brief Execute a callback after a certain time interval.
   * @param delay the time interval to wait before the callback is executed.
   * @param callback the callback to run. Ownership is transferred.
   * @returns a timeout_id which can be used later to cancel the timeout.
   */
  virtual timeout_id RegisterSingleTimeout(
      const ola::TimeInterval &delay,
      SingleUseCallback0<void> *callback) = 0;

  /**
   * @brief Cancel an existing timeout
   * @param id the timeout_id returned by a call to RegisterRepeatingTimeout or
   *   RegisterSingleTimeout.
   */
  virtual void RemoveTimeout(timeout_id id) = 0;
};
}  // namespace thread
}  // namespace ola
#endif  // INCLUDE_OLA_THREAD_SCHEDULERINTERFACE_H_
