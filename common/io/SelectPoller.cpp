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
 * SelectPoller.cpp
 * A Poller which uses select()
 * Copyright (C) 2013 Simon Newton
 */

#ifdef _WIN32
// Pull in fd_set and related definitions.
#include <ola/win/CleanWinSock2.h>
#endif  // _WIN32

#include "common/io/SelectPoller.h"

#include <string.h>
#include <errno.h>

#include <algorithm>
#include <map>
#include <queue>
#include <string>
#include <utility>

#include "ola/Clock.h"
#include "ola/Logging.h"
#include "ola/base/Macro.h"
#include "ola/io/Descriptor.h"
#include "ola/stl/STLUtils.h"

namespace ola {
namespace io {

using std::pair;
using std::map;
using std::string;

/**
 * @brief Insert a descriptor into one of the descriptor maps.
 * @param descriptor_map the descriptor_map to insert into.
 * @param fd the FD to use as the key
 * @param value the value to associate with the key
 * @param type the name of the map, used for logging if the fd already exists
 *   in the map.
 * @returns true if the descriptor was inserted, false if it was already in the
 *   map.
 *
 * There are three possibilities:
 *  - The fd does not already exist in the map
 *  - The fd exists and the value is NULL.
 *  - The fd exists and is not NULL.
 */
template <typename T>
bool InsertIntoDescriptorMap(map<int, T*> *descriptor_map, int fd, T *value,
                             const string &type) {
  typedef map<int, T*> MapType;
  pair<typename MapType::iterator, bool> p = descriptor_map->insert(
      typename MapType::value_type(fd, value));

  if (!p.second) {
    // already in map
    if (p.first->second == NULL) {
      p.first->second = value;
    } else {
      OLA_WARN << "FD " << fd << " was already in the " << type
          << " descriptor map: " << p.first->second << " : " << value;
      return false;
    }
  }
  return true;
}

/**
 * @brief Remove a FD from a descriptor map by setting the value to NULL.
 * @returns true if the FD was removed from the map, false if it didn't exist
 *   in the map.
 */
template <typename T>
bool RemoveFromDescriptorMap(map<int, T*> *descriptor_map, int fd) {
  T **ptr = STLFind(descriptor_map, fd);
  if (ptr) {
    *ptr = NULL;
    return true;
  }
  return false;
}

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
  ConnectedDescriptorMap::iterator iter = m_connected_read_descriptors.begin();
  for (; iter != m_connected_read_descriptors.end(); ++iter) {
    if (iter->second) {
      if (iter->second->delete_on_close) {
        delete iter->second->descriptor;
      }
      delete iter->second;
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

  return InsertIntoDescriptorMap(&m_read_descriptors,
      descriptor->ReadDescriptor(), descriptor, "read");
}

bool SelectPoller::AddReadDescriptor(class ConnectedDescriptor *descriptor,
                                     bool delete_on_close) {
  if (!descriptor->ValidReadDescriptor()) {
    OLA_WARN << "AddReadDescriptor called with invalid descriptor";
    return false;
  }

  connected_descriptor_t *cd = new connected_descriptor_t();
  cd->descriptor = descriptor;
  cd->delete_on_close = delete_on_close;

  bool ok = InsertIntoDescriptorMap(&m_connected_read_descriptors,
      descriptor->ReadDescriptor(), cd, "connected");
  if (!ok) {
    delete cd;
  }
  return ok;
}

bool SelectPoller::RemoveReadDescriptor(class ReadFileDescriptor *descriptor) {
  if (!descriptor->ValidReadDescriptor()) {
    OLA_WARN << "Removing an invalid ReadDescriptor";
    return false;
  }

  return RemoveFromDescriptorMap(&m_read_descriptors,
                                 descriptor->ReadDescriptor());
}

bool SelectPoller::RemoveReadDescriptor(class ConnectedDescriptor *descriptor) {
  if (!descriptor->ValidReadDescriptor()) {
    OLA_WARN << "Removing an invalid ConnectedDescriptor";
    return false;
  }

  connected_descriptor_t **ptr = STLFind(
      &m_connected_read_descriptors, descriptor->ReadDescriptor());
  if (ptr && *ptr) {
    delete *ptr;
    *ptr = NULL;
    return true;
  }
  return false;
}

bool SelectPoller::AddWriteDescriptor(class WriteFileDescriptor *descriptor) {
  if (!descriptor->ValidWriteDescriptor()) {
    OLA_WARN << "AddWriteDescriptor called with invalid descriptor";
    return false;
  }

  return InsertIntoDescriptorMap(&m_write_descriptors,
      descriptor->WriteDescriptor(), descriptor, "write");
}

bool SelectPoller::RemoveWriteDescriptor(
    class WriteFileDescriptor *descriptor) {
  if (!descriptor->ValidWriteDescriptor()) {
    OLA_WARN << "Removing an invalid WriteDescriptor";
    return false;
  }

  return RemoveFromDescriptorMap(&m_write_descriptors,
                                 descriptor->WriteDescriptor());
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
  m_clock->CurrentMonotonicTime(&now);

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
      m_clock->CurrentMonotonicTime(&m_wake_up_time);
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
      m_clock->CurrentMonotonicTime(&m_wake_up_time);
      CheckDescriptors(&r_fds, &w_fds);
      m_clock->CurrentMonotonicTime(&m_wake_up_time);
      timeout_manager->ExecuteTimeouts(&m_wake_up_time);
  }

  return true;
}

/*
 * Add all the descriptors to the FD_SET.
 * @returns true if there are descriptors that have been closed.
 *
 * This also takes care of removing any entries from the maps where the value
 * is NULL. This is safe because we don't execute any callbacks from within
 * this method.
 */
bool SelectPoller::AddDescriptorsToSet(fd_set *r_set,
                                       fd_set *w_set,
                                       int *max_sd) {
  bool closed_descriptors = false;

  ReadDescriptorMap::iterator iter = m_read_descriptors.begin();
  while (iter != m_read_descriptors.end()) {
    ReadDescriptorMap::iterator this_iter = iter;
    iter++;

    ReadFileDescriptor *descriptor = this_iter->second;
    if (!descriptor) {
      // This one was removed.
      m_read_descriptors.erase(this_iter);
      continue;
    }

    if (descriptor->ValidReadDescriptor()) {
      *max_sd = std::max(*max_sd, descriptor->ReadDescriptor());
      FD_SET(descriptor->ReadDescriptor(), r_set);
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

  ConnectedDescriptorMap::iterator con_iter =
      m_connected_read_descriptors.begin();
  while (con_iter != m_connected_read_descriptors.end()) {
    ConnectedDescriptorMap::iterator this_iter = con_iter;
    con_iter++;

    if (!this_iter->second) {
      // This one was removed.
      m_connected_read_descriptors.erase(this_iter);
      continue;
    }

    if (this_iter->second->descriptor->ValidReadDescriptor()) {
      *max_sd = std::max(
          *max_sd, this_iter->second->descriptor->ReadDescriptor());
      FD_SET(this_iter->second->descriptor->ReadDescriptor(), r_set);
    } else {
      closed_descriptors = true;
    }
  }

  WriteDescriptorMap::iterator write_iter = m_write_descriptors.begin();
  while (write_iter != m_write_descriptors.end()) {
    WriteDescriptorMap::iterator this_iter = write_iter;
    write_iter++;

    WriteFileDescriptor *descriptor = this_iter->second;
    if (!descriptor) {
      // This one was removed.
      m_write_descriptors.erase(this_iter);
      continue;
    }

    if (descriptor->ValidWriteDescriptor()) {
      *max_sd = std::max(*max_sd, descriptor->WriteDescriptor());
      FD_SET(descriptor->WriteDescriptor(), w_set);
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
 *  - Execute OnClose if a remote end closed the connection
 */
void SelectPoller::CheckDescriptors(fd_set *r_set, fd_set *w_set) {
  // Remember the add / remove methods above may be called during
  // PerformRead(), PerformWrite() or the on close handler. Our iterators are
  // safe because we only ever call erase from within AddDescriptorsToSet(),
  // which isn't called from any of the Add / Remove methods.
  ReadDescriptorMap::iterator iter = m_read_descriptors.begin();
  for (; iter != m_read_descriptors.end(); ++iter) {
    if (iter->second && FD_ISSET(iter->second->ReadDescriptor(), r_set)) {
      iter->second->PerformRead();
    }
  }

  ConnectedDescriptorMap::iterator con_iter =
      m_connected_read_descriptors.begin();
  for (; con_iter != m_connected_read_descriptors.end(); ++con_iter) {
    if (!con_iter->second) {
      continue;
    }

    connected_descriptor_t *cd = con_iter->second;
    ConnectedDescriptor *descriptor = cd->descriptor;

    bool closed = false;
    if (!descriptor->ValidReadDescriptor()) {
      closed = true;
    } else if (FD_ISSET(descriptor->ReadDescriptor(), r_set)) {
      if (descriptor->IsClosed()) {
        closed = true;
      } else {
        descriptor->PerformRead();
      }
    }

    if (closed) {
      ConnectedDescriptor::OnCloseCallback *on_close =
        descriptor->TransferOnClose();
      bool delete_on_close = cd->delete_on_close;

      delete con_iter->second;
      con_iter->second = NULL;
      if (m_export_map) {
        (*m_export_map->GetIntegerVar(K_CONNECTED_DESCRIPTORS_VAR))--;
      }

      if (on_close)
        on_close->Run();

      if (delete_on_close)
        delete descriptor;
    }
  }

  // Check the write sockets. These may have changed since the start of the
  // method due to running callbacks.
  WriteDescriptorMap::iterator write_iter = m_write_descriptors.begin();
  for (; write_iter != m_write_descriptors.end(); write_iter++) {
    if (write_iter->second &&
        FD_ISSET(write_iter->second->WriteDescriptor(), w_set)) {
      write_iter->second->PerformWrite();
    }
  }
}
}  // namespace io
}  // namespace ola
