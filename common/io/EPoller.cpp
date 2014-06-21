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

/**
 * @brief The maximum number of events to return in one epoll cycle
 */
const int EPoller::MAX_EVENTS = 10;

/**
 * @brief the EPOLL flags used for read descriptors.
 */
const int EPoller::READ_FLAGS = EPOLLIN | EPOLLRDHUP;

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

  STLDeleteValues(&m_descriptor_map);
}

bool EPoller::AddReadDescriptor(ReadFileDescriptor *descriptor) {
  if (m_epoll_fd == INVALID_DESCRIPTOR) {
    return false;
  }

  if (!descriptor->ValidReadDescriptor()) {
    OLA_WARN << "AddReadDescriptor called with invalid descriptor";
    return false;
  }

  pair<EPollDescriptor*, bool> result = LookupOrCreateDescriptor(
      descriptor->ReadDescriptor());
  if (result.first->events & READ_FLAGS) {
    OLA_WARN << "Descriptor " << descriptor->ReadDescriptor()
             << " already in read set";
    return false;
  }

  result.first->events |= READ_FLAGS;
  result.first->read_descriptor = descriptor;

  if (result.second) {
    return AddEvent(descriptor->ReadDescriptor(), result.first);
  } else {
    return UpdateEvent(descriptor->ReadDescriptor(), result.first);
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

  pair<EPollDescriptor*, bool> result = LookupOrCreateDescriptor(
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
    return AddEvent(descriptor->ReadDescriptor(), result.first);
  } else {
    return UpdateEvent(descriptor->ReadDescriptor(), result.first);
  }
}

bool EPoller::RemoveReadDescriptor(ReadFileDescriptor *descriptor) {
  OLA_INFO << "Call to remove " << descriptor->ReadDescriptor();
  return RemoveDescriptor(descriptor->ReadDescriptor(), READ_FLAGS);
}

bool EPoller::RemoveReadDescriptor(ConnectedDescriptor *descriptor) {
  OLA_INFO << "Call to remove " << descriptor->ReadDescriptor();
  return RemoveDescriptor(descriptor->ReadDescriptor(), READ_FLAGS);
}

bool EPoller::AddWriteDescriptor(WriteFileDescriptor *descriptor) {
  if (m_epoll_fd == INVALID_DESCRIPTOR) {
    return false;
  }

  if (!descriptor->ValidWriteDescriptor()) {
    OLA_WARN << "AddWriteDescriptor called with invalid descriptor";
    return false;
  }

  OLA_INFO << "checking for fd " << descriptor->WriteDescriptor();
  pair<EPollDescriptor*, bool> result = LookupOrCreateDescriptor(
      descriptor->WriteDescriptor());

  if (result.first->events & EPOLLOUT) {
    OLA_WARN << "Descriptor " << descriptor->WriteDescriptor()
             << " already in write set";
    return false;
  }

  OLA_INFO << "marking " << descriptor->WriteDescriptor()  << " for EPOLLOUT ("
    << EPOLLOUT << ")";
  result.first->events |= EPOLLOUT;
  result.first->write_descriptor = descriptor;

  if (result.second) {
    return AddEvent(descriptor->WriteDescriptor(), result.first);
  } else {
    return UpdateEvent(descriptor->WriteDescriptor(), result.first);
  }
}

bool EPoller::RemoveWriteDescriptor(WriteFileDescriptor *descriptor) {
  OLA_INFO << "Call to remove write for " << descriptor->WriteDescriptor();
  return RemoveDescriptor(descriptor->WriteDescriptor(), EPOLLOUT);
}

bool EPoller::Poll(TimeoutManager *timeout_manager,
                   const TimeInterval &poll_interval) {
  if (m_epoll_fd == INVALID_DESCRIPTOR) {
    return false;
  }

  epoll_event events[MAX_EVENTS];
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

  int ms_to_sleep = sleep_interval.InMilliSeconds();
  int ready = epoll_wait(m_epoll_fd, reinterpret_cast<epoll_event*>(&events),
                         MAX_EVENTS, ms_to_sleep ? ms_to_sleep : 1);

  if (ready == 0) {
    m_clock->CurrentTime(&m_wake_up_time);
    timeout_manager->ExecuteTimeouts(&m_wake_up_time);
    return true;
  } else if (ready == -1) {
    if (errno == EINTR)
      return true;
    OLA_WARN << "epoll() error, " << strerror(errno);
    return false;
  }

  m_clock->CurrentTime(&m_wake_up_time);

  OLA_INFO << "CheckDescriptors, num " << ready;
  for (int i = 0; i < ready; i++) {
    EPollDescriptor *descriptor = reinterpret_cast<EPollDescriptor*>(
        events[i].data.ptr);
    CheckDescriptor(&events[i], descriptor);
  }
  m_clock->CurrentTime(&m_wake_up_time);
  timeout_manager->ExecuteTimeouts(&m_wake_up_time);

  return true;
}


/*
 * Check all the registered descriptors:
 *  - Execute the callback for descriptors with data
 *  - Excute OnClose if a remote end closed the connection
 */
void EPoller::CheckDescriptor(struct epoll_event *event,
                              EPollDescriptor *descriptor) {
  OLA_INFO << "Events for " << descriptor << " are "
           << std::hex << event->events;
  if (event->events & (EPOLLHUP | EPOLLRDHUP)) {
    if (descriptor->write_descriptor) {
      OLA_INFO << "doing write for " << descriptor;
      descriptor->write_descriptor->PerformWrite();
    } else if (descriptor->connected_descriptor) {
      OLA_INFO << descriptor->connected_descriptor
               << " has been closed, delete_connected_on_close is "
               << descriptor->delete_connected_on_close;
      ConnectedDescriptor::OnCloseCallback *on_close =
          descriptor->connected_descriptor->TransferOnClose();
      if (on_close)
        on_close->Run();
      if (descriptor->delete_connected_on_close) {
        if (RemoveReadDescriptor(descriptor->connected_descriptor) &&
            m_export_map) {
          (*m_export_map->GetIntegerVar(K_CONNECTED_DESCRIPTORS_VAR))--;
        }
        delete descriptor->connected_descriptor;
        descriptor->connected_descriptor = NULL;
      }
    } else {
      OLA_FATAL << "HUP event for " << descriptor
                << " but no write or connected descriptor found!";
    }
    event->events = 0;
  }

  if (event->events & EPOLLIN) {
    if (descriptor->read_descriptor) {
      descriptor->read_descriptor->PerformRead();
    } else if (descriptor->connected_descriptor) {
      descriptor->connected_descriptor->PerformRead();
    }
  }

  if (event->events & EPOLLOUT) {
    if (descriptor->write_descriptor) {
      OLA_INFO << "doing write for " << descriptor;
      descriptor->write_descriptor->PerformWrite();
    } else {
      OLA_FATAL << "EPOLLOUT active but write_descriptor is NULL";
    }
  }
}

std::pair<EPoller::EPollDescriptor*, bool> EPoller::LookupOrCreateDescriptor(
    int fd) {
  pair<DescriptorMap::iterator, bool> result = m_descriptor_map.insert(
      DescriptorMap::value_type(fd, NULL));
  bool new_descriptor = result.second;

  if (new_descriptor) {
    result.first->second = new EPollDescriptor();
    OLA_DEBUG << "Created EPollDescriptor " << result.first->second
              << " for fd " << fd;
  }
  return std::make_pair(result.first->second, new_descriptor);
}

bool EPoller::AddEvent(int fd, EPollDescriptor *descriptor) {
  epoll_event event;
  event.events = descriptor->events;
  event.data.ptr = descriptor;

  OLA_INFO << "EPOLL_CTL_ADD " << fd << "descriptor: " << descriptor;
  int r = epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, fd, &event);
  if (r) {
    OLA_WARN << "EPOLL_CTL_ADD " << fd << " failed: " << strerror(errno);
    return false;
  }
  return true;
}

bool EPoller::UpdateEvent(int fd, EPollDescriptor *descriptor) {
  epoll_event event;
  event.events = descriptor->events;
  event.data.ptr = descriptor;

  OLA_INFO << "EPOLL_CTL_MOD " << fd;
  int r = epoll_ctl(m_epoll_fd, EPOLL_CTL_MOD, fd, &event);
  if (r) {
    OLA_WARN << "EPOLL_CTL_MOD " << fd << " failed: " << strerror(errno);
    return false;
  }
  return true;
}

bool EPoller::RemoveEvent(int fd) {
  epoll_event event;
  int r = epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, fd, &event);
  if (r) {
    OLA_WARN << "EPOLL_CTL_DEL " << fd << " failed: " << strerror(errno);
    return false;
  }
  return true;
}

bool EPoller::RemoveDescriptor(int fd, int event) {
  if (fd == INVALID_DESCRIPTOR) {
    OLA_WARN << "Removing an invalid file descriptor";
    return false;
  }

  EPollDescriptor *epoll_descriptor = STLFindOrNull(m_descriptor_map, fd);
  if (!epoll_descriptor) {
    OLA_WARN << "Descriptor " << fd << " not found";
    return false;
  }

  epoll_descriptor->events &= (~event);
  if (epoll_descriptor->events == 0) {
    OLA_DEBUG << "Removed descriptor " << epoll_descriptor;
    RemoveEvent(fd);
    STLRemoveAndDelete(&m_descriptor_map, fd);
  } else {
    return UpdateEvent(fd, epoll_descriptor);
  }
  return true;
}
}  // namespace io
}  // namespace ola
