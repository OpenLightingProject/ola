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
 * EPoller.cpp
 * A Poller which uses epoll()
 * Copyright (C) 2013 Simon Newton
 */

#include "common/io/EPoller.h"

#include <string.h>
#include <errno.h>
#include <sys/epoll.h>

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
class EPollData {
 public:
  EPollData()
      : events(0),
        read_descriptor(NULL),
        write_descriptor(NULL),
        connected_descriptor(NULL),
        delete_connected_on_close(false) {
  }

  void Reset() {
    events = 0;
    read_descriptor = NULL;
    write_descriptor = NULL;
    connected_descriptor = NULL;
    delete_connected_on_close = false;
  }

  uint32_t events;
  ReadFileDescriptor *read_descriptor;
  WriteFileDescriptor *write_descriptor;
  ConnectedDescriptor *connected_descriptor;
  bool delete_connected_on_close;
};

namespace {

/*
 * Add the fd to the epoll_fd.
 * descriptor is the user data to associated with the event
 */
bool AddEvent(int epoll_fd, int fd, EPollData *descriptor) {
  epoll_event event;
  event.events = descriptor->events;
  event.data.ptr = descriptor;

  OLA_DEBUG << "EPOLL_CTL_ADD " << fd << ", events " << std::hex
            << event.events << ", descriptor: " << descriptor;
  int r = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
  if (r) {
    OLA_WARN << "EPOLL_CTL_ADD " << fd << " failed: " << strerror(errno);
    return false;
  }
  return true;
}

/*
 * Update the fd in the epoll event set
 * descriptor is the user data to associated with the event
 */
bool UpdateEvent(int epoll_fd, int fd, EPollData *descriptor) {
  epoll_event event;
  event.events = descriptor->events;
  event.data.ptr = descriptor;

  OLA_DEBUG << "EPOLL_CTL_MOD " << fd << ", events " << std::hex
            << event.events << ", descriptor: " << descriptor;
  int r = epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &event);
  if (r) {
    OLA_WARN << "EPOLL_CTL_MOD " << fd << " failed: " << strerror(errno);
    return false;
  }
  return true;
}

/*
 * Remove the fd from the epoll fd
 */
bool RemoveEvent(int epoll_fd, int fd) {
  epoll_event event;
  OLA_DEBUG << "EPOLL_CTL_DEL " << fd;
  int r = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, &event);
  if (r) {
    OLA_WARN << "EPOLL_CTL_DEL " << fd << " failed: " << strerror(errno);
    return false;
  }
  return true;
}
}  // namespace

/**
 * @brief The maximum number of events to return in one epoll cycle
 */
const int EPoller::MAX_EVENTS = 10;


/**
 * @brief the EPOLL flags used for read descriptors.
 */
const int EPoller::READ_FLAGS = EPOLLIN | EPOLLRDHUP;

/**
 * @brief The number of pre-allocated EPollData to have.
 */
const unsigned int EPoller::MAX_FREE_DESCRIPTORS = 10;

EPoller::EPoller(ExportMap *export_map, Clock* clock)
    : m_export_map(export_map),
      m_loop_iterations(NULL),
      m_loop_time(NULL),
      m_epoll_fd(INVALID_DESCRIPTOR),
      m_clock(clock) {
  if (m_export_map) {
    m_loop_time = m_export_map->GetCounterVar(K_LOOP_TIME);
    m_loop_iterations = m_export_map->GetCounterVar(K_LOOP_COUNT);
  }

  m_epoll_fd = epoll_create1(EPOLL_CLOEXEC);
  if (m_epoll_fd < 0) {
    OLA_FATAL << "Failed to create new epoll instance";
  }
}

EPoller::~EPoller() {
  if (m_epoll_fd != INVALID_DESCRIPTOR) {
    close(m_epoll_fd);
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

bool EPoller::AddReadDescriptor(ReadFileDescriptor *descriptor) {
  if (m_epoll_fd == INVALID_DESCRIPTOR) {
    return false;
  }

  if (!descriptor->ValidReadDescriptor()) {
    OLA_WARN << "AddReadDescriptor called with invalid descriptor";
    return false;
  }

  pair<EPollData*, bool> result = LookupOrCreateDescriptor(
      descriptor->ReadDescriptor());
  if (result.first->events & READ_FLAGS) {
    OLA_WARN << "Descriptor " << descriptor->ReadDescriptor()
             << " already in read set";
    return false;
  }

  result.first->events |= READ_FLAGS;
  result.first->read_descriptor = descriptor;

  if (result.second) {
    return AddEvent(m_epoll_fd, descriptor->ReadDescriptor(), result.first);
  } else {
    return UpdateEvent(m_epoll_fd, descriptor->ReadDescriptor(), result.first);
  }
}

bool EPoller::AddReadDescriptor(ConnectedDescriptor *descriptor,
                                bool delete_on_close) {
  if (m_epoll_fd == INVALID_DESCRIPTOR) {
    return false;
  }

  if (!descriptor->ValidReadDescriptor()) {
    OLA_WARN << "AddReadDescriptor called with invalid descriptor";
    return false;
  }

  pair<EPollData*, bool> result = LookupOrCreateDescriptor(
      descriptor->ReadDescriptor());

  if (result.first->events & READ_FLAGS) {
    OLA_WARN << "Descriptor " << descriptor->ReadDescriptor()
             << " already in read set";
    return false;
  }

  result.first->events |= READ_FLAGS;
  result.first->connected_descriptor = descriptor;
  result.first->delete_connected_on_close = delete_on_close;

  if (result.second) {
    return AddEvent(m_epoll_fd, descriptor->ReadDescriptor(), result.first);
  } else {
    return UpdateEvent(m_epoll_fd, descriptor->ReadDescriptor(), result.first);
  }
}

bool EPoller::RemoveReadDescriptor(ReadFileDescriptor *descriptor) {
  return RemoveDescriptor(descriptor->ReadDescriptor(), READ_FLAGS, true);
}

bool EPoller::RemoveReadDescriptor(ConnectedDescriptor *descriptor) {
  return RemoveDescriptor(descriptor->ReadDescriptor(), READ_FLAGS, true);
}

bool EPoller::AddWriteDescriptor(WriteFileDescriptor *descriptor) {
  if (m_epoll_fd == INVALID_DESCRIPTOR) {
    return false;
  }

  if (!descriptor->ValidWriteDescriptor()) {
    OLA_WARN << "AddWriteDescriptor called with invalid descriptor";
    return false;
  }

  pair<EPollData*, bool> result = LookupOrCreateDescriptor(
      descriptor->WriteDescriptor());

  if (result.first->events & EPOLLOUT) {
    OLA_WARN << "Descriptor " << descriptor->WriteDescriptor()
             << " already in write set";
    return false;
  }

  result.first->events |= EPOLLOUT;
  result.first->write_descriptor = descriptor;

  if (result.second) {
    return AddEvent(m_epoll_fd, descriptor->WriteDescriptor(), result.first);
  } else {
    return UpdateEvent(m_epoll_fd, descriptor->WriteDescriptor(),
                       result.first);
  }
}

bool EPoller::RemoveWriteDescriptor(WriteFileDescriptor *descriptor) {
  return RemoveDescriptor(descriptor->WriteDescriptor(), EPOLLOUT, true);
}

bool EPoller::Poll(TimeoutManager *timeout_manager,
                   const TimeInterval &poll_interval) {
  if (m_epoll_fd == INVALID_DESCRIPTOR) {
    return false;
  }

  epoll_event events[MAX_EVENTS];
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

  int ms_to_sleep = sleep_interval.InMilliSeconds();
  int ready = epoll_wait(m_epoll_fd, reinterpret_cast<epoll_event*>(&events),
                         MAX_EVENTS, ms_to_sleep ? ms_to_sleep : 1);

  if (ready == 0) {
    m_clock->CurrentMonotonicTime(&m_wake_up_time);
    timeout_manager->ExecuteTimeouts(&m_wake_up_time);
    return true;
  } else if (ready == -1) {
    if (errno == EINTR)
      return true;
    OLA_WARN << "epoll() error, " << strerror(errno);
    return false;
  }

  m_clock->CurrentMonotonicTime(&m_wake_up_time);

  for (int i = 0; i < ready; i++) {
    EPollData *descriptor = reinterpret_cast<EPollData*>(
        events[i].data.ptr);
    CheckDescriptor(&events[i], descriptor);
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
void EPoller::CheckDescriptor(struct epoll_event *event,
                              EPollData *epoll_data) {
  if (event->events & (EPOLLHUP | EPOLLRDHUP)) {
    if (epoll_data->read_descriptor) {
      epoll_data->read_descriptor->PerformRead();
    } else if (epoll_data->write_descriptor) {
      epoll_data->write_descriptor->PerformWrite();
    } else if (epoll_data->connected_descriptor) {
      ConnectedDescriptor::OnCloseCallback *on_close =
          epoll_data->connected_descriptor->TransferOnClose();
      if (on_close)
        on_close->Run();

      // At this point the descriptor may be sitting in the orphan list if the
      // OnClose handler called into RemoveReadDescriptor()
      if (epoll_data->delete_connected_on_close &&
          epoll_data->connected_descriptor) {
        bool removed = RemoveDescriptor(
            epoll_data->connected_descriptor->ReadDescriptor(), READ_FLAGS,
            false);
        if (removed && m_export_map) {
          (*m_export_map->GetIntegerVar(K_CONNECTED_DESCRIPTORS_VAR))--;
        }
        delete epoll_data->connected_descriptor;
        epoll_data->connected_descriptor = NULL;
      }
    } else {
      OLA_FATAL << "HUP event for " << epoll_data
                << " but no write or connected descriptor found!";
    }
    event->events = 0;
  }

  if (event->events & EPOLLIN) {
    if (epoll_data->read_descriptor) {
      epoll_data->read_descriptor->PerformRead();
    } else if (epoll_data->connected_descriptor) {
      epoll_data->connected_descriptor->PerformRead();
    }
  }

  if (event->events & EPOLLOUT) {
    // epoll_data->write_descriptor may be null here if this descriptor was
    // removed between when kevent returned and now.
    if (epoll_data->write_descriptor) {
      epoll_data->write_descriptor->PerformWrite();
    }
  }
}

std::pair<EPollData*, bool> EPoller::LookupOrCreateDescriptor(int fd) {
  pair<DescriptorMap::iterator, bool> result = m_descriptor_map.insert(
      DescriptorMap::value_type(fd, NULL));
  bool new_descriptor = result.second;

  if (new_descriptor) {
    if (m_free_descriptors.empty()) {
      result.first->second = new EPollData();
    } else {
      result.first->second = m_free_descriptors.back();
      m_free_descriptors.pop_back();
    }
  }
  return std::make_pair(result.first->second, new_descriptor);
}

bool EPoller::RemoveDescriptor(int fd, int event, bool warn_on_missing) {
  if (fd == INVALID_DESCRIPTOR) {
    OLA_WARN << "Attempt to remove an invalid file descriptor";
    return false;
  }

  EPollData *epoll_data = STLFindOrNull(m_descriptor_map, fd);
  if (!epoll_data) {
    if (warn_on_missing) {
      OLA_WARN << "Couldn't find EPollData for " << fd;
    }
    return false;
  }

  epoll_data->events &= (~event);

  if (event & EPOLLOUT) {
    epoll_data->write_descriptor = NULL;
  } else if (event & EPOLLIN) {
    epoll_data->read_descriptor = NULL;
    epoll_data->connected_descriptor = NULL;
  }

  if (epoll_data->events == 0) {
    RemoveEvent(m_epoll_fd, fd);
    m_orphaned_descriptors.push_back(
        STLLookupAndRemovePtr(&m_descriptor_map, fd));
  } else {
    return UpdateEvent(m_epoll_fd, fd, epoll_data);
  }
  return true;
}
}  // namespace io
}  // namespace ola
