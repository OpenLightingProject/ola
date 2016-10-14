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
 * SelectServer.cpp
 * Implementation of the SelectServer class
 * Copyright (C) 2005 Simon Newton
 */

#include "ola/io/SelectServer.h"

#ifdef _WIN32
#include <ola/win/CleanWinSock2.h>
#else
#include <sys/select.h>
#endif  // _WIN32

#if HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H

#include <string.h>
#include <errno.h>

#include <algorithm>
#include <set>
#include <string>
#include <vector>

#ifdef _WIN32
#include "common/io/WindowsPoller.h"
#else
#include "ola/base/Flags.h"


#include "common/io/SelectPoller.h"
#endif  // _WIN32

#include "ola/io/Descriptor.h"
#include "ola/Logging.h"
#include "ola/network/Socket.h"
#include "ola/stl/STLUtils.h"

#ifdef HAVE_EPOLL
#include "common/io/EPoller.h"
DEFINE_default_bool(use_epoll, true,
                    "Disable the use of epoll(), revert to select()");
#endif  // HAVE_EPOLL

#ifdef HAVE_KQUEUE
#include "common/io/KQueuePoller.h"
DEFINE_default_bool(use_kqueue, false,
                    "Use kqueue() rather than select()");
#endif  // HAVE_KQUEUE

namespace ola {
namespace io {

using ola::Callback0;
using ola::ExportMap;
using ola::thread::timeout_id;
using std::max;

const TimeStamp SelectServer::empty_time;

SelectServer::SelectServer(ExportMap *export_map,
                           Clock *clock)
    : m_export_map(export_map),
      m_terminate(false),
      m_is_running(false),
      m_poll_interval(POLL_INTERVAL_SECOND, POLL_INTERVAL_USECOND),
      m_clock(clock),
      m_free_clock(false) {
  Options options;
  Init(options);
}

SelectServer::SelectServer(const Options &options)
    : m_export_map(options.export_map),
      m_terminate(false),
      m_is_running(false),
      m_poll_interval(POLL_INTERVAL_SECOND, POLL_INTERVAL_USECOND),
      m_clock(options.clock),
      m_free_clock(false) {
  Init(options);
}

SelectServer::~SelectServer() {
  DrainCallbacks();

  STLDeleteElements(&m_loop_callbacks);
  if (m_free_clock) {
    delete m_clock;
  }
}

const TimeStamp *SelectServer::WakeUpTime() const {
  if (m_poller.get()) {
    return m_poller->WakeUpTime();
  } else {
    return &empty_time;
  }
}

void SelectServer::Terminate() {
  if (m_is_running) {
    Execute(NewSingleCallback(this, &SelectServer::SetTerminate));
  }
}

void SelectServer::SetDefaultInterval(const TimeInterval &poll_interval) {
  m_poll_interval = poll_interval;
}

void SelectServer::Run() {
  if (m_is_running) {
    OLA_FATAL << "SelectServer::Run() called recursively";
    return;
  }

  m_is_running = true;
  m_terminate = false;
  while (!m_terminate) {
    // false indicates an error in CheckForEvents();
    if (!CheckForEvents(m_poll_interval)) {
      break;
    }
  }
  m_is_running = false;
}

void SelectServer::RunOnce() {
  RunOnce(TimeInterval(0, 0));
}

void SelectServer::RunOnce(const TimeInterval &block_interval) {
  m_is_running = true;
  CheckForEvents(block_interval);
  m_is_running = false;
}

bool SelectServer::AddReadDescriptor(ReadFileDescriptor *descriptor) {
  bool added =  m_poller->AddReadDescriptor(descriptor);
  if (added && m_export_map) {
    (*m_export_map->GetIntegerVar(PollerInterface::K_READ_DESCRIPTOR_VAR))++;
  }
  return added;
}

bool SelectServer::AddReadDescriptor(ConnectedDescriptor *descriptor,
                                     bool delete_on_close) {
  bool added =  m_poller->AddReadDescriptor(descriptor, delete_on_close);
  if (added && m_export_map) {
    (*m_export_map->GetIntegerVar(
        PollerInterface::K_CONNECTED_DESCRIPTORS_VAR))++;
  }
  return added;
}

void SelectServer::RemoveReadDescriptor(ReadFileDescriptor *descriptor) {
  if (!descriptor->ValidReadDescriptor()) {
    OLA_WARN << "Removing an invalid file descriptor: " << descriptor;
    return;
  }

  bool removed = m_poller->RemoveReadDescriptor(descriptor);
  if (removed && m_export_map) {
    (*m_export_map->GetIntegerVar(
        PollerInterface::K_READ_DESCRIPTOR_VAR))--;
  }
}

void SelectServer::RemoveReadDescriptor(ConnectedDescriptor *descriptor) {
  if (!descriptor->ValidReadDescriptor()) {
    OLA_WARN << "Removing an invalid file descriptor: " << descriptor;
    return;
  }

  bool removed = m_poller->RemoveReadDescriptor(descriptor);
  if (removed && m_export_map) {
    (*m_export_map->GetIntegerVar(
        PollerInterface::K_CONNECTED_DESCRIPTORS_VAR))--;
  }
}

bool SelectServer::AddWriteDescriptor(WriteFileDescriptor *descriptor) {
  bool added = m_poller->AddWriteDescriptor(descriptor);
  if (added && m_export_map) {
    (*m_export_map->GetIntegerVar(PollerInterface::K_WRITE_DESCRIPTOR_VAR))++;
  }
  return added;
}

void SelectServer::RemoveWriteDescriptor(WriteFileDescriptor *descriptor) {
  if (!descriptor->ValidWriteDescriptor()) {
    OLA_WARN << "Removing a closed descriptor";
    return;
  }

  bool removed = m_poller->RemoveWriteDescriptor(descriptor);
  if (removed && m_export_map) {
    (*m_export_map->GetIntegerVar(PollerInterface::K_WRITE_DESCRIPTOR_VAR))--;
  }
}

timeout_id SelectServer::RegisterRepeatingTimeout(
    unsigned int ms,
    ola::Callback0<bool> *callback) {
  return m_timeout_manager->RegisterRepeatingTimeout(
      TimeInterval(ms / 1000, ms % 1000 * 1000),
      callback);
}

timeout_id SelectServer::RegisterRepeatingTimeout(
    const TimeInterval &interval,
    ola::Callback0<bool> *callback) {
  return m_timeout_manager->RegisterRepeatingTimeout(interval, callback);
}

timeout_id SelectServer::RegisterSingleTimeout(
    unsigned int ms,
    ola::SingleUseCallback0<void> *callback) {
  return m_timeout_manager->RegisterSingleTimeout(
      TimeInterval(ms / 1000, ms % 1000 * 1000),
      callback);
}

timeout_id SelectServer::RegisterSingleTimeout(
    const TimeInterval &interval,
    ola::SingleUseCallback0<void> *callback) {
  return m_timeout_manager->RegisterSingleTimeout(interval, callback);
}

void SelectServer::RemoveTimeout(timeout_id id) {
  return m_timeout_manager->CancelTimeout(id);
}

void SelectServer::RunInLoop(Callback0<void> *callback) {
  m_loop_callbacks.insert(callback);
}

void SelectServer::Execute(ola::BaseCallback0<void> *callback) {
  {
    ola::thread::MutexLocker locker(&m_incoming_mutex);
    m_incoming_callbacks.push_back(callback);
  }

  // kick select(), we do this even if we're in the same thread as select() is
  // called. If we don't do this there is a race condition because a callback
  // may be added just prior to select(). Without this kick, select() will
  // sleep for the poll_interval before executing the callback.
  uint8_t wake_up = 'a';
  m_incoming_descriptor.Send(&wake_up, sizeof(wake_up));
}


void SelectServer::DrainCallbacks() {
  Callbacks callbacks_to_run;
  while (true) {
    {
      ola::thread::MutexLocker locker(&m_incoming_mutex);
      if (m_incoming_callbacks.empty()) {
        return;
      }
      callbacks_to_run.swap(m_incoming_callbacks);
    }
    RunCallbacks(&callbacks_to_run);
  }
}

void SelectServer::Init(const Options &options) {
  if (!m_clock) {
    m_clock = new Clock;
    m_free_clock = true;
  }

  if (m_export_map) {
    m_export_map->GetIntegerVar(PollerInterface::K_READ_DESCRIPTOR_VAR);
    m_export_map->GetIntegerVar(PollerInterface::K_WRITE_DESCRIPTOR_VAR);
    m_export_map->GetIntegerVar(PollerInterface::K_CONNECTED_DESCRIPTORS_VAR);
  }

  m_timeout_manager.reset(new TimeoutManager(m_export_map, m_clock));
#ifdef _WIN32
  m_poller.reset(new WindowsPoller(m_export_map, m_clock));
  (void) options;
#else

#ifdef HAVE_EPOLL
  if (FLAGS_use_epoll && !options.force_select) {
    m_poller.reset(new EPoller(m_export_map, m_clock));
  }
  if (m_export_map) {
    m_export_map->GetBoolVar("using-epoll")->Set(FLAGS_use_epoll);
  }
#endif  // HAVE_EPOLL

#ifdef HAVE_KQUEUE
  bool using_kqueue = false;
  if (FLAGS_use_kqueue && !m_poller.get() && !options.force_select) {
    m_poller.reset(new KQueuePoller(m_export_map, m_clock));
    using_kqueue = true;
  }
  if (m_export_map) {
    m_export_map->GetBoolVar("using-kqueue")->Set(using_kqueue);
  }
#endif  // HAVE_KQUEUE

  // Default to the SelectPoller
  if (!m_poller.get()) {
    m_poller.reset(new SelectPoller(m_export_map, m_clock));
  }
#endif  // _WIN32

  // TODO(simon): this should really be in an Init() method that returns a
  // bool.
  if (!m_incoming_descriptor.Init()) {
    OLA_FATAL << "Failed to init LoopbackDescriptor, Execute() won't work!";
  }
  m_incoming_descriptor.SetOnData(
      ola::NewCallback(this, &SelectServer::DrainAndExecute));
  AddReadDescriptor(&m_incoming_descriptor);
}

/*
 * One iteration of the event loop.
 * @return false on error, true on success.
 */
bool SelectServer::CheckForEvents(const TimeInterval &poll_interval) {
  LoopClosureSet::iterator loop_iter;
  for (loop_iter = m_loop_callbacks.begin();
       loop_iter != m_loop_callbacks.end();
       ++loop_iter) {
    (*loop_iter)->Run();
  }

  TimeInterval default_poll_interval = poll_interval;
  // if we've been told to terminate, make this very short.
  if (m_terminate) {
    default_poll_interval = std::min(
        default_poll_interval, TimeInterval(0, 1000));
  }
  return m_poller->Poll(m_timeout_manager.get(), default_poll_interval);
}

void SelectServer::DrainAndExecute() {
  while (m_incoming_descriptor.DataRemaining()) {
    // try to get everything in one read
    uint8_t message[100];
    unsigned int size;
    m_incoming_descriptor.Receive(reinterpret_cast<uint8_t*>(&message),
                                  sizeof(message), size);
  }

  // We can't hold the mutex while we execute the callback, so instead we swap
  // out the vector under a lock, release the lock and then run all the
  // callbacks.
  Callbacks callbacks_to_run;
  {
    thread::MutexLocker lock(&m_incoming_mutex);
    callbacks_to_run.swap(m_incoming_callbacks);
  }

  RunCallbacks(&callbacks_to_run);
}

void SelectServer::RunCallbacks(Callbacks *callbacks) {
  Callbacks::iterator iter = callbacks->begin();
  for (; iter != callbacks->end(); ++iter) {
    if (*iter) {
      (*iter)->Run();
    }
  }
  callbacks->clear();
}
}  // namespace io
}  // namespace ola
