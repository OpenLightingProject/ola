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
 * TimeoutManager.h
 * Manages timeout events.
 * Copyright (C) 2013 Simon Newton
 */

#ifndef COMMON_IO_TIMEOUTMANAGER_H_
#define COMMON_IO_TIMEOUTMANAGER_H_

#include <queue>
#include <set>
#include <vector>

#include "ola/Callback.h"
#include "ola/Clock.h"
#include "ola/ExportMap.h"
#include "ola/base/Macro.h"
#include "ola/thread/SchedulerInterface.h"

namespace ola {
namespace io {


/**
 * @class TimeoutManager
 * @brief Manages timer events.
 *
 * The TimeoutManager allows Callbacks to trigger at some point in the future.
 * Callbacks can be invoked once, or periodically.
 */
class TimeoutManager {
 public :
  /**
   * @brief Create a new TimeoutManager.
   * @param export_map an ExportMap to update
   * @param clock the Clock to use.
   */
  TimeoutManager(ola::ExportMap *export_map, Clock *clock);

  ~TimeoutManager();

  /**
   * @brief Register a repeating timeout.
   * Returning false from the Callback will cancel this timer.
   * @param interval the delay before the closure will be run.
   * @param closure the closure to invoke when the event triggers. Ownership is
   * given up to the select server - make sure nothing else uses this Callback.
   * @returns the identifier for this timeout, this can be used to remove it
   * later.
   */
  ola::thread::timeout_id RegisterRepeatingTimeout(
      const ola::TimeInterval &interval,
      ola::Callback0<bool> *closure);

  /**
   * @brief Register a single use timeout function.
   * @param interval the delay between function calls
   * @param closure the Callback to invoke when the event triggers
   * @returns the identifier for this timeout, this can be used to remove it
   * later.
   */
  ola::thread::timeout_id RegisterSingleTimeout(
      const ola::TimeInterval &interval,
      ola::SingleUseCallback0<void> *closure);

  /**
   * @brief Cancel a timeout.
   * @param id the id of the timeout
   */
  void CancelTimeout(ola::thread::timeout_id id);

  /**
   * @brief Check if there are any events in the queue.
   * Events remain in the queue even if they have been cancelled.
   * @returns true if there are events pending, false otherwise.
   */
  bool EventsPending() const {
    return !m_events.empty();
  }

  /**
   * @brief Execute any expired timeouts.
   * @param[in,out] now the current time, set to the last time events were
   * checked.
   * @returns the time until the next event.
   */
  TimeInterval ExecuteTimeouts(TimeStamp *now);

  static const char K_TIMER_VAR[];

 private :
  class Event {
   public:
    explicit Event(const TimeInterval &interval, const Clock *clock)
        : m_interval(interval) {
      TimeStamp now;
      clock->CurrentMonotonicTime(&now);
      m_next = now + m_interval;
    }
    virtual ~Event() {}
    virtual bool Trigger() = 0;

    void UpdateTime(const TimeStamp &now) {
      m_next = now + m_interval;
    }

    TimeStamp NextTime() const { return m_next; }

   private:
    TimeInterval m_interval;
    TimeStamp m_next;
  };

  // An event that only happens once
  class SingleEvent: public Event {
   public:
    SingleEvent(const TimeInterval &interval,
                const Clock *clock,
                ola::BaseCallback0<void> *closure):
      Event(interval, clock),
      m_closure(closure) {
    }

    virtual ~SingleEvent() {
      if (m_closure)
        delete m_closure;
    }

    bool Trigger() {
      if (m_closure) {
        m_closure->Run();
        // it's deleted itself at this point
        m_closure = NULL;
      }
      return false;
    }

   private:
     ola::BaseCallback0<void> *m_closure;
  };

  /*
   * An event that occurs more than once. The closure can return false to
   * indicate that it should not be called again.
   */
  class RepeatingEvent: public Event {
   public:
    RepeatingEvent(const TimeInterval &interval,
                   const Clock *clock,
                   ola::BaseCallback0<bool> *closure):
      Event(interval, clock),
      m_closure(closure) {
    }
    ~RepeatingEvent() {
      delete m_closure;
    }
    bool Trigger() {
      if (!m_closure)
        return false;
      return m_closure->Run();
    }

   private:
    ola::BaseCallback0<bool> *m_closure;
  };

  struct ltevent {
    bool operator()(Event *e1, Event *e2) const {
      return e1->NextTime() > e2->NextTime();
    }
  };

  typedef std::priority_queue<Event*, std::vector<Event*>, ltevent>
      event_queue_t;

  ola::ExportMap *m_export_map;
  Clock *m_clock;

  event_queue_t m_events;
  std::set<ola::thread::timeout_id> m_removed_timeouts;

  DISALLOW_COPY_AND_ASSIGN(TimeoutManager);
};
}  // namespace io
}  // namespace ola
#endif  // COMMON_IO_TIMEOUTMANAGER_H_
