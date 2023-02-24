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
 * KQueuePoller.cpp
 * A Poller which uses kqueue / kevent
 * Copyright (C) 2014 Simon Newton
 */

#include "common/io/KQueuePoller.h"

#include <string.h>
#include <errno.h>
#include <sys/event.h>

#include <algorithm>
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

/*
 * Represents a FD
 */
class KQueueData {
 public:
  KQueueData()
      : enable_read(false),
        enable_write(false),
        delete_connected_on_close(false),
        connected_close_in_progress(false),
        read_descriptor(NULL),
        write_descriptor(NULL),
        connected_descriptor(NULL) {
  }

  void Reset() {
    enable_read = false;
    enable_write = false;
    delete_connected_on_close = false;
    // True if this is a ConnectedDescriptor and it's in the process of being
    // closed
    connected_close_in_progress = false;
    read_descriptor = NULL;
    write_descriptor = NULL;
    connected_descriptor = NULL;
  }

  uint8_t enable_read : 1;
  uint8_t enable_write : 1;
  uint8_t delete_connected_on_close : 1;
  uint8_t connected_close_in_progress : 1;
  ReadFileDescriptor *read_descriptor;
  WriteFileDescriptor *write_descriptor;
  ConnectedDescriptor *connected_descriptor;
};

/**
 * @brief The maximum number of events to return in one epoll cycle
 */
const int KQueuePoller::MAX_EVENTS = 10;

/**
 * @brief The number of pre-allocated KQueueData to have.
 */
const unsigned int KQueuePoller::MAX_FREE_DESCRIPTORS = 10;

KQueuePoller::KQueuePoller(ExportMap *export_map, Clock* clock)
    : m_export_map(export_map),
      m_loop_iterations(NULL),
      m_loop_time(NULL),
      m_kqueue_fd(INVALID_DESCRIPTOR),
      m_next_change_entry(0),
      m_clock(clock) {
  if (m_export_map) {
    m_loop_time = m_export_map->GetCounterVar(K_LOOP_TIME);
    m_loop_iterations = m_export_map->GetCounterVar(K_LOOP_COUNT);
  }

  m_kqueue_fd = kqueue();
  if (m_kqueue_fd < 0) {
    OLA_FATAL << "Failed to create new kqueue";
  }
}

KQueuePoller::~KQueuePoller() {
  if (m_kqueue_fd != INVALID_DESCRIPTOR) {
    close(m_kqueue_fd);
  }

  {
    DescriptorMap::iterator iter = m_descriptor_map.begin();
    for (; iter != m_descriptor_map.end(); ++iter) {
      if (iter->second->delete_connected_on_close) {
        delete iter->second->connected_descriptor;
      }
      delete iter->second;
    }
  }

  DescriptorList::iterator iter = m_orphaned_descriptors.begin();
  for (; iter != m_orphaned_descriptors.end(); ++iter) {
    if ((*iter)->delete_connected_on_close) {
      delete (*iter)->connected_descriptor;
    }
    delete *iter;
  }

  STLDeleteElements(&m_free_descriptors);
}

bool KQueuePoller::AddReadDescriptor(ReadFileDescriptor *descriptor) {
  if (m_kqueue_fd == INVALID_DESCRIPTOR) {
    return false;
  }

  if (!descriptor->ValidReadDescriptor()) {
    OLA_WARN << "AddReadDescriptor called with invalid descriptor";
    return false;
  }

  pair<KQueueData*, bool> result = LookupOrCreateDescriptor(
      descriptor->ReadDescriptor());
  KQueueData *kqueue_data = result.first;

  if (kqueue_data->enable_read) {
    OLA_WARN << "Descriptor " << descriptor->ReadDescriptor()
             << " already in read set";
    return false;
  }

  kqueue_data->enable_read = true;
  kqueue_data->read_descriptor = descriptor;
  return ApplyChange(descriptor->ReadDescriptor(), EVFILT_READ, EV_ADD,
                     kqueue_data, false);
}

bool KQueuePoller::AddReadDescriptor(ConnectedDescriptor *descriptor,
                                     bool delete_on_close) {
  if (m_kqueue_fd == INVALID_DESCRIPTOR) {
    return false;
  }

  if (!descriptor->ValidReadDescriptor()) {
    OLA_WARN << "AddReadDescriptor called with invalid descriptor";
    return false;
  }

  pair<KQueueData*, bool> result = LookupOrCreateDescriptor(
      descriptor->ReadDescriptor());

  KQueueData *kqueue_data = result.first;

  if (kqueue_data->enable_read) {
    OLA_WARN << "Descriptor " << descriptor->ReadDescriptor()
             << " already in read set";
    return false;
  }

  kqueue_data->enable_read = true;
  kqueue_data->connected_descriptor = descriptor;
  kqueue_data->delete_connected_on_close = delete_on_close;
  return ApplyChange(descriptor->ReadDescriptor(), EVFILT_READ, EV_ADD,
                     kqueue_data, false);
}

bool KQueuePoller::RemoveReadDescriptor(ReadFileDescriptor *descriptor) {
  return RemoveDescriptor(descriptor->ReadDescriptor(), EVFILT_READ);
}

bool KQueuePoller::RemoveReadDescriptor(ConnectedDescriptor *descriptor) {
  return RemoveDescriptor(descriptor->ReadDescriptor(), EVFILT_READ);
}

bool KQueuePoller::AddWriteDescriptor(WriteFileDescriptor *descriptor) {
  if (m_kqueue_fd == INVALID_DESCRIPTOR) {
    return false;
  }

  if (!descriptor->ValidWriteDescriptor()) {
    OLA_WARN << "AddWriteDescriptor called with invalid descriptor";
    return false;
  }

  pair<KQueueData*, bool> result = LookupOrCreateDescriptor(
      descriptor->WriteDescriptor());
  KQueueData *kqueue_data = result.first;

  if (kqueue_data->enable_write) {
    OLA_WARN << "Descriptor " << descriptor->WriteDescriptor()
             << " already in write set";
    return false;
  }

  kqueue_data->enable_write = true;
  kqueue_data->write_descriptor = descriptor;
  return ApplyChange(descriptor->WriteDescriptor(), EVFILT_WRITE, EV_ADD,
                     kqueue_data, false);
}

bool KQueuePoller::RemoveWriteDescriptor(WriteFileDescriptor *descriptor) {
  return RemoveDescriptor(descriptor->WriteDescriptor(), EVFILT_WRITE);
}

bool KQueuePoller::Poll(TimeoutManager *timeout_manager,
                        const TimeInterval &poll_interval) {
  if (m_kqueue_fd == INVALID_DESCRIPTOR) {
    return false;
  }

  struct kevent events[MAX_EVENTS];

  TimeInterval sleep_interval = poll_interval;
  TimeStamp now;
  m_clock->CurrentMonotonicTime(&now);

  TimeInterval next_event_in = timeout_manager->ExecuteTimeouts(&now);
  if (!next_event_in.IsZero()) {
    sleep_interval = std::min(next_event_in, sleep_interval);
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

  struct timespec sleep_time;
  sleep_time.tv_sec = sleep_interval.Seconds();
  sleep_time.tv_nsec = sleep_interval.MicroSeconds() * 1000;

  int ready = kevent(
      m_kqueue_fd, reinterpret_cast<struct kevent*>(m_change_set),
      m_next_change_entry, events, MAX_EVENTS, &sleep_time);

  m_next_change_entry = 0;

  if (ready == 0) {
    m_clock->CurrentMonotonicTime(&m_wake_up_time);
    timeout_manager->ExecuteTimeouts(&m_wake_up_time);
    return true;
  } else if (ready == -1) {
    if (errno == EINTR)
      return true;
    OLA_WARN << "kqueue() error, " << strerror(errno);
    return false;
  }

  m_clock->CurrentMonotonicTime(&m_wake_up_time);

  for (int i = 0; i < ready; i++) {
    if (events[i].flags & EV_ERROR) {
      OLA_WARN << "Error from kqueue on fd: " << events[i].ident << ": "
               << strerror(events[i].data);
    } else {
      CheckDescriptor(&events[i]);
    }
  }

  // Now that we're out of the callback phase, clean up descriptors that were
  // removed.
  DescriptorList::iterator iter = m_orphaned_descriptors.begin();
  for (; iter != m_orphaned_descriptors.end(); ++iter) {
    if (m_free_descriptors.size() == MAX_FREE_DESCRIPTORS) {
      delete *iter;
    } else {
      (*iter)->Reset();
      m_free_descriptors.push_back(*iter);
    }
  }
  m_orphaned_descriptors.clear();

  m_clock->CurrentMonotonicTime(&m_wake_up_time);
  timeout_manager->ExecuteTimeouts(&m_wake_up_time);
  return true;
}


/*
 * Check all the registered descriptors:
 *  - Execute the callback for descriptors with data
 *  - Execute OnClose if a remote end closed the connection
 */
void KQueuePoller::CheckDescriptor(struct kevent *event) {
  KQueueData *kqueue_data = reinterpret_cast<KQueueData*>(
      event->udata);
  if (event->filter == EVFILT_READ) {
    if (kqueue_data->read_descriptor) {
      kqueue_data->read_descriptor->PerformRead();
    } else if (kqueue_data->connected_descriptor) {
      ConnectedDescriptor *connected_descriptor =
          kqueue_data->connected_descriptor;

      if (event->data) {
        connected_descriptor->PerformRead();
      } else if (event->flags & EV_EOF) {
        // The remote end closed the descriptor.
        // According to man kevent, closing the descriptor removes it from the
        // list of kevents. We don't want to queue up a EV_DELETE for the FD
        // because the FD number may be reused in short order.
        // So instead we set connected_close_in_progress which is a signal to
        // RemoveDescriptor not to create an EV_DELETE event if
        // RemoveReadDescriptor() is called.
        kqueue_data->connected_close_in_progress = true;

        ConnectedDescriptor::OnCloseCallback *on_close =
            connected_descriptor->TransferOnClose();
        if (on_close)
          on_close->Run();

        // At this point the descriptor may be sitting in the orphan list
        // if the OnClose handler called into RemoveReadDescriptor()
        if (kqueue_data->delete_connected_on_close) {
          delete connected_descriptor;

          // Remove from m_descriptor_map if it's still there
          kqueue_data = STLLookupAndRemovePtr(&m_descriptor_map, event->ident);
          if (kqueue_data) {
            m_orphaned_descriptors.push_back(kqueue_data);
            if (m_export_map) {
              (*m_export_map->GetIntegerVar(K_CONNECTED_DESCRIPTORS_VAR))--;
            }
          }
        }
      }
    }
  }

  if (event->filter == EVFILT_WRITE) {
    // kqueue_data->write_descriptor may be null here if this descriptor was
    // removed between when kevent returned and now.
    if (kqueue_data->write_descriptor) {
      kqueue_data->write_descriptor->PerformWrite();
    }
  }
}

std::pair<KQueueData*, bool> KQueuePoller::LookupOrCreateDescriptor(
    int fd) {
  pair<DescriptorMap::iterator, bool> result = m_descriptor_map.insert(
      DescriptorMap::value_type(fd, NULL));
  bool new_descriptor = result.second;

  if (new_descriptor) {
    if (m_free_descriptors.empty()) {
      result.first->second = new KQueueData();
    } else {
      result.first->second = m_free_descriptors.back();
      m_free_descriptors.pop_back();
    }
  }
  return std::make_pair(result.first->second, new_descriptor);
}

bool KQueuePoller::ApplyChange(int fd, int16_t filter, uint16_t flags,
                               KQueueData *descriptor,
                               bool apply_immediately) {
#ifdef __NetBSD__
  EV_SET(&m_change_set[m_next_change_entry++], fd, filter, flags, 0, 0,
         reinterpret_cast<intptr_t>(descriptor));
#else
  EV_SET(&m_change_set[m_next_change_entry++], fd, filter, flags, 0, 0,
         descriptor);
#endif  // __NetBSD__

  if (m_next_change_entry == CHANGE_SET_SIZE || apply_immediately) {
    int r = kevent(m_kqueue_fd, m_change_set, m_next_change_entry, NULL, 0,
                   NULL);
    if (r < 0) {
      OLA_WARN << "Failed to apply kqueue changes: " << strerror(errno);
    }
    m_next_change_entry = 0;
  }
  return true;
}

bool KQueuePoller::RemoveDescriptor(int fd, int16_t filter) {
  if (fd == INVALID_DESCRIPTOR) {
    OLA_WARN << "Attempt to remove an invalid file descriptor";
    return false;
  }

  KQueueData *kqueue_data = STLFindOrNull(m_descriptor_map, fd);
  if (!kqueue_data) {
    OLA_WARN << "Couldn't find KQueueData for fd " << fd;
    return false;
  }

  bool remove_from_kevent = true;

  if (filter == EVFILT_READ) {
    kqueue_data->enable_read = false;
    kqueue_data->read_descriptor = NULL;
    if (kqueue_data->connected_descriptor) {
      remove_from_kevent = !kqueue_data->connected_close_in_progress;
      kqueue_data->connected_descriptor = NULL;
    }
  } else if (filter == EVFILT_WRITE) {
    kqueue_data->enable_write = false;
    kqueue_data->write_descriptor = NULL;
  } else {
    OLA_WARN << "Unknown kqueue filter: " << filter;
  }

  if (remove_from_kevent) {
    ApplyChange(fd, filter, EV_DELETE, NULL, true);
  }

  if (!kqueue_data->enable_read && !kqueue_data->enable_write) {
    m_orphaned_descriptors.push_back(
        STLLookupAndRemovePtr(&m_descriptor_map, fd));
  }
  return true;
}
}  // namespace io
}  // namespace ola
