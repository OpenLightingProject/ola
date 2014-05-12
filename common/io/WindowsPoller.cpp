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
 * WindowsPoller.h
 * A Poller for the Windows platform
 * Copyright (C) 2014 Lukas Erlinghagen
 */

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <Winsock2.h>

#include "common/io/WindowsPoller.h"

#include <string.h>
#include <errno.h>

#include <algorithm>
#include <queue>
#include <string>

#include "ola/Clock.h"
#include "ola/Logging.h"
#include "ola/base/Macro.h"
#include "ola/io/Descriptor.h"
#include "ola/stl/STLUtils.h"

namespace ola {
namespace io {

WindowsPoller::WindowsPoller(ExportMap *export_map, Clock* clock)
    : m_export_map(export_map),
      m_loop_iterations(NULL),
      m_loop_time(NULL),
      m_clock(clock) {
  if (m_export_map) {
    m_loop_time = m_export_map->GetCounterVar(K_LOOP_TIME);
    m_loop_iterations = m_export_map->GetCounterVar(K_LOOP_COUNT);
  }
}

WindowsPoller::~WindowsPoller() {
}

bool WindowsPoller::AddReadDescriptor(class ReadFileDescriptor *descriptor) {
  return false;
}

bool WindowsPoller::AddReadDescriptor(class ConnectedDescriptor *descriptor,
                                     bool delete_on_close) {
  return false;
}

bool WindowsPoller::RemoveReadDescriptor(class ReadFileDescriptor *descriptor) {
  return false;
}

bool WindowsPoller::RemoveReadDescriptor(class ConnectedDescriptor *descriptor) {
  return false;
}

bool WindowsPoller::AddWriteDescriptor(class WriteFileDescriptor *descriptor) {
  return false;
}

bool WindowsPoller::RemoveWriteDescriptor(
    class WriteFileDescriptor *descriptor) {
  return false;
}

bool WindowsPoller::Poll(TimeoutManager *timeout_manager,
                        const TimeInterval &poll_interval) {
  return false;
}
}  // namespace io
}  // namespace ola
