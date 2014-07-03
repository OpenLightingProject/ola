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

std::string FilterToString(int16_t filter) {
  if (filter == EVFILT_READ) {
    return "EVFILT_READ";
  } else if (filter == EVFILT_WRITE) {
    return "EVFILT_WRITE";
  } else {
    return "unknown";
  }
}

std::string FlagsToString(uint16_t flags) {
  std::ostringstream str;

  if (flags & EV_ADD) {
    str << "EV_ADD, ";
  }
  if (flags & EV_DELETE) {
    str << "EV_DELETE, ";
  }
  if (flags & EV_ERROR) {
    str << "EV_ERROR, ";
  }
  if (flags & EV_EOF) {
    str << "EV_EOF, ";
  }
  str << std::hex << " (" << flags << ")";
  return str.str();
}


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
                     kqueue_descriptor, false);
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
                     kqueue_descriptor, false);
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
                     kqueue_descriptor, false);
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
  sleep_time.tv_nsec = sleep_interval.MicroSeconds() * 1000;

  /*
  OLA_INFO << "Calling kevent with " << m_next_change_entry
           << " changes, sleep for " << sleep_interval;
  OLA_INFO << "sleep is " << sleep_time.tv_sec << "." << sleep_time.tv_nsec;
  */
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

  //OLA_INFO << "Got " << ready << " events";
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
  /*
  OLA_INFO << "Event " << event->ident << ", filter "
           << FilterToString(event->filter)
           << ", flags " << FlagsToString(event->flags)
           << ", data is " << event->data
           << ", Udata was " << event->udata;
           */

  if (event->filter == EVFILT_READ) {
    if (descriptor->read_descriptor) {
      descriptor->read_descriptor->PerformRead();
    } else if (descriptor->connected_descriptor) {
      ConnectedDescriptor *connected_descriptor =
          descriptor->connected_descriptor;

      if (event->data) {
        connected_descriptor->PerformRead();
      } else if (event->flags & EV_EOF) {
        // The remote end closed the descriptor.
        // According to man kevent, closing the descriptor remove it from the
        // list of kevents. We don't want to queue up a EV_DELETE for the FD
        // because the FD number may be reused in short order.
        // So instead we ste connected_close_in_progress which is a signal to
        // RemoveDescriptor not to create an EV_DELETE event if
        // RemoveReadDescriptor() is called.
        descriptor->connected_close_in_progress = true;

        ConnectedDescriptor::OnCloseCallback *on_close =
            connected_descriptor->TransferOnClose();
        if (on_close)
          on_close->Run();

        // At this point the descriptor may be sitting in the orphan list
        // if the OnClose handler called into RemoveReadDescriptor()
        if (descriptor->delete_connected_on_close) {
          delete connected_descriptor;

          // Remove from m_descriptor_map if it's still there
          descriptor = STLLookupAndRemovePtr(&m_descriptor_map, event->ident);
          if (descriptor) {
            m_orphaned_descriptors.push_back(descriptor);
            if (m_export_map) {
              (*m_export_map->GetIntegerVar(K_CONNECTED_DESCRIPTORS_VAR))--;
            }
          }
        }
      }
    }
  }

  if (event->filter == EVFILT_WRITE) {
    if (descriptor->write_descriptor) {
      descriptor->write_descriptor->PerformWrite();
    } else {
      OLA_FATAL << "EVFILT_WRITE active but write_descriptor is NULL";
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
                               KQueueDescriptor *descriptor,
                               bool apply_immediately) {
  /*
  OLA_INFO << "Set " << fd << ", filter " << FilterToString(filter)
           << ", flags " << FlagsToString(flags) << ", udata " << descriptor;
           */
  EV_SET(&m_change_set[m_next_change_entry++], fd, filter, flags, 0, 0,
         descriptor);

  if (m_next_change_entry == CHANGE_SET_SIZE || apply_immediately) {
    int r = kevent(m_kqueue_fd, m_change_set, m_next_change_entry, NULL, 0,
                   NULL);
    if (r < 0) {
      OLA_WARN << "Failed to apply kqueue changes";
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

  KQueueDescriptor *kqueue_descriptor = STLFindOrNull(m_descriptor_map, fd);
  if (!kqueue_descriptor) {
    OLA_WARN << "Couldn't find KQueueDescriptor for " << fd;
    return false;
  }

  bool remove_from_kevent = true;

  if (filter == EVFILT_READ) {
    kqueue_descriptor->enable_read = false;
    kqueue_descriptor->read_descriptor = NULL;
    if (kqueue_descriptor->connected_descriptor) {
      remove_from_kevent = !kqueue_descriptor->connected_close_in_progress;
      kqueue_descriptor->connected_descriptor = NULL;
    }
  } else if (filter == EVFILT_WRITE) {
    kqueue_descriptor->enable_write = false;
    kqueue_descriptor->write_descriptor = NULL;
  } else {
    OLA_WARN << "Unknown kqueue filter: " << filter;
  }

  if (remove_from_kevent) {
    ApplyChange(fd, filter, EV_DELETE, NULL, true);
  }

  if (!kqueue_descriptor->enable_read && !kqueue_descriptor->enable_write) {
    m_orphaned_descriptors.push_back(
        STLLookupAndRemovePtr(&m_descriptor_map, fd));
  }
  return true;
}
}  // namespace io
}  // namespace ola
