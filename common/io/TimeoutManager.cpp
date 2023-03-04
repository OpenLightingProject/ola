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
 * TimeoutManager.cpp
 * Manages timeout events.
 * Copyright (C) 2013 Simon Newton
 */

#include <queue>
#include <set>
#include <vector>

#include "ola/Logging.h"
#include "common/io/TimeoutManager.h"

namespace ola {
namespace io {

// Tracks the # of timer functions registered
const char TimeoutManager::K_TIMER_VAR[] = "ss-timers";

using ola::Callback0;
using ola::ExportMap;
using ola::thread::INVALID_TIMEOUT;
using ola::thread::timeout_id;

TimeoutManager::TimeoutManager(ExportMap *export_map,
                               Clock *clock)
    : m_export_map(export_map),
      m_clock(clock) {
  if (m_export_map) {
    m_export_map->GetIntegerVar(K_TIMER_VAR);
  }
}

TimeoutManager::~TimeoutManager() {
  m_removed_timeouts.clear();

  while (!m_events.empty()) {
    delete m_events.top();
    m_events.pop();
  }
}

timeout_id TimeoutManager::RegisterRepeatingTimeout(
    const TimeInterval &interval,
    ola::Callback0<bool> *closure) {
  if (!closure)
    return INVALID_TIMEOUT;

  if (m_export_map)
    (*m_export_map->GetIntegerVar(K_TIMER_VAR))++;

  Event *event = new RepeatingEvent(interval, m_clock, closure);
  m_events.push(event);
  return event;
}

timeout_id TimeoutManager::RegisterSingleTimeout(
    const TimeInterval &interval,
    ola::SingleUseCallback0<void> *closure) {
  if (!closure)
    return INVALID_TIMEOUT;

  if (m_export_map)
    (*m_export_map->GetIntegerVar(K_TIMER_VAR))++;

  Event *event = new SingleEvent(interval, m_clock, closure);
  m_events.push(event);
  return event;
}

void TimeoutManager::CancelTimeout(timeout_id id) {
  // TODO(simon): just mark the timeouts as cancelled rather than using a
  // remove set.
  if (id == INVALID_TIMEOUT)
    return;

  if (!m_removed_timeouts.insert(id).second)
    OLA_WARN << "timeout " << id << " already in remove set";
}

TimeInterval TimeoutManager::ExecuteTimeouts(TimeStamp *now) {
  Event *e;
  if (m_events.empty())
    return TimeInterval();

  // make sure we only try to access m_events.top() if m_events isn't empty
  while (!m_events.empty() && ((e = m_events.top())->NextTime() <= *now)) {
    m_events.pop();

    // if this was removed, skip it
    if (m_removed_timeouts.erase(e)) {
      delete e;
      if (m_export_map)
        (*m_export_map->GetIntegerVar(K_TIMER_VAR))--;
      continue;
    }

    if (e->Trigger()) {
      // true implies we need to run this again
      e->UpdateTime(*now);
      m_events.push(e);
    } else {
      delete e;
      if (m_export_map)
        (*m_export_map->GetIntegerVar(K_TIMER_VAR))--;
    }
    m_clock->CurrentMonotonicTime(now);
  }

  if (m_events.empty())
    return TimeInterval();
  else
    return m_events.top()->NextTime() - *now;
}
}  // namespace io
}  // namespace ola
