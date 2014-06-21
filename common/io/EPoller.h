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
 * EPoller.h
 * A Poller which uses epoll()
 * Copyright (C) 2013 Simon Newton
 */

#ifndef COMMON_IO_EPOLLER_H_
#define COMMON_IO_EPOLLER_H_

#include <ola/base/Macro.h>
#include <ola/Clock.h>
#include <ola/ExportMap.h>
#include <ola/io/Descriptor.h>
#include <sys/epoll.h>

#include <map>
#include <string>
#include <set>
#include <utility>

#include "common/io/PollerInterface.h"
#include "common/io/TimeoutManager.h"

namespace ola {
namespace io {

/**
 * @class EPoller
 * @brief An implementation of PollerInterface that uses epoll().
 *
 * epoll() is more efficient than select() but only newer Linux systems support
 * it.
 */
class EPoller : public PollerInterface {
 public :
  /**
   * @brief Create a new EPoller.
   * @param export_map the ExportMap to use
   * @param clock the Clock to use
   */
  EPoller(ExportMap *export_map, Clock *clock);

  ~EPoller();

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
  struct EPollDescriptor {
   public:
     EPollDescriptor()
         : events(0),
           read_descriptor(NULL),
           write_descriptor(NULL),
           connected_descriptor(NULL),
           delete_connected_on_close(false) {
    }

    uint32_t events;
    ReadFileDescriptor *read_descriptor;
    WriteFileDescriptor *write_descriptor;
    ConnectedDescriptor *connected_descriptor;
    bool delete_connected_on_close;
  };

  typedef std::map<int, EPollDescriptor*> DescriptorMap;

  DescriptorMap m_descriptor_map;
  ExportMap *m_export_map;
  CounterVariable *m_loop_iterations;
  CounterVariable *m_loop_time;
  int m_epoll_fd;
  Clock *m_clock;
  TimeStamp m_wake_up_time;

  std::pair<EPollDescriptor*, bool> LookupOrCreateDescriptor(int fd);

  bool AddEvent(int fd, EPollDescriptor *descriptor);
  bool UpdateEvent(int fd, EPollDescriptor *descriptor);
  bool RemoveEvent(int fd);
  bool RemoveDescriptor(int fd, int event);

  void CheckDescriptor(struct epoll_event *event, EPollDescriptor *descriptor);

  static const int MAX_EVENTS;
  static const int READ_FLAGS;

  DISALLOW_COPY_AND_ASSIGN(EPoller);
};
}  // namespace io
}  // namespace ola
#endif  // COMMON_IO_EPOLLER_H_
