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
 * Watchdog.cpp
 * Copyright (C) 2015 Simon Newton
 */

#include "ola/util/Watchdog.h"

namespace ola {

using ola::thread::MutexLocker;

Watchdog::Watchdog(unsigned int cycle_limit, Callback0<void> *reset_callback)
    : m_limit(cycle_limit),
      m_callback(reset_callback),
      m_enabled(false),
      m_count(0),
      m_fired(false) {
}

void Watchdog::Enable() {
  MutexLocker lock(&m_mu);
  m_count = 0;
  m_fired = false;
  m_enabled = true;
}

void Watchdog::Disable() {
  MutexLocker lock(&m_mu);
  m_enabled = false;
  m_fired = false;
}

void Watchdog::Kick() {
  MutexLocker lock(&m_mu);
  m_count = 0;
}

void Watchdog::Clock() {
  bool run_callback = false;

  {
    MutexLocker lock(&m_mu);
    if (!m_enabled) {
      return;
    }
    m_count++;
    if (m_count >= m_limit && !m_fired) {
      m_fired = true;
      run_callback = true;
    }
  }
  if (run_callback) {
    m_callback->Run();
  }
}
}  // namespace ola
