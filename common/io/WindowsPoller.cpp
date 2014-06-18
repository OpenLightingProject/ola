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
  ConnectedPipeDescriptorSet::iterator iter =
      m_connected_pipe_read_descriptors.begin();
  for (; iter != m_connected_pipe_read_descriptors.end(); ++iter) {
    if (iter->delete_on_close) {
      delete iter->descriptor;
    }
  }
  m_socket_read_descriptors.clear();
  m_connected_pipe_read_descriptors.clear();

  OverlappedHandleMap::iterator overlapped_it = m_overlapped_handle_map.begin();
  for (; overlapped_it != m_overlapped_handle_map.end(); ++overlapped_it) {
    CloseHandle(overlapped_it->second.m_overlapped.hEvent);
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

  return STLInsertIfNotPresent(&m_socket_read_descriptors, descriptor);
}

bool WindowsPoller::AddReadDescriptor(ConnectedDescriptor *descriptor,
                                      bool delete_on_close) {
  if (!descriptor->ValidReadDescriptor()) {
    OLA_WARN << "AddReadDescriptor called with invalid descriptor";
    return false;
  }

  ola::io::DescriptorHandle handle = descriptor->ReadDescriptor();

  if (handle.m_type != ola::io::PIPE_DESCRIPTOR) {
    OLA_WARN << "Handle type " << handle.m_type << "not supported";
    return false;
  }

  // We make use of the fact that connected_descriptor_t_lt operates on the
  // descriptor handle value alone.
  connected_pipe_descriptor_t registered_descriptor =
      {descriptor, delete_on_close};

  bool result = STLInsertIfNotPresent(&m_connected_pipe_read_descriptors,
                                      registered_descriptor);

  if (result) {
    overlapped_handle_context_t context;
    memset(&context, 0, sizeof(context));
    context.m_overlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    m_overlapped_handle_map.insert(
        std::pair<void*, overlapped_handle_context_t>(descriptor, context));
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

  return STLRemove(&m_socket_read_descriptors, descriptor);
}

bool WindowsPoller::RemoveReadDescriptor(ConnectedDescriptor *descriptor) {
  if (!descriptor->ValidReadDescriptor()) {
    OLA_WARN << "Removing an invalid file descriptor";
    return false;
  }

  OverlappedHandleMap::iterator iter = m_overlapped_handle_map.find(descriptor);
  if (iter != m_overlapped_handle_map.end()) {
    CloseHandle(iter->second.m_overlapped.hEvent);
    m_overlapped_handle_map.erase(iter);
  }

  // Comparison is based on descriptor only, so the second value is redundant.
  connected_pipe_descriptor_t registered_descriptor = {descriptor, false};
  return STLRemove(&m_connected_pipe_read_descriptors, registered_descriptor);
}

bool WindowsPoller::AddWriteDescriptor(WriteFileDescriptor *descriptor) {
  // TODO(lukase) Implement this method
  OLA_WARN << "AddWriteDescriptor(WriteFileDescriptor*) not implemented";
  return false;
}

bool WindowsPoller::RemoveWriteDescriptor(WriteFileDescriptor *descriptor) {
  // TODO(lukase) Implement this method
  OLA_WARN << "RemoveWriteDescriptor(WriteFileDescriptor*) not implemented";
  return false;
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
       m_connected_pipe_read_descriptors.size()) > MAXIMUM_WAIT_OBJECTS) {
    OLA_WARN << "Too many descriptors";
    return false;
  }

  std::vector<HANDLE> handles;
  std::vector<HANDLE> io_handles;
  std::vector<ConnectedDescriptor*> connected_descriptors;

  std::queue<ReadFileDescriptor*> read_ready_queue;
  std::queue<connected_pipe_descriptor_t> closed_queue;
  bool closed_descriptors = false;

  // Make sure all descriptors are in a valid and up-to-date state
  UpdateDescriptorData();

  // Launch read calls
  bool data_pending = false;
  ConnectedPipeDescriptorSet::iterator chd_iter =
      m_connected_pipe_read_descriptors.begin();
  for (; chd_iter != m_connected_pipe_read_descriptors.end(); ++chd_iter) {
    OverlappedHandleMap::iterator overlapped =
        m_overlapped_handle_map.find(chd_iter->descriptor);
    if (overlapped == m_overlapped_handle_map.end()) {
      OLA_WARN << "No overlapped entry for descriptor " << chd_iter->descriptor;
      continue;
    }

    // Check if this descriptor should be closed
    if (!chd_iter->descriptor->ValidReadDescriptor()) {
      closed_descriptors = true;
      closed_queue.push(*chd_iter);
      continue;
    }

    DescriptorHandle handle = chd_iter->descriptor->ReadDescriptor();
    
    if (*(handle.m_read_data_size) > 0) {
      data_pending = true;
    }

    // Async read call to be used with WaitForMultipleObjects
    DWORD bytes_already_in_buffer = *handle.m_read_data_size;
    void* read_buffer = &(handle.m_read_data[bytes_already_in_buffer]);
    DWORD bytes_to_read = READ_DATA_BUFFER_SIZE - bytes_already_in_buffer;
    // If you remove the following, empty OLA_WARN output, the call to
    // ReadFile will fail.
    // TODO(lukase) investigate why removing this leads to error 998 below
    OLA_WARN << "";
    if (!ReadFile(handle.m_handle.m_handle,
                  read_buffer,
                  bytes_to_read,
                  reinterpret_cast<DWORD*>(handle.m_read_call_size),
                  &(overlapped->second.m_overlapped))) {
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

    handles.push_back(overlapped->second.m_overlapped.hEvent);
    io_handles.push_back(handle.m_handle.m_handle);
    connected_descriptors.push_back(chd_iter->descriptor);
  }

  // Call select() on sockets
  // TODO(lukase)

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
      do {
        DWORD index = wait_result - WAIT_OBJECT_0;
        if (index < connected_descriptors.size()) {
          ConnectedDescriptor* descriptor = connected_descriptors[index];
          read_ready_queue.push(descriptor);
          handles.erase(handles.begin() + index);
          io_handles.erase(io_handles.begin() + index);
          connected_descriptors.erase(connected_descriptors.begin() + index);
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

  // Close descriptors
  chd_iter = m_connected_pipe_read_descriptors.begin();
  while (chd_iter != m_connected_pipe_read_descriptors.end()) {
    ConnectedPipeDescriptorSet::iterator this_iter = chd_iter;
    ++chd_iter;
    if (!this_iter->descriptor->ValidReadDescriptor()) {
      closed_queue.push(*this_iter);
      m_connected_pipe_read_descriptors.erase(this_iter);
    }
  }

  while (!closed_queue.empty()) {
    const connected_pipe_descriptor_t &connected_descriptor =
        closed_queue.front();
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

  return return_result;
}

void WindowsPoller::UpdateDescriptorData()
{
  ConnectedPipeDescriptorSet::iterator iter =
      m_connected_pipe_read_descriptors.begin();
  for(; iter != m_connected_pipe_read_descriptors.end(); ++iter) {
    DescriptorHandle descriptor = iter->descriptor->ReadDescriptor();
    if (*(descriptor.m_read_call_size) > 0) {
      *(descriptor.m_read_data_size) += *(descriptor.m_read_call_size);
      *(descriptor.m_read_call_size) = 0;
    }
  }
}

}  // namespace io
}  // namespace ola
