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
#include <Windows.h>

#include <map>
#include <set>

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
  typedef struct {
    ConnectedDescriptor *descriptor;
    bool delete_on_close;
  } connected_descriptor_t;

  struct connected_descriptor_t_lt {
    bool operator()(const connected_descriptor_t &c1,
                    const connected_descriptor_t &c2) const {
      return c1.descriptor->ReadDescriptor().m_handle.m_handle <
          c2.descriptor->ReadDescriptor().m_handle.m_handle;
    }
  };

  typedef struct {
    OVERLAPPED m_overlapped;
  } overlapped_handle_context_t;

  typedef std::set<connected_descriptor_t, connected_descriptor_t_lt>
      ConnectedDescriptorSet;
  typedef std::set<ReadFileDescriptor*> SocketDescriptorSet;
  typedef std::set<WriteFileDescriptor*> SocketWriteDescriptorSet;
  typedef std::map<void*, overlapped_handle_context_t> OverlappedHandleMap;
  typedef std::map<ReadFileDescriptor*, HANDLE> HandleMap;
  typedef std::map<WriteFileDescriptor*, HANDLE> WriteHandleMap;

  ExportMap *m_export_map;
  CounterVariable *m_loop_iterations;
  CounterVariable *m_loop_time;
  Clock *m_clock;
  TimeStamp m_wake_up_time;

  SocketDescriptorSet m_socket_read_descriptors;
  SocketWriteDescriptorSet m_socket_write_descriptors;
  ConnectedDescriptorSet m_connected_read_descriptors;
  OverlappedHandleMap m_overlapped_handle_map;
  HandleMap m_socket_handles;
  WriteHandleMap m_socket_write_handles;

  void UpdateDescriptorData();

  DISALLOW_COPY_AND_ASSIGN(WindowsPoller);
};
}  // namespace io
}  // namespace ola
#endif  // COMMON_IO_WINDOWSPOLLER_H_
