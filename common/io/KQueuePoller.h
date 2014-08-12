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
 * KQueuePoller.h
 * A Poller which uses kqueue / kevent
 * Copyright (C) 2014 Simon Newton
 */

#ifndef COMMON_IO_KQUEUEPOLLER_H_
#define COMMON_IO_KQUEUEPOLLER_H_

#include <ola/base/Macro.h>
#include <ola/Clock.h>
#include <ola/ExportMap.h>
#include <ola/io/Descriptor.h>
#include <sys/event.h>

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "common/io/PollerInterface.h"
#include "common/io/TimeoutManager.h"

namespace ola {
namespace io {

class KQueueData;

/**
 * @class KQueuePoller
 * @brief An implementation of PollerInterface that uses kevent / kqueue.
 *
 * kevent is more efficient than select() but only BSD-style systems support
 * it.
 */
class KQueuePoller : public PollerInterface {
 public :
  /**
   * @brief Create a new KQueuePoller.
   * @param export_map the ExportMap to use
   * @param clock the Clock to use
   */
  KQueuePoller(ExportMap *export_map, Clock *clock);

  ~KQueuePoller();

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
  enum { CHANGE_SET_SIZE = 10 };

  typedef std::map<int, KQueueData*> DescriptorMap;
  typedef std::vector<KQueueData*> DescriptorList;

  DescriptorMap m_descriptor_map;

  // KQueuePoller is re-enterant. Remove may be called while we hold a pointer
  // to an KQueueData. To avoid deleting data out from underneath
  // ourselves, we instead move the removed descriptors to this list and then
  // clean them up outside the callback loop.
  DescriptorList m_orphaned_descriptors;
  // A list of pre-allocated descriptors we can use.
  DescriptorList m_free_descriptors;
  ExportMap *m_export_map;
  CounterVariable *m_loop_iterations;
  CounterVariable *m_loop_time;
  int m_kqueue_fd;

  struct kevent m_change_set[CHANGE_SET_SIZE];
  unsigned int m_next_change_entry;

  Clock *m_clock;
  TimeStamp m_wake_up_time;

  void CheckDescriptor(struct kevent *event);
  std::pair<KQueueData*, bool> LookupOrCreateDescriptor(int fd);
  bool ApplyChange(int fd, int16_t filter, uint16_t flags,
                   KQueueData *kqueue_data, bool apply_immediately);
  bool RemoveDescriptor(int fd, int16_t filter);

  static const int MAX_EVENTS;
  static const unsigned int MAX_FREE_DESCRIPTORS;

  DISALLOW_COPY_AND_ASSIGN(KQueuePoller);
};
}  // namespace io
}  // namespace ola
#endif  // COMMON_IO_KQUEUEPOLLER_H_
