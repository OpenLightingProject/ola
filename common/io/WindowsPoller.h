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
 * WindowsPoller.h
 * A Poller for the Windows platform
 * Copyright (C) 2014 Lukas Erlinghagen
 */

#ifndef COMMON_IO_WINDOWSPOLLER_H_
#define COMMON_IO_WINDOWSPOLLER_H_

#include <ola/Clock.h>
#include <ola/ExportMap.h>
#include <ola/base/Macro.h>
#include <ola/io/Descriptor.h>

#define WIN32_LEAN_AND_MEAN
#include <ola/win/CleanWindows.h>

#include <map>
#include <utility>
#include <vector>

#include "common/io/PollerInterface.h"
#include "common/io/TimeoutManager.h"

namespace ola {
namespace io {

/**
 * @class WindowsPoller
 * @brief An implementation of PollerInterface for Windows.
 */
class WindowsPoller : public PollerInterface {
 public :
  /**
   * @brief Create a new WindowsPoller.
   * @param export_map the ExportMap to use
   * @param clock the Clock to use
   */
  WindowsPoller(ExportMap *export_map, Clock *clock);

  ~WindowsPoller();

  bool AddReadDescriptor(class ReadFileDescriptor *descriptor);
  bool AddReadDescriptor(class ConnectedDescriptor *descriptor,
                         bool delete_on_close);
  bool RemoveReadDescriptor(class ReadFileDescriptor *descriptor);
  bool RemoveReadDescriptor(class ConnectedDescriptor *descriptor);

  bool AddWriteDescriptor(class WriteFileDescriptor *descriptor);
  bool RemoveWriteDescriptor(class WriteFileDescriptor *descriptor);

  const TimeStamp *WakeUpTime() const { return &m_wake_up_time; }

  bool Poll(TimeoutManager *timeout_manager,
            const TimeInterval &poll_interval);

 private:
  typedef std::map<void*, class WindowsPollerDescriptor*> DescriptorMap;
  typedef std::vector<class WindowsPollerDescriptor*> OrphanedDescriptors;

  ExportMap *m_export_map;
  CounterVariable *m_loop_iterations;
  CounterVariable *m_loop_time;
  Clock *m_clock;
  TimeStamp m_wake_up_time;

  DescriptorMap m_descriptor_map;
  OrphanedDescriptors m_orphaned_descriptors;

  std::pair<WindowsPollerDescriptor*, bool>
      LookupOrCreateDescriptor(void* handle);
  bool RemoveDescriptor(const DescriptorHandle &handle,
                        int flag,
                        bool warn_on_missing);

  void HandleWakeup(class PollData* data);
  void FinalCheckIOs(std::vector<class PollData*> data);

  DISALLOW_COPY_AND_ASSIGN(WindowsPoller);
};
}  // namespace io
}  // namespace ola
#endif  // COMMON_IO_WINDOWSPOLLER_H_
