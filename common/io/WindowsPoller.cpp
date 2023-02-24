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
 * WindowsPoller.cpp
 * A Poller for the Windows platform
 * Copyright (C) 2014 Lukas Erlinghagen
 */

#include "common/io/WindowsPoller.h"

#if HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H

#include <string.h>
#include <errno.h>

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <ola/win/CleanWinSock2.h>

#if HAVE_WINERROR_H
#include <winerror.h>
#endif  // HAVE_WINERROR_H

#include <algorithm>
#include <queue>
#include <string>
#include <utility>
#include <vector>

#include "ola/Clock.h"
#include "ola/Logging.h"
#include "ola/base/Macro.h"
#include "ola/io/Descriptor.h"
#include "ola/stl/STLUtils.h"

using std::map;
using std::pair;
using std::vector;

namespace ola {
namespace io {

//////////////////////////////////////////////////////////////////////////////

static const int FLAG_READ = 1;
static const int FLAG_WRITE = 2;

class WindowsPollerDescriptor {
 public:
  WindowsPollerDescriptor()
      : read_descriptor(NULL),
        write_descriptor(NULL),
        connected_descriptor(NULL),
        delete_connected_on_close(false),
        type(GENERIC_DESCRIPTOR),
        flags(0) {
  }
  ReadFileDescriptor *read_descriptor;
  WriteFileDescriptor *write_descriptor;
  ConnectedDescriptor *connected_descriptor;
  bool delete_connected_on_close;
  DescriptorType type;
  int flags;
};

//////////////////////////////////////////////////////////////////////////////

class EventHolder {
 public:
  EventHolder()
      : event(CreateEvent(NULL, FALSE, FALSE, NULL)) {
  }

  ~EventHolder() {
    CloseHandle(event);
  }

  operator HANDLE() {
    return event;
  }

 private:
  HANDLE event;
};

//////////////////////////////////////////////////////////////////////////////

WindowsPoller::WindowsPoller(ExportMap *export_map, Clock *clock)
    : m_export_map(export_map),
      m_loop_iterations(NULL),
      m_loop_time(NULL),
      m_clock(clock) {
  if (m_export_map) {
    m_loop_time = m_export_map->GetCounterVar(K_LOOP_TIME);
    m_loop_iterations = m_export_map->GetCounterVar(K_LOOP_COUNT);
  }
}

WindowsPoller::~WindowsPoller() {
  {
    DescriptorMap::iterator iter = m_descriptor_map.begin();
    for (; iter != m_descriptor_map.end(); ++iter) {
      if (iter->second->delete_connected_on_close) {
        delete iter->second->connected_descriptor;
      }
      delete iter->second;
    }
  }

  OrphanedDescriptors::iterator iter = m_orphaned_descriptors.begin();
  for (; iter != m_orphaned_descriptors.end(); ++iter) {
    if ((*iter)->delete_connected_on_close) {
      delete (*iter)->connected_descriptor;
    }
    delete *iter;
  }
}

bool WindowsPoller::AddReadDescriptor(ReadFileDescriptor *descriptor) {
  if (!descriptor->ValidReadDescriptor()) {
    OLA_WARN << "AddReadDescriptor called with invalid descriptor";
    return false;
  }

  pair<WindowsPollerDescriptor*, bool> result = LookupOrCreateDescriptor(
      ToHandle(descriptor->ReadDescriptor()));
  if (result.first->flags & FLAG_READ) {
    OLA_WARN << "Descriptor " << descriptor->ReadDescriptor()
             << " already in read set";
    return false;
  }

  result.first->flags |= FLAG_READ;
  result.first->read_descriptor = descriptor;
  result.first->type = descriptor->ReadDescriptor().m_type;

  return result.second ? true : false;
}

bool WindowsPoller::AddReadDescriptor(ConnectedDescriptor *descriptor,
                                      bool delete_on_close) {
  if (!descriptor->ValidReadDescriptor()) {
    OLA_WARN << "AddReadDescriptor called with invalid descriptor";
    return false;
  }
  pair<WindowsPollerDescriptor*, bool> result = LookupOrCreateDescriptor(
      ToHandle(descriptor->ReadDescriptor()));
  if (result.first->flags & FLAG_READ) {
    OLA_WARN << "Descriptor " << descriptor->ReadDescriptor()
             << " already in read set";
    return false;
  }

  result.first->flags |= FLAG_READ;
  result.first->connected_descriptor = descriptor;
  result.first->type = descriptor->ReadDescriptor().m_type;
  result.first->delete_connected_on_close = delete_on_close;

  return (result.second)? true : false;
}

bool WindowsPoller::RemoveReadDescriptor(ReadFileDescriptor *descriptor) {
  return RemoveDescriptor(descriptor->ReadDescriptor(),
                          FLAG_READ,
                          true);
}

bool WindowsPoller::RemoveReadDescriptor(ConnectedDescriptor *descriptor) {
  return RemoveDescriptor(descriptor->ReadDescriptor(),
                          FLAG_READ,
                          true);
}

bool WindowsPoller::AddWriteDescriptor(WriteFileDescriptor *descriptor) {
  if (!descriptor->ValidWriteDescriptor()) {
    OLA_WARN << "AddWriteDescriptor called with invalid descriptor";
    return false;
  }

  if ((descriptor->WriteDescriptor().m_type != SOCKET_DESCRIPTOR) &&
      (descriptor->WriteDescriptor().m_type != PIPE_DESCRIPTOR)) {
    OLA_WARN << "Cannot add descriptor " << descriptor << " for writing.";
    return false;
  }

  pair<WindowsPollerDescriptor*, bool> result = LookupOrCreateDescriptor(
      ToHandle(descriptor->WriteDescriptor()));

  if (result.first->flags & FLAG_WRITE) {
    OLA_WARN << "Descriptor " << descriptor->WriteDescriptor()
             << " already in write set";
    return false;
  }

  result.first->flags |= FLAG_WRITE;
  result.first->write_descriptor = descriptor;
  result.first->type = descriptor->WriteDescriptor().m_type;

  return (result.second)? true : false;
}

bool WindowsPoller::RemoveWriteDescriptor(WriteFileDescriptor *descriptor) {
  return RemoveDescriptor(descriptor->WriteDescriptor(),
                          FLAG_WRITE,
                          true);
}

//////////////////////////////////////////////////////////////////////////////
class PollData {
 public:
  PollData(HANDLE event, HANDLE handle, bool read)
    : event(event),
      handle(handle),
      buffer(NULL),
      size(0),
      overlapped(NULL),
      read(read) {
  }

  ~PollData() {
    if (buffer) {
      delete[] buffer;
      buffer = NULL;
    }

    if (overlapped) {
      delete overlapped;
      overlapped = NULL;
    }
  }

  bool AllocBuffer(DWORD size) {
    if (buffer) {
      OLA_WARN << "Buffer already allocated";
      return false;
    }

    buffer = new char[size];
    this->size = size;
    return true;
  }

  bool CreateOverlapped() {
    if (overlapped) {
      OLA_WARN << "Overlapped already allocated";
      return false;
    }

    overlapped = new OVERLAPPED;
    memset(overlapped, 0, sizeof(*overlapped));
    overlapped->hEvent = event;
    return true;
  }

  HANDLE event;
  HANDLE handle;
  char* buffer;
  DWORD size;
  OVERLAPPED* overlapped;
  bool read;
};

void CancelIOs(vector<PollData*>* data) {
  vector<PollData*>::iterator iter = data->begin();
  for (; iter != data->end(); ++iter) {
    PollData* poll_data = *iter;
    if (poll_data->overlapped) {
      CancelIo(poll_data->handle);
    }
  }
}

bool WindowsPoller::Poll(TimeoutManager *timeout_manager,
                         const TimeInterval &poll_interval) {
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

  // Prepare events
  vector<HANDLE> events;
  vector<PollData*> data;
  vector<EventHolder*> event_holders;

  DescriptorMap::iterator next, iter = m_descriptor_map.begin();
  bool success = true;

  // We're not using a for loop here since we might call RemoveDescriptor(),
  // thereby invalidating the 'iter' iterator.
  while (iter != m_descriptor_map.end()) {
    next = iter;
    ++next;

    WindowsPollerDescriptor* descriptor = iter->second;
    PollData* poll_data = NULL;
    DWORD result = 0;
    DescriptorHandle descriptor_handle;
    EventHolder* event_holder;

    switch (descriptor->type) {
      case PIPE_DESCRIPTOR:
        if (descriptor->connected_descriptor) {
          descriptor_handle =
              descriptor->connected_descriptor->ReadDescriptor();
          event_holder = new EventHolder();
          poll_data = new PollData(*event_holder, ToHandle(descriptor_handle),
              true);
          poll_data->AllocBuffer(ASYNC_DATA_BUFFER_SIZE);
          poll_data->CreateOverlapped();

          success = ReadFile(poll_data->handle,
                             poll_data->buffer,
                             poll_data->size,
                             &(poll_data->size),
                             poll_data->overlapped);
          result = GetLastError();
          if (success || result == ERROR_IO_PENDING) {
            data.push_back(poll_data);
            events.push_back(poll_data->event);
            event_holders.push_back(event_holder);
          } else if (!success && (result != ERROR_IO_PENDING)) {
            if (result == ERROR_BROKEN_PIPE) {
              OLA_DEBUG << "Broken pipe: " << ToHandle(descriptor_handle);
              // Pipe was closed, so close the descriptor
              ConnectedDescriptor::OnCloseCallback *on_close =
                descriptor->connected_descriptor->TransferOnClose();
              if (on_close)
                on_close->Run();
              if (descriptor->connected_descriptor) {
                if (descriptor->delete_connected_on_close) {
                  if (RemoveReadDescriptor(descriptor->connected_descriptor) &&
                      m_export_map) {
                    (*m_export_map->GetIntegerVar(
                        K_CONNECTED_DESCRIPTORS_VAR))--;
                  }
                  delete descriptor->connected_descriptor;
                  descriptor->connected_descriptor = NULL;
                }
              }
              delete poll_data;
              delete event_holder;
            } else {
              OLA_WARN << "ReadFile failed with " << result << " for "
                       << ToHandle(descriptor_handle);
              delete poll_data;
              delete event_holder;
            }
          }
        }
        if (descriptor->write_descriptor) {
          descriptor_handle =
              descriptor->write_descriptor->WriteDescriptor();
          event_holder = new EventHolder();
          poll_data = new PollData(*event_holder, ToHandle(descriptor_handle),
              false);
          poll_data->AllocBuffer(1);
          poll_data->CreateOverlapped();

          success = WriteFile(poll_data->handle,
                             poll_data->buffer,
                             poll_data->size,
                             &(poll_data->size),
                             poll_data->overlapped);
          result = GetLastError();
          if (success || (result == ERROR_IO_PENDING)) {
            data.push_back(poll_data);
            events.push_back(poll_data->event);
            event_holders.push_back(event_holder);
          } else if (result == ERROR_BROKEN_PIPE) {
              OLA_DEBUG << "Broken pipe: " << ToHandle(descriptor_handle);
              // Pipe was closed, so close the descriptor
              descriptor->write_descriptor = NULL;
              delete poll_data;
              delete event_holder;
          } else {
              OLA_WARN << "WriteFile failed with " << result << " for "
                       << ToHandle(descriptor_handle);
              delete poll_data;
              delete event_holder;
          }
        }
        break;
      case SOCKET_DESCRIPTOR:
        if (descriptor->connected_descriptor || descriptor->read_descriptor) {
          // select() for readable events
          if (descriptor->connected_descriptor) {
            descriptor_handle =
                descriptor->connected_descriptor->ReadDescriptor();
          } else {
            descriptor_handle = descriptor->read_descriptor->ReadDescriptor();
          }
          event_holder = new EventHolder();
          poll_data = new PollData(*event_holder, ToHandle(descriptor_handle),
              true);
          if (WSAEventSelect(ToFD(descriptor_handle),
                             *event_holder,
                             FD_READ | FD_CLOSE | FD_ACCEPT) != 0) {
            OLA_WARN << "WSAEventSelect failed with " << WSAGetLastError();
            delete poll_data;
            delete event_holder;
          } else {
            data.push_back(poll_data);
            events.push_back(poll_data->event);
            event_holders.push_back(event_holder);
          }
        }
        if (descriptor->write_descriptor) {
          // select() for writeable events
          descriptor_handle = descriptor->write_descriptor->WriteDescriptor();
          event_holder = new EventHolder();
          poll_data = new PollData(*event_holder, ToHandle(descriptor_handle),
              false);
          if (WSAEventSelect(ToFD(descriptor_handle),
                             *event_holder,
                             FD_WRITE | FD_CLOSE | FD_CONNECT) != 0) {
            OLA_WARN << "WSAEventSelect failed with " << WSAGetLastError();
            delete poll_data;
            delete event_holder;
          } else {
            data.push_back(poll_data);
            events.push_back(poll_data->event);
            event_holders.push_back(event_holder);
          }
        }
        break;
      default:
        OLA_WARN << "Descriptor type not implemented: " << descriptor->type;
        break;
    }

    iter = next;
  }

  bool return_value = true;

  // Wait for events or timeout
  if (events.size() > 0) {
    DWORD result = WaitForMultipleObjectsEx(events.size(),
                                            events.data(),
                                            FALSE,
                                            ms_to_sleep,
                                            TRUE);
    CancelIOs(&data);

    if (result == WAIT_TIMEOUT) {
      m_clock->CurrentMonotonicTime(&m_wake_up_time);
      timeout_manager->ExecuteTimeouts(&m_wake_up_time);
      // We can't return here since any of the cancelled IO calls still might
      // have succeeded.
    } else if (result == WAIT_FAILED) {
      OLA_WARN << "WaitForMultipleObjectsEx failed with " << GetLastError();
      return_value = false;
      // We can't return here since any of the cancelled IO calls still might
      // have succeeded.
    } else if ((result >= WAIT_OBJECT_0) &&
              (result < (WAIT_OBJECT_0 + events.size()))) {
      do {
        DWORD index = result - WAIT_OBJECT_0;
        PollData* poll_data = data[index];
        HandleWakeup(poll_data);

        events.erase(events.begin() + index);
        data.erase(data.begin() + index);
        event_holders.erase(event_holders.begin() + index);

        result = WaitForMultipleObjectsEx(events.size(),
                                          events.data(),
                                          FALSE,
                                          0,
                                          TRUE);
      } while ((result >= WAIT_OBJECT_0) &&
               (result < (WAIT_OBJECT_0 + events.size())));
    } else {
      OLA_WARN << "Unhandled return value from WaitForMultipleObjectsEx: "
               << result;
    }
  } else {
    Sleep(ms_to_sleep);
    CancelIOs(&data);
  }

  m_clock->CurrentMonotonicTime(&m_wake_up_time);
  timeout_manager->ExecuteTimeouts(&m_wake_up_time);

  FinalCheckIOs(data);

  // Loop through all descriptors again and look for any with pending data
  DescriptorMap::iterator map_iter = m_descriptor_map.begin();
  for (; map_iter != m_descriptor_map.end(); ++map_iter) {
    WindowsPollerDescriptor* descriptor = map_iter->second;
    if (!descriptor->connected_descriptor) {
      continue;
    }
    if (descriptor->type != PIPE_DESCRIPTOR) {
      continue;
    }
    DescriptorHandle handle =
        descriptor->connected_descriptor->ReadDescriptor();
    if (*handle.m_async_data_size > 0) {
      descriptor->connected_descriptor->PerformRead();
    }
  }

  STLDeleteElements(&m_orphaned_descriptors);
  STLDeleteElements(&data);
  STLDeleteElements(&event_holders);

  m_clock->CurrentMonotonicTime(&m_wake_up_time);
  timeout_manager->ExecuteTimeouts(&m_wake_up_time);
  return return_value;
}

//////////////////////////////////////////////////////////////////////////////

std::pair<WindowsPollerDescriptor*, bool>
    WindowsPoller::LookupOrCreateDescriptor(void* handle) {
  pair<DescriptorMap::iterator, bool> result = m_descriptor_map.insert(
      DescriptorMap::value_type(handle, NULL));
  bool new_descriptor = result.second;

  if (new_descriptor) {
    result.first->second = new WindowsPollerDescriptor();
    OLA_DEBUG << "Created WindowsPollerDescriptor " << result.first->second
              << " for handle " << handle;
  }
  return std::make_pair(result.first->second, new_descriptor);
}

bool WindowsPoller::RemoveDescriptor(const DescriptorHandle &handle,
                                     int flag,
                                     bool warn_on_missing) {
  if (!handle.IsValid()) {
    OLA_WARN << "Attempt to remove an invalid file descriptor";
    return false;
  }

  WindowsPollerDescriptor *descriptor = STLFindOrNull(m_descriptor_map,
                                                      ToHandle(handle));
  if (!descriptor) {
    if (warn_on_missing) {
      OLA_WARN << "Couldn't find WindowsPollerDescriptor for " << handle;
    }
    return false;
  }

  if (flag & FLAG_READ) {
    descriptor->connected_descriptor = NULL;
    descriptor->read_descriptor = NULL;
  } else if (flag & FLAG_WRITE) {
    descriptor->write_descriptor = NULL;
  }

  descriptor->flags &= ~flag;

  if (descriptor->flags == 0) {
    m_orphaned_descriptors.push_back(
        STLLookupAndRemovePtr(&m_descriptor_map, ToHandle(handle)));
  }
  return true;
}

void WindowsPoller::HandleWakeup(PollData* data) {
  DescriptorMap::iterator iter = m_descriptor_map.find(data->handle);
  if (iter == m_descriptor_map.end()) {
    OLA_WARN << "Descriptor not found for handle " << data->handle;
    return;
  }
  WindowsPollerDescriptor* descriptor = iter->second;

  switch (descriptor->type) {
    case PIPE_DESCRIPTOR:
      {
        if (!data->overlapped) {
          OLA_WARN << "No overlapped entry for pipe descriptor";
          return;
        }
        if (data->read && descriptor->connected_descriptor) {
          if (!descriptor->connected_descriptor->ValidReadDescriptor()) {
            RemoveDescriptor(descriptor->connected_descriptor->ReadDescriptor(),
                             FLAG_READ,
                             false);
            return;
          }

          DescriptorHandle handle =
              descriptor->connected_descriptor->ReadDescriptor();

          DWORD bytes_transferred = 0;
          if (!GetOverlappedResult(data->handle,
                                   data->overlapped,
                                   &bytes_transferred,
                                   TRUE)) {
            if (GetLastError() != ERROR_OPERATION_ABORTED) {
              OLA_WARN << "GetOverlappedResult failed with " << GetLastError();
              return;
            }
          }

          uint32_t to_copy = std::min(static_cast<uint32_t>(bytes_transferred),
              (ASYNC_DATA_BUFFER_SIZE - *handle.m_async_data_size));
          if (to_copy < bytes_transferred) {
            OLA_WARN << "Pipe descriptor has lost data";
          }

          memcpy(&(handle.m_async_data[*handle.m_async_data_size]),
              data->buffer, to_copy);
          *handle.m_async_data_size += to_copy;
          if (*handle.m_async_data_size > 0) {
            descriptor->connected_descriptor->PerformRead();
          }
        } else if (!data->read && descriptor->write_descriptor) {
          OLA_WARN << "Write wakeup";
          if (!descriptor->write_descriptor->ValidWriteDescriptor()) {
            RemoveDescriptor(descriptor->write_descriptor->WriteDescriptor(),
                             FLAG_WRITE,
                             false);
            return;
          }

          descriptor->write_descriptor->PerformWrite();
        } else {
          OLA_WARN << "Overlapped wakeup with data mismatch";
        }
      }
      break;
    case SOCKET_DESCRIPTOR:
      {
        WSANETWORKEVENTS events;
        if (WSAEnumNetworkEvents(reinterpret_cast<SOCKET>(data->handle),
                                 data->event,
                                 & events) != 0) {
          OLA_WARN << "WSAEnumNetworkEvents failed with " << WSAGetLastError();
        } else {
          if (events.lNetworkEvents & (FD_READ | FD_ACCEPT)) {
            if (descriptor->connected_descriptor) {
              descriptor->connected_descriptor->PerformRead();
            } else if (descriptor->read_descriptor) {
              descriptor->read_descriptor->PerformRead();
            } else {
              OLA_WARN << "No read descriptor for socket with read event";
            }
          }

          if (events.lNetworkEvents & (FD_WRITE | FD_CONNECT)) {
            if (descriptor->write_descriptor) {
              descriptor->write_descriptor->PerformWrite();
            } else {
              OLA_WARN << "No write descriptor for socket with write event";
            }
          }

          if (events.lNetworkEvents & FD_CLOSE) {
            if (descriptor->connected_descriptor) {
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
              OLA_WARN << "Close event for " << descriptor
                       << " but no connected descriptor found";
            }
          }
        }
      }
      break;
    default:
      OLA_WARN << "Unhandled descriptor type";
      break;
  }
}

void WindowsPoller::FinalCheckIOs(vector<PollData*> data) {
  vector<PollData*>::iterator iter = data.begin();
  for (; iter != data.end(); ++iter) {
    PollData* poll_data = *iter;
    if (!poll_data->overlapped) {
      // No overlapped input for this descriptor, skip it
      continue;
    }

    DWORD bytes_transferred = 0;
    if (!GetOverlappedResult(poll_data->handle,
                             poll_data->overlapped,
                             &bytes_transferred,
                             TRUE)) {
      if (GetLastError() != ERROR_OPERATION_ABORTED) {
        OLA_WARN << "GetOverlappedResult failed with " << GetLastError();
        return;
      }
    }

    if (bytes_transferred > 0) {
      DescriptorMap::iterator iter = m_descriptor_map.find(poll_data->handle);
      if (iter == m_descriptor_map.end()) {
        OLA_WARN << "Descriptor not found for handle " << poll_data->handle;
        return;
      }
      WindowsPollerDescriptor* descriptor = iter->second;
      DescriptorHandle handle =
        descriptor->connected_descriptor->ReadDescriptor();

      uint32_t to_copy = std::min(static_cast<uint32_t>(bytes_transferred),
          (ASYNC_DATA_BUFFER_SIZE - *handle.m_async_data_size));
      if (to_copy < bytes_transferred) {
        OLA_WARN << "Pipe descriptor has lost data";
      }

      memcpy(&(handle.m_async_data[*handle.m_async_data_size]),
          poll_data->buffer, to_copy);
      *handle.m_async_data_size += to_copy;
      descriptor->connected_descriptor->PerformRead();
    }
  }
}

}  // namespace io
}  // namespace ola
