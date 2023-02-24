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
 * SelectPoller.h
 * A Poller which uses select()
 * Copyright (C) 2013 Simon Newton
 */

#ifndef COMMON_IO_SELECTPOLLER_H_
#define COMMON_IO_SELECTPOLLER_H_

#include <ola/Clock.h>
#include <ola/ExportMap.h>
#include <ola/base/Macro.h>
#include <ola/io/Descriptor.h>

#include <map>
#include <set>

#include "common/io/PollerInterface.h"
#include "common/io/TimeoutManager.h"

namespace ola {
namespace io {

/**
 * @class SelectPoller
 * @brief An implementation of PollerInterface that uses select().
 *
 */
class SelectPoller : public PollerInterface {
 public :
  /**
   * @brief Create a new SelectPoller.
   * @param export_map the ExportMap to use
   * @param clock the Clock to use
   */
  SelectPoller(ExportMap *export_map, Clock *clock);

  ~SelectPoller();

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

  typedef std::map<int, ReadFileDescriptor*> ReadDescriptorMap;
  typedef std::map<int, WriteFileDescriptor*> WriteDescriptorMap;
  typedef std::map<int, connected_descriptor_t*> ConnectedDescriptorMap;

  ExportMap *m_export_map;
  CounterVariable *m_loop_iterations;
  CounterVariable *m_loop_time;
  Clock *m_clock;
  TimeStamp m_wake_up_time;

  ReadDescriptorMap m_read_descriptors;
  WriteDescriptorMap m_write_descriptors;
  ConnectedDescriptorMap m_connected_read_descriptors;

  void CheckDescriptors(fd_set *r_set, fd_set *w_set);
  bool AddDescriptorsToSet(fd_set *r_set, fd_set *w_set, int *max_sd);

  DISALLOW_COPY_AND_ASSIGN(SelectPoller);
};
}  // namespace io
}  // namespace ola
#endif  // COMMON_IO_SELECTPOLLER_H_
