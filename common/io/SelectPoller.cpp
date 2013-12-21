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
 * SelectPoller.cpp
 * A Poller which uses select()
 * Copyright (C) 2013 Simon Newton
 */

#include "common/io/SelectPoller.h"

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

SelectPoller::SelectPoller(ExportMap *export_map, Clock* clock)
    : m_export_map(export_map),
      m_loop_iterations(NULL),
      m_loop_time(NULL),
      m_clock(clock) {
  if (m_export_map) {
    m_loop_time = m_export_map->GetCounterVar(K_LOOP_TIME);
    m_loop_iterations = m_export_map->GetCounterVar(K_LOOP_COUNT);
  }
}

SelectPoller::~SelectPoller() {
  ConnectedDescriptorSet::iterator iter = m_connected_read_descriptors.begin();
  for (; iter != m_connected_read_descriptors.end(); ++iter) {
    if (iter->delete_on_close) {
      delete iter->descriptor;
    }
  }
  m_read_descriptors.clear();
  m_connected_read_descriptors.clear();
  m_write_descriptors.clear();
}

bool SelectPoller::AddReadDescriptor(class ReadFileDescriptor *descriptor) {
  if (!descriptor->ValidReadDescriptor()) {
    OLA_WARN << "AddReadDescriptor called with invalid descriptor";
    return false;
  }
  return STLInsertIfNotPresent(&m_read_descriptors, descriptor);
}

bool SelectPoller::AddReadDescriptor(class ConnectedDescriptor *descriptor,
                                     bool delete_on_close) {
  if (!descriptor->ValidReadDescriptor()) {
    OLA_WARN << "AddReadDescriptor called with invalid descriptor";
    return false;
  }

  // We make use of the fact that connected_descriptor_t_lt operates on the
  // descriptor value alone.
  connected_descriptor_t registered_descriptor = {descriptor, delete_on_close};

  return STLInsertIfNotPresent(&m_connected_read_descriptors,
                               registered_descriptor);
}

bool SelectPoller::RemoveReadDescriptor(class ReadFileDescriptor *descriptor) {
  if (!descriptor->ValidReadDescriptor()) {
    OLA_WARN << "Removing an invalid file descriptor";
    return false;
  }

  return STLRemove(&m_read_descriptors, descriptor);
}

bool SelectPoller::RemoveReadDescriptor(class ConnectedDescriptor *descriptor) {
  if (!descriptor->ValidReadDescriptor()) {
    OLA_WARN << "Removing an invalid file descriptor";
    return false;
  }

  // Comparison is based on descriptor only, so the second value is redundant.
  connected_descriptor_t registered_descriptor = {descriptor, false};
  return STLRemove(&m_connected_read_descriptors, registered_descriptor);
}

bool SelectPoller::AddWriteDescriptor(class WriteFileDescriptor *descriptor) {
  if (!descriptor->ValidWriteDescriptor()) {
    OLA_WARN << "AddWriteDescriptor called with invalid descriptor";
    return false;
  }

  return STLInsertIfNotPresent(&m_write_descriptors, descriptor);
}

bool SelectPoller::RemoveWriteDescriptor(
    class WriteFileDescriptor *descriptor) {
  if (!descriptor->ValidWriteDescriptor()) {
    OLA_WARN << "Removing a closed descriptor";
    return false;
  }

  return STLRemove(&m_write_descriptors, descriptor);
}

bool SelectPoller::Poll(TimeoutManager *timeout_manager,
                        const TimeInterval &poll_interval) {
  int maxsd;
  fd_set r_fds, w_fds;
  TimeStamp now;
  TimeInterval sleep_interval = poll_interval;
  struct timeval tv;

  maxsd = 0;
  FD_ZERO(&r_fds);
  FD_ZERO(&w_fds);
  m_clock->CurrentTime(&now);

  TimeInterval next_event_in = timeout_manager->ExecuteTimeouts(&now);
  if (!next_event_in.IsZero()) {
    sleep_interval = std::min(next_event_in, sleep_interval);
  }

  // Adding descriptors should be the last thing we do, they may have changed
  // due to timeouts above.
  bool closed_descriptors = AddDescriptorsToSet(&r_fds, &w_fds, &maxsd);
  // If there are closed descriptors, set the timeout to something
  // very small (1ms). This ensures we at least make a pass through the
  // descriptors.
  if (closed_descriptors) {
    sleep_interval = std::min(sleep_interval, TimeInterval(0, 1000));
  }

  // take care of stats accounting
  if (m_wake_up_time.IsSet()) {
    TimeInterval loop_time = now - m_wake_up_time;
    OLA_DEBUG << "ss process time was " << loop_time.ToString();
    if (m_loop_time)
      (*m_loop_time) += loop_time.AsInt();
    if (m_loop_iterations)
      (*m_loop_iterations)++;
  }

  sleep_interval.AsTimeval(&tv);
  switch (select(maxsd + 1, &r_fds, &w_fds, NULL, &tv)) {
    case 0:
      // timeout
      m_clock->CurrentTime(&m_wake_up_time);
      timeout_manager->ExecuteTimeouts(&m_wake_up_time);

      if (closed_descriptors) {
        // there were closed descriptors before the select() we need to deal
        // with them.
        FD_ZERO(&r_fds);
        FD_ZERO(&w_fds);
        CheckDescriptors(&r_fds, &w_fds);
      }
      return true;
    case -1:
      if (errno == EINTR)
        return true;
      OLA_WARN << "select() error, " << strerror(errno);
      return false;
    default:
      m_clock->CurrentTime(&m_wake_up_time);
      CheckDescriptors(&r_fds, &w_fds);
      m_clock->CurrentTime(&m_wake_up_time);
      timeout_manager->ExecuteTimeouts(&m_wake_up_time);
  }

  return true;
}

/*
 * Add all the descriptors to the FD_SET
 * @returns true if there are closed descriptors.
 */
bool SelectPoller::AddDescriptorsToSet(fd_set *r_set,
                                       fd_set *w_set,
                                       int *max_sd) {
  bool closed_descriptors = false;

  ReadDescriptorSet::iterator iter = m_read_descriptors.begin();
  while (iter != m_read_descriptors.end()) {
    ReadDescriptorSet::iterator this_iter = iter;
    iter++;

    if ((*this_iter)->ValidReadDescriptor()) {
      *max_sd = std::max(*max_sd, (*this_iter)->ReadDescriptor());
      FD_SET((*this_iter)->ReadDescriptor(), r_set);
    } else {
      // The descriptor was probably closed without removing it from the select
      // server
      if (m_export_map) {
        (*m_export_map->GetIntegerVar(K_READ_DESCRIPTOR_VAR))--;
      }
      m_read_descriptors.erase(this_iter);
      OLA_WARN << "Removed a inactive descriptor from the select server";
    }
  }

  ConnectedDescriptorSet::iterator con_iter =
      m_connected_read_descriptors.begin();
  while (con_iter != m_connected_read_descriptors.end()) {
    ConnectedDescriptorSet::iterator this_iter = con_iter;
    con_iter++;

    if (this_iter->descriptor->ValidReadDescriptor()) {
      *max_sd = std::max(*max_sd, this_iter->descriptor->ReadDescriptor());
      FD_SET(this_iter->descriptor->ReadDescriptor(), r_set);
    } else {
      closed_descriptors = true;
    }
  }

  WriteDescriptorSet::iterator write_iter = m_write_descriptors.begin();
  while (write_iter != m_write_descriptors.end()) {
    WriteDescriptorSet::iterator this_iter = write_iter;
    write_iter++;

    if ((*this_iter)->ValidWriteDescriptor()) {
      *max_sd = std::max(*max_sd, (*this_iter)->WriteDescriptor());
      FD_SET((*this_iter)->WriteDescriptor(), w_set);
    } else {
      // The descriptor was probably closed without removing it from the select
      // server
      if (m_export_map) {
        (*m_export_map->GetIntegerVar(K_WRITE_DESCRIPTOR_VAR))--;
      }
      m_write_descriptors.erase(this_iter);
      OLA_WARN << "Removed a disconnected descriptor from the select server";
    }
  }
  return closed_descriptors;
}


/*
 * Check all the registered descriptors:
 *  - Execute the callback for descriptors with data
 *  - Excute OnClose if a remote end closed the connection
 */
void SelectPoller::CheckDescriptors(fd_set *r_set, fd_set *w_set) {
  // Because the callbacks can add or remove descriptors from the select
  // server, we have to call them after we've used the iterators.
  std::queue<ReadFileDescriptor*> read_ready_queue;
  std::queue<WriteFileDescriptor*> write_ready_queue;
  std::queue<connected_descriptor_t> closed_queue;

  ReadDescriptorSet::iterator iter = m_read_descriptors.begin();
  for (; iter != m_read_descriptors.end(); ++iter) {
    if (FD_ISSET((*iter)->ReadDescriptor(), r_set))
      read_ready_queue.push(*iter);
  }

  // check the read sockets
  ConnectedDescriptorSet::iterator con_iter =
      m_connected_read_descriptors.begin();
  while (con_iter != m_connected_read_descriptors.end()) {
    ConnectedDescriptorSet::iterator this_iter = con_iter;
    con_iter++;
    bool closed = false;
    if (!this_iter->descriptor->ValidReadDescriptor()) {
      closed = true;
    } else if (FD_ISSET(this_iter->descriptor->ReadDescriptor(), r_set)) {
      if (this_iter->descriptor->IsClosed())
        closed = true;
      else
        read_ready_queue.push(this_iter->descriptor);
    }

    if (closed) {
      closed_queue.push(*this_iter);
      m_connected_read_descriptors.erase(this_iter);
    }
  }

  // check the write sockets
  WriteDescriptorSet::iterator write_iter = m_write_descriptors.begin();
  for (; write_iter != m_write_descriptors.end(); write_iter++) {
    if (FD_ISSET((*write_iter)->WriteDescriptor(), w_set))
      write_ready_queue.push(*write_iter);
  }

  // deal with anything that needs an action
  while (!read_ready_queue.empty()) {
    ReadFileDescriptor *descriptor = read_ready_queue.front();
    descriptor->PerformRead();
    read_ready_queue.pop();
  }

  while (!write_ready_queue.empty()) {
    WriteFileDescriptor *descriptor = write_ready_queue.front();
    descriptor->PerformWrite();
    write_ready_queue.pop();
  }

  while (!closed_queue.empty()) {
    const connected_descriptor_t &connected_descriptor = closed_queue.front();
    ConnectedDescriptor::OnCloseCallback *on_close =
      connected_descriptor.descriptor->TransferOnClose();
    if (on_close)
      on_close->Run();
    if (connected_descriptor.delete_on_close)
      delete connected_descriptor.descriptor;
    if (m_export_map) {
      (*m_export_map->GetIntegerVar(K_CONNECTED_DESCRIPTORS_VAR))--;
    }
    closed_queue.pop();
  }
}
}  // namespace io
}  // namespace ola
