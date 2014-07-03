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
class KQueueDescriptor {
 public:
  KQueueDescriptor()
      : enable_read(0),
        enable_write(0),
        delete_connected_on_close(0),
        read_descriptor(NULL),
        write_descriptor(NULL),
        connected_descriptor(NULL) {
  }

  void Reset() {
    enable_read = false;
    enable_write = false;
    delete_connected_on_close = false;
    read_descriptor = NULL;
    write_descriptor = NULL;
    connected_descriptor = NULL;
  }

  uint8_t enable_read : 1;
  uint8_t enable_write : 1;
  uint8_t delete_connected_on_close : 1;
  ReadFileDescriptor *read_descriptor;
  WriteFileDescriptor *write_descriptor;
  ConnectedDescriptor *connected_descriptor;
};

/**
 * @brief The maximum number of events to return in one epoll cycle
 */
const int KQueuePoller::MAX_EVENTS = 10;

/**
 * @brief The number of pre-allocated KQueueDescriptor to have.
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

  pair<KQueueDescriptor*, bool> result = LookupOrCreateDescriptor(
      descriptor->ReadDescriptor());
  KQueueDescriptor *kqueue_descriptor = result.first;

  if (kqueue_descriptor->enable_read) {
    OLA_WARN << "Descriptor " << descriptor->ReadDescriptor()
             << " already in read set";
    return false;
  }

  kqueue_descriptor->enable_read = true;
  kqueue_descriptor->read_descriptor = descriptor;
  return ApplyChange(descriptor->ReadDescriptor(), EVFILT_READ, EV_ADD,
                     kqueue_descriptor);
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

  pair<KQueueDescriptor*, bool> result = LookupOrCreateDescriptor(
      descriptor->ReadDescriptor());

  KQueueDescriptor *kqueue_descriptor = result.first;

  if (kqueue_descriptor->enable_read) {
    OLA_WARN << "Descriptor " << descriptor->ReadDescriptor()
             << " already in read set";
    return false;
  }

  kqueue_descriptor->enable_read = true;
  kqueue_descriptor->connected_descriptor = descriptor;
  kqueue_descriptor->delete_connected_on_close = delete_on_close;
  return ApplyChange(descriptor->ReadDescriptor(), EVFILT_READ, EV_ADD,
                     kqueue_descriptor);
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

  pair<KQueueDescriptor*, bool> result = LookupOrCreateDescriptor(
      descriptor->WriteDescriptor());
  KQueueDescriptor *kqueue_descriptor = result.first;

  if (kqueue_descriptor->enable_write) {
    OLA_WARN << "Descriptor " << descriptor->WriteDescriptor()
             << " already in write set";
    return false;
  }

  kqueue_descriptor->enable_write = true;
  kqueue_descriptor->write_descriptor = descriptor;
  return ApplyChange(descriptor->WriteDescriptor(), EVFILT_WRITE, EV_ADD,
                     kqueue_descriptor);
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
  m_clock->CurrentTime(&now);

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
  sleep_time.tv_nsec = sleep_interval.InMilliSeconds() / 1000;

  OLA_INFO << "Calling kevent with " << m_next_change_entry << " changes";
  int ready = kevent(
      m_kqueue_fd, reinterpret_cast<struct kevent*>(m_change_set),
      m_next_change_entry, events, MAX_EVENTS, &sleep_time);

  m_next_change_entry = 0;

  if (ready == 0) {
    m_clock->CurrentTime(&m_wake_up_time);
    timeout_manager->ExecuteTimeouts(&m_wake_up_time);
    return true;
  } else if (ready == -1) {
    if (errno == EINTR)
      return true;
    OLA_WARN << "kqueue() error, " << strerror(errno);
    return false;
  }

  m_clock->CurrentTime(&m_wake_up_time);

  OLA_INFO << "Got " << ready << " events";
  for (int i = 0; i < ready; i++) {
    if (events[i].flags & EV_ERROR) {
      OLA_WARN << "Error from kqueue " << strerror(events[i].data);
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

  m_clock->CurrentTime(&m_wake_up_time);
  timeout_manager->ExecuteTimeouts(&m_wake_up_time);
  return true;
}


/*
 * Check all the registered descriptors:
 *  - Execute the callback for descriptors with data
 *  - Excute OnClose if a remote end closed the connection
 */
void KQueuePoller::CheckDescriptor(struct kevent *event) {
  KQueueDescriptor *descriptor = reinterpret_cast<KQueueDescriptor*>(
      event->udata);
  OLA_INFO << "Event " << event->ident << ", filter " << event->filter
           << ", flags 0x" << std::hex << event->flags
           << ", Udata was " << event->udata;

  if (event->filter == EVFILT_READ) {
    if (descriptor->read_descriptor) {
      descriptor->read_descriptor->PerformRead();
    } else if (descriptor->connected_descriptor) {
      if (event->data) {
        descriptor->connected_descriptor->PerformRead();
      } else if (event->flags & EV_EOF) {
        // FD was closed
        bool delete_on_close = descriptor->delete_connected_on_close;
        ConnectedDescriptor::OnCloseCallback *on_close =
            descriptor->connected_descriptor->TransferOnClose();
        if (on_close)
          on_close->Run();

        if (delete_on_close) {
          ApplyChange(descriptor->connected_descriptor->WriteDescriptor(),
                      EVFILT_READ, EV_DELETE, NULL);
          if (m_export_map) {
            (*m_export_map->GetIntegerVar(K_CONNECTED_DESCRIPTORS_VAR))--;
          }
          delete descriptor->connected_descriptor;
          descriptor->connected_descriptor = NULL;
        }
      }
    }
  }

  if (event->filter == EVFILT_WRITE) {
    if (descriptor->write_descriptor) {
      descriptor->write_descriptor->PerformWrite();
    } else {
      OLA_FATAL << "EPOLLOUT active but write_descriptor is NULL";
    }
  }
}

std::pair<KQueueDescriptor*, bool> KQueuePoller::LookupOrCreateDescriptor(
    int fd) {
  pair<DescriptorMap::iterator, bool> result = m_descriptor_map.insert(
      DescriptorMap::value_type(fd, NULL));
  bool new_descriptor = result.second;

  if (new_descriptor) {
    if (m_free_descriptors.empty()) {
      result.first->second = new KQueueDescriptor();
    } else {
      result.first->second = m_free_descriptors.back();
      m_free_descriptors.pop_back();
    }
  }
  return std::make_pair(result.first->second, new_descriptor);
}

bool KQueuePoller::ApplyChange(int fd, int16_t filter, uint16_t flags,
                               KQueueDescriptor *descriptor) {
  if (m_next_change_entry == CHANGE_SET_SIZE) {
    // Our change set is full, so call kevent
    OLA_INFO << "Applying change set ";
    int r = kevent(m_kqueue_fd, m_change_set, m_next_change_entry, NULL, 0,
                   NULL);
    OLA_INFO << "kevent returned " << r;
    m_next_change_entry = 0;
  }

  OLA_INFO << "Set " << fd << ", filter " << filter << ", flags " << flags
           << ", udata " << descriptor;
  EV_SET(&m_change_set[m_next_change_entry++], fd, filter, flags, 0, 0,
         descriptor);
  return true;
}

bool KQueuePoller::RemoveDescriptor(int fd, int16_t filter) {
  if (fd == INVALID_DESCRIPTOR) {
    OLA_WARN << "Attempt to remove an invalid file descriptor";
    return false;
  }

  KQueueDescriptor *kqueue_descriptor = STLFindOrNull(m_descriptor_map, fd);
  if (!kqueue_descriptor) {
    OLA_WARN << "Couldn't find KQueueDescriptor for " << fd;
    return false;
  }

  if (filter == EVFILT_READ) {
    kqueue_descriptor->enable_read = false;
    kqueue_descriptor->read_descriptor = NULL;
    kqueue_descriptor->connected_descriptor = NULL;
  } else if (filter == EVFILT_WRITE) {
    kqueue_descriptor->enable_write = false;
    kqueue_descriptor->write_descriptor = NULL;
  } else {
    OLA_WARN << "Unknown kqueue filter: " << filter;
  }

  ApplyChange(fd, filter, EV_DELETE, NULL);

  if (!kqueue_descriptor->enable_read && !kqueue_descriptor->enable_write) {
    m_orphaned_descriptors.push_back(
        STLLookupAndRemovePtr(&m_descriptor_map, fd));
  }
  return true;
}
}  // namespace io
}  // namespace ola
