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
 * Watchdog.h
 * Copyright (C) 2015 Simon Newton
 */

#ifndef INCLUDE_OLA_UTIL_WATCHDOG_H_
#define INCLUDE_OLA_UTIL_WATCHDOG_H_

#include <ola/Callback.h>
#include <ola/thread/Mutex.h>
#include <memory>

namespace ola {

/**
 * @brief Detects if an operation stalls.
 *
 * The Watchdog can be used to detect when a operation has stalled. When
 * enabled, the Clock() method should be called at regular intervals. While the
 * operation is making forward progress, it should kick the watchdog by
 * calling Kick().
 *
 * If Kick() isn't called for the specified number of clock cycles, the reset
 * callback will be triggered.
 *
 * Once a watchdog has fired, it must be Disabled() and Enabled() again to
 * reset it.
 *
 * This class is thread safe.
 * See also https://en.wikipedia.org/wiki/Watchdog_timer
 *
 * @examplepara
 *
 * One way (not recommended) of using this would be:
 *
 * ~~~~~~~~~~~~~~~~~~~~~
 * // Call periodically
 * void WatchdogThread() {
 *   while (true) {
 *     watchdog->Clock();
 *     sleep(1);
 *   }
 * }
 *
 * void WorkerThread() {
 *   watchdog->Enable();
 *   // Start operation.
 *   while (true) {
 *     DoWorkSlice();
 *     watchdog->Kick();
 *   }
 *   watchdog->Disable();
 * }
 * ~~~~~~~~~~~~~~~~~~~~~
 */
class Watchdog {
 public:
  /**
   * @brief Create a new Watchdog.
   * @param cycle_limit The number of cycles allowed before the reset callback
   *   is run.
   * @param reset_callback the callback to run when the cycle limit is reached.
   */
  Watchdog(unsigned int cycle_limit, Callback0<void> *reset_callback);

  /**
   * @brief Enable the watchdog.
   */
  void Enable();

  /**
   * @brief Disable the watchdog.
   */
  void Disable();

  /**
   * @brief Kick the watchdog to avoid a reset.
   *
   * This should be called regularly while the watchdog is enabled.
   */
  void Kick();

  /**
   * @brief Check if the process has stalled due to a lack of Kick() calls.
   *
   * This should be called regularly. If it's been more than the specified
   * number of clock cycles since a kick, the callback is run.
   */
  void Clock();

 private:
  const unsigned int m_limit;
  std::auto_ptr<Callback0<void> > m_callback;
  ola::thread::Mutex m_mu;
  bool m_enabled;  // GUARDED_BY(m_mu);
  unsigned int m_count;  // GUARDED_BY(m_mu);
  bool m_fired;  // GUARDED_BY(m_mu);
};
}  // namespace ola
#endif  // INCLUDE_OLA_UTIL_WATCHDOG_H_
