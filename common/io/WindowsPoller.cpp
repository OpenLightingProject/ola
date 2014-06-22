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

#include <string.h>
#include <errno.h>

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <Winsock2.h>


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

namespace ola {
namespace io {

WindowsPoller::WindowsPoller(ExportMap *export_map, Clock* clock)
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
  ConnectedDescriptorSet::iterator iter =
      m_connected_read_descriptors.begin();
  for (; iter != m_connected_read_descriptors.end(); ++iter) {
    if (iter->delete_on_close) {
      delete iter->descriptor;
    }
  }
  m_socket_read_descriptors.clear();
  m_connected_read_descriptors.clear();

  HandleMap::iterator handle_it = m_socket_handles.begin();
  for (; handle_it != m_socket_handles.end(); ++handle_it) {
    CloseHandle(handle_it->second);
  }

  WriteHandleMap::iterator write_handle_it = m_socket_write_handles.begin();
  for (; write_handle_it != m_socket_write_handles.end(); ++write_handle_it) {
    CloseHandle(write_handle_it->second);
  }
}

bool WindowsPoller::AddReadDescriptor(ReadFileDescriptor *descriptor) {
  if (!descriptor->ValidReadDescriptor()) {
    OLA_WARN << "AddReadDescriptor called with invalid descriptor";
    return false;
  }

  ola::io::DescriptorHandle handle = descriptor->ReadDescriptor();

  if (handle.m_type != ola::io::SOCKET_DESCRIPTOR) {
    OLA_WARN << "Handle type " << handle.m_type << "not supported";
    return false;
  }

  bool result = STLInsertIfNotPresent(&m_socket_read_descriptors, descriptor);

  if (result) {
    m_socket_handles.insert(
        std::pair<ReadFileDescriptor*, HANDLE>(descriptor, WSACreateEvent()));
  }

  return result;
}

bool WindowsPoller::AddReadDescriptor(ConnectedDescriptor *descriptor,
                                      bool delete_on_close) {
  if (!descriptor->ValidReadDescriptor()) {
    OLA_WARN << "AddReadDescriptor called with invalid descriptor";
    return false;
  }

  ola::io::DescriptorHandle handle = descriptor->ReadDescriptor();

  if ((handle.m_type != ola::io::PIPE_DESCRIPTOR) &&
      (handle.m_type != ola::io::SOCKET_DESCRIPTOR)) {
    OLA_WARN << "Handle type " << handle.m_type << "not supported";
    return false;
  }

  // We make use of the fact that connected_descriptor_t_lt operates on the
  // descriptor handle value alone.
  connected_descriptor_t registered_descriptor = {descriptor, delete_on_close};

  bool result = STLInsertIfNotPresent(&m_connected_read_descriptors,
                                      registered_descriptor);

  if (result) {
    switch (handle.m_type) {
      case ola::io::SOCKET_DESCRIPTOR:
        {
          m_socket_handles.insert(
              std::pair<ReadFileDescriptor*, HANDLE>(
                  descriptor, WSACreateEvent()));
        }
        break;
      default:
        break;
    }
  }
  return result;
}

bool WindowsPoller::RemoveReadDescriptor(ReadFileDescriptor *descriptor) {
  if (!descriptor->ValidReadDescriptor()) {
    OLA_WARN << "Removing an invalid file descriptor";
    return false;
  }

  ola::io::DescriptorHandle handle = descriptor->ReadDescriptor();

  if (handle.m_type != ola::io::SOCKET_DESCRIPTOR) {
    OLA_WARN << "Handle type " << handle.m_type << "not supported";
    return false;
  }

  HandleMap::iterator iter = m_socket_handles.find(descriptor);
  if (iter != m_socket_handles.end()) {
    CloseHandle(iter->second);
  }

  return STLRemove(&m_socket_read_descriptors, descriptor);
}

bool WindowsPoller::RemoveReadDescriptor(ConnectedDescriptor *descriptor) {
  if (!descriptor->ValidReadDescriptor()) {
    OLA_WARN << "Removing an invalid file descriptor";
    return false;
  }

  switch (descriptor->ReadDescriptor().m_type) {
    case ola::io::SOCKET_DESCRIPTOR:
      {
        HandleMap::iterator iter = m_socket_handles.find(descriptor);
        if (iter != m_socket_handles.end()) {
          CloseHandle(iter->second);
        }
      }
      break;
    default:
        break;
  }

  // Comparison is based on descriptor only, so the second value is redundant.
  connected_descriptor_t registered_descriptor = {descriptor, false};
  return STLRemove(&m_connected_read_descriptors, registered_descriptor);
}

bool WindowsPoller::AddWriteDescriptor(WriteFileDescriptor *descriptor) {
  if (!descriptor->ValidWriteDescriptor()) {
    OLA_WARN << "AddReadDescriptor called with invalid descriptor";
    return false;
  }

  ola::io::DescriptorHandle handle = descriptor->WriteDescriptor();

  if (handle.m_type != ola::io::SOCKET_DESCRIPTOR) {
    OLA_WARN << "Handle type " << handle.m_type << "not supported";
    return false;
  }

  bool result = STLInsertIfNotPresent(&m_socket_write_descriptors, descriptor);

  if (result) {
    m_socket_write_handles.insert(
        std::pair<WriteFileDescriptor*, HANDLE>(descriptor, WSACreateEvent()));
  }

  return result;
}

bool WindowsPoller::RemoveWriteDescriptor(WriteFileDescriptor *descriptor) {
  if (!descriptor->ValidWriteDescriptor()) {
    OLA_WARN << "Removing an invalid file descriptor";
    return false;
  }

  ola::io::DescriptorHandle handle = descriptor->WriteDescriptor();

  if (handle.m_type != ola::io::SOCKET_DESCRIPTOR) {
    OLA_WARN << "Handle type " << handle.m_type << "not supported";
    return false;
  }

  WriteHandleMap::iterator iter = m_socket_write_handles.find(descriptor);
  if (iter != m_socket_write_handles.end()) {
    CloseHandle(iter->second);
  }

  return STLRemove(&m_socket_write_descriptors, descriptor);
}

static void CloseOverlappedEvents(std::vector<OVERLAPPED>& overlapped) {
  std::vector<OVERLAPPED>::iterator iter = overlapped.begin();
  for (; iter != overlapped.end(); ++iter) {
    CloseHandle(iter->hEvent);
  }
}

bool WindowsPoller::Poll(TimeoutManager *timeout_manager,
                        const TimeInterval &poll_interval) {
  TimeStamp now;
  TimeInterval sleep_interval = poll_interval;

  m_clock->CurrentTime(&now);

  // Adjust sleep time for timeouts
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

  // Prepare for polling

  // Check that we don't have too many descriptors
  if ((m_socket_read_descriptors.size() +
       m_connected_read_descriptors.size()) > MAXIMUM_WAIT_OBJECTS) {
    OLA_WARN << "Too many descriptors";
    return false;
  }

  std::vector<HANDLE> handles;
  std::vector<HANDLE> io_handles;
  std::vector<ConnectedDescriptor*> connected_descriptors;
  std::vector<ReadFileDescriptor*> socket_descriptors;
  std::vector<WriteFileDescriptor*> socket_write_descriptors;

  std::vector<OVERLAPPED> overlapped;

  std::queue<ReadFileDescriptor*> read_ready_queue;
  std::queue<WriteFileDescriptor*> write_ready_queue;
  std::queue<connected_descriptor_t> closed_queue;
  bool closed_descriptors = false;

  // Make sure all descriptors are in a valid and up-to-date state
  UpdateDescriptorData();

  // Launch read calls
  bool data_pending = false;
  ConnectedDescriptorSet::iterator chd_iter =
      m_connected_read_descriptors.begin();
  for (; chd_iter != m_connected_read_descriptors.end(); ++chd_iter) {
    DescriptorHandle handle = chd_iter->descriptor->ReadDescriptor();

    // Check if this descriptor should be closed
    if (!chd_iter->descriptor->ValidReadDescriptor()) {
      OLA_WARN << "Closing descriptor " << chd_iter->descriptor;
      closed_descriptors = true;
      closed_queue.push(*chd_iter);
      continue;
    }

    switch (handle.m_type) {
      case ola::io::SOCKET_DESCRIPTOR:
        socket_descriptors.push_back(chd_iter->descriptor);
        break;
      case ola::io::PIPE_DESCRIPTOR:
        {
          if (*(handle.m_read_data_size) > 0) {
            data_pending = true;
          }

          // Async read call to be used with WaitForMultipleObjects
          DWORD bytes_already_in_buffer = *handle.m_read_data_size;
          void* read_buffer = &(handle.m_read_data[bytes_already_in_buffer]);
          DWORD bytes_to_read = READ_DATA_BUFFER_SIZE - bytes_already_in_buffer;

          OVERLAPPED over;
          memset(&over, 0, sizeof(over));
          overlapped.push_back(over);
          POVERLAPPED pover = &(overlapped[overlapped.size() - 1]);

          pover->hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

          if (!ReadFile(handle.m_handle.m_handle,
                        read_buffer,
                        bytes_to_read,
                        NULL,
                        pover)) {
            int last_error = GetLastError();
            if (last_error != ERROR_IO_PENDING) {
              // Detect closed pipe descriptor
              if (last_error == ERROR_BROKEN_PIPE) {
                closed_queue.push(*chd_iter);
                closed_descriptors = true;
              } else {
                OLA_WARN << "ReadFile failed with " << last_error << " for "
                         << handle.m_handle.m_handle;
              }
              continue;
            }
          }

          handles.push_back(pover->hEvent);
          io_handles.push_back(handle.m_handle.m_handle);
          connected_descriptors.push_back(chd_iter->descriptor);
        }
        break;
      default:
        continue;
    }
  }

  // Collect and call select() on sockets
  // First, all sockets registered for reading
  {
    SocketDescriptorSet::iterator iter = m_socket_read_descriptors.begin();
    for (; iter != m_socket_read_descriptors.end(); ++iter) {
      socket_descriptors.push_back(*iter);
    }

    std::vector<ReadFileDescriptor*>::iterator socket_iter =
        socket_descriptors.begin();
    for (; socket_iter != socket_descriptors.end(); ++socket_iter) {
      DescriptorHandle handle = (*socket_iter)->ReadDescriptor();
      HandleMap::iterator event = m_socket_handles.find(*socket_iter);
      if (event == m_socket_handles.end()) {
        OLA_WARN << "No event found for socket " << handle.m_handle.m_fd;
        continue;
      }
      int32_t events = FD_READ | FD_ACCEPT | FD_CLOSE;
      if (WSAEventSelect(handle.m_handle.m_fd, event->second, events) != 0) {
        OLA_WARN << "WSAEventSelect failed with " << WSAGetLastError() <<
            " for socket " << handle.m_handle.m_fd;
        continue;
      }

      handles.push_back(event->second);
    }
  }
  // Now, all sockets registered for writing
  {
    SocketWriteDescriptorSet::iterator iter =
        m_socket_write_descriptors.begin();
    for (; iter != m_socket_write_descriptors.end(); ++iter) {
      socket_write_descriptors.push_back(*iter);
      DescriptorHandle handle = (*iter)->WriteDescriptor();
      WriteHandleMap::iterator event = m_socket_write_handles.find(*iter);
      if (event == m_socket_write_handles.end()) {
        OLA_WARN << "No write event found for socket " << handle.m_handle.m_fd;
        continue;
      }

      int32_t events = FD_WRITE | FD_CONNECT | FD_CLOSE;
      if (WSAEventSelect(handle.m_handle.m_fd, event->second, events) != 0) {
        OLA_WARN << "WSAEventSelect failed with " << WSAGetLastError() <<
            " for socket " << handle.m_handle.m_fd;
        continue;
      }

      handles.push_back(event->second);
    }
  }

  // If there are closed descriptors or descriptors with pending data, set the
  // timeout to something very small (1ms). This ensures we at least make a pass
  // through the descriptors.
  if (closed_descriptors || data_pending) {
    sleep_interval = std::min(sleep_interval, TimeInterval(0, 1000));
  }

  // Wait on all events
  DWORD wait_result = WaitForMultipleObjects(handles.size(),
                                             handles.data(),
                                             FALSE,
                                             sleep_interval.InMilliSeconds());
  bool return_result = true;

  switch (wait_result) {
    case WAIT_TIMEOUT:
      // Cancel IO
      {
        std::vector<HANDLE>::iterator io = io_handles.begin();
        for (; io != io_handles.end(); ++io) {
          CancelIo(*io);
        }
      }
      io_handles.clear();
      // timeout
      m_clock->CurrentTime(&m_wake_up_time);
      timeout_manager->ExecuteTimeouts(&m_wake_up_time);
      break;
    case WAIT_FAILED:
    // Cancel IO
      {
        std::vector<HANDLE>::iterator io = io_handles.begin();
        for (; io != io_handles.end(); ++io) {
          CancelIo(*io);
        }
      }
      io_handles.clear();
      OLA_WARN << "WaitForMultipleObjects() failed with " << GetLastError();
      return_result = false;
      break;
    default:
      m_clock->CurrentTime(&m_wake_up_time);
      DWORD index = wait_result - WAIT_OBJECT_0;
      do {
        if (index < connected_descriptors.size()) {
          CancelIo(io_handles[index]);
          io_handles.erase(io_handles.begin() + index);
          ConnectedDescriptor* descriptor = connected_descriptors[index];
          uint32_t* bytes_transferred =
              descriptor->ReadDescriptor().m_read_call_size;
              POVERLAPPED pover = &(overlapped[index]);
          if (!GetOverlappedResult(handles[index], pover,
              reinterpret_cast<DWORD*>(bytes_transferred), FALSE)) {
            OLA_WARN << "GetOverlappedResult failed with " << GetLastError();
          }
          read_ready_queue.push(descriptor);
          handles.erase(handles.begin() + index);
          CloseHandle(overlapped[index].hEvent);
          overlapped.erase(overlapped.begin() + index);
          connected_descriptors.erase(connected_descriptors.begin() + index);
        } else if (index < connected_descriptors.size() +
                           socket_descriptors.size()) {
          DWORD socket_index = index - connected_descriptors.size();
          ReadFileDescriptor* descriptor = socket_descriptors[socket_index];
          WSANETWORKEVENTS events;
          WSAEnumNetworkEvents(descriptor->ReadDescriptor().m_handle.m_fd,
              handles[index], &events);
          if ((events.lNetworkEvents & FD_READ) ||
              (events.lNetworkEvents & FD_ACCEPT)) {
            read_ready_queue.push(descriptor);
          } else if (events.lNetworkEvents & FD_CLOSE) {
            SocketDescriptorSet::iterator socket_iter =
                m_socket_read_descriptors.find(descriptor);
            if (socket_iter != m_socket_read_descriptors.end()) {
              // This is a ReadFileDescriptor based socket, just remove it
              m_socket_read_descriptors.erase(socket_iter);
              HandleMap::iterator iter = m_socket_handles.find(descriptor);
              if (iter != m_socket_handles.end()) {
                CloseHandle(iter->second);
                m_socket_handles.erase(iter);
              }
            } else {
              // It's a ConnectedDescriptor, add it to the close list
              ConnectedDescriptorSet::iterator iter =
                  m_connected_read_descriptors.begin();
              for (; iter != m_connected_read_descriptors.end(); ++iter) {
                if (iter->descriptor == descriptor) {
                  closed_queue.push(*iter);
                  m_connected_read_descriptors.erase(iter);
                  break;
                }
              }
            }
          }
          handles.erase(handles.begin() + index);
          socket_descriptors.erase(socket_descriptors.begin() + socket_index);
        } else if (index < connected_descriptors.size() +
                           socket_descriptors.size() +
                           socket_write_descriptors.size()) {
          DWORD socket_index = index - connected_descriptors.size() -
              socket_descriptors.size();
          WriteFileDescriptor* descriptor =
              socket_write_descriptors[socket_index];
          WSANETWORKEVENTS events;
          WSAEnumNetworkEvents(descriptor->WriteDescriptor().m_handle.m_fd,
              handles[index], &events);
          if ((events.lNetworkEvents & FD_WRITE) ||
              (events.lNetworkEvents & FD_CONNECT)) {
                write_ready_queue.push(descriptor);
          } else if (events.lNetworkEvents & FD_CLOSE) {
            SocketWriteDescriptorSet::iterator socket_iter =
                m_socket_write_descriptors.find(descriptor);
            if (socket_iter != m_socket_write_descriptors.end()) {
              m_socket_write_descriptors.erase(socket_iter);
              WriteHandleMap::iterator iter =
                  m_socket_write_handles.find(descriptor);
              if (iter != m_socket_write_handles.end()) {
                CloseHandle(iter->second);
                m_socket_write_handles.erase(iter);
              }
            }
          }
          handles.erase(handles.begin() + index);
          socket_write_descriptors.erase(socket_write_descriptors.begin() +
              socket_index);
        } else {
          // TODO(lukase)
          OLA_WARN << "Unkown index";
        }
        // Check for other signalled events
        wait_result = WaitForMultipleObjects(handles.size(),
                                             handles.data(),
                                             FALSE,
                                             0);
      } while ((wait_result != WAIT_TIMEOUT) && (wait_result != WAIT_FAILED));

      // Cancel IO
      {
        std::vector<HANDLE>::iterator io = io_handles.begin();
        for (; io != io_handles.end(); ++io) {
          CancelIo(*io);
        }
      }
      io_handles.clear();

      m_clock->CurrentTime(&m_wake_up_time);
      timeout_manager->ExecuteTimeouts(&m_wake_up_time);
      break;
  }

  UpdateDescriptorData();

  // Add all descriptors with pending data to the queue
  std::vector<ConnectedDescriptor*>::iterator pending_iter =
      connected_descriptors.begin();
  for (; pending_iter != connected_descriptors.end(); ++pending_iter) {
    DescriptorHandle descriptor = (*pending_iter)->ReadDescriptor();
    if (*(descriptor.m_read_data_size) > 0) {
      read_ready_queue.push(*pending_iter);
    }
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

  // Close descriptors
  chd_iter = m_connected_read_descriptors.begin();
  while (chd_iter != m_connected_read_descriptors.end()) {
    ConnectedDescriptorSet::iterator this_iter = chd_iter;
    ++chd_iter;
    if (!this_iter->descriptor->ValidReadDescriptor()) {
      closed_queue.push(*this_iter);
      m_connected_read_descriptors.erase(this_iter);
    }
    if ((this_iter->descriptor->ReadDescriptor().m_type ==
         ola::io::SOCKET_DESCRIPTOR) &&
        (this_iter->descriptor->IsClosed())) {
      closed_queue.push(*this_iter);
      m_connected_read_descriptors.erase(this_iter);
    }
  }

  while (!closed_queue.empty()) {
    const connected_descriptor_t &connected_descriptor = closed_queue.front();
    ConnectedDescriptor* descriptor = connected_descriptor.descriptor;
    ConnectedDescriptor::OnCloseCallback *on_close =
        descriptor->TransferOnClose();
    if (on_close)
      on_close->Run();
    switch (descriptor->ReadDescriptor().m_type) {
      case ola::io::SOCKET_DESCRIPTOR:
        {
          HandleMap::iterator iter = m_socket_handles.find(descriptor);
          if (iter != m_socket_handles.end()) {
            CloseHandle(iter->second);
            m_socket_handles.erase(iter);
          }
        }
        break;
      default:
        break;
    }
    if (connected_descriptor.delete_on_close)
      delete descriptor;
    if (m_export_map) {
      (*m_export_map->GetIntegerVar(K_CONNECTED_DESCRIPTORS_VAR))--;
    }
    closed_queue.pop();
  }

  CloseOverlappedEvents(overlapped);

  return return_result;
}

void WindowsPoller::UpdateDescriptorData() {
  ConnectedDescriptorSet::iterator iter = m_connected_read_descriptors.begin();
  for (; iter != m_connected_read_descriptors.end(); ++iter) {
    DescriptorHandle descriptor = iter->descriptor->ReadDescriptor();
    switch (descriptor.m_type) {
      case ola::io::PIPE_DESCRIPTOR:
        if (*(descriptor.m_read_call_size) > 0) {
          *(descriptor.m_read_data_size) += *(descriptor.m_read_call_size);
          *(descriptor.m_read_call_size) = 0;
        }
      default:
        break;
    }
  }
}

}  // namespace io
}  // namespace ola
