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
 * SelectServer.cpp
 * Implementation of the SelectServer class
 * Copyright (C) 2005 Simon Newton
 */

#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/select.h>
#endif

#include <string.h>
#include <errno.h>

#include <algorithm>
#include <queue>
#include <set>
#include <string>
#include <vector>

#include "common/io/SelectPoller.h"
#include "ola/Logging.h"
#include "ola/io/Descriptor.h"
#include "ola/io/SelectServer.h"
#include "ola/network/Socket.h"
#include "ola/stl/STLUtils.h"

namespace ola {
namespace io {

using ola::Callback0;
using ola::ExportMap;
using ola::thread::timeout_id;
using std::max;

const TimeStamp SelectServer::empty_time;


/*
 * Constructor
 * @param export_map an ExportMap to update
 * @param wake_up_time a TimeStamp which is updated with the current wake up
 * time.
 */
SelectServer::SelectServer(ExportMap *export_map,
                           Clock *clock)
    : m_export_map(export_map),
      m_terminate(false),
      m_is_running(false),
      m_poll_interval(POLL_INTERVAL_SECOND, POLL_INTERVAL_USECOND),
      m_clock(clock),
      m_free_clock(false) {
  if (!m_clock) {
    m_clock = new Clock;
    m_free_clock = true;
  }

  if (m_export_map) {
    m_export_map->GetIntegerVar(PollerInterface::K_READ_DESCRIPTOR_VAR);
    m_export_map->GetIntegerVar(PollerInterface::K_WRITE_DESCRIPTOR_VAR);
    m_export_map->GetIntegerVar(PollerInterface::K_CONNECTED_DESCRIPTORS_VAR);
  }

  m_timeout_manager.reset(new TimeoutManager(export_map, m_clock));
  m_poller.reset(new SelectPoller(export_map, m_clock));

  // TODO(simon): this should really be in an Init() method.
  if (!m_incoming_descriptor.Init())
    OLA_FATAL << "Failed to init LoopbackDescriptor, Execute() won't work!";
  m_incoming_descriptor.SetOnData(
      ola::NewCallback(this, &SelectServer::DrainAndExecute));
  AddReadDescriptor(&m_incoming_descriptor);
}

/*
 * Clean up
 */
SelectServer::~SelectServer() {
  while (!m_incoming_queue.empty()) {
    delete m_incoming_queue.front();
    m_incoming_queue.pop();
  }

  STLDeleteElements(&m_loop_closures);
  if (m_free_clock)
    delete m_clock;
}

const TimeStamp *SelectServer::WakeUpTime() const {
  if (m_poller.get()) {
    return m_poller->WakeUpTime();
  } else {
    return &empty_time;
  }
}

/**
 * A thread safe terminate
 */
void SelectServer::Terminate() {
  if (m_is_running)
    Execute(NewSingleCallback(this, &SelectServer::SetTerminate));
}


/*
 * Set the default poll delay time
 */
void SelectServer::SetDefaultInterval(const TimeInterval &poll_interval) {
  m_poll_interval = poll_interval;
}

/*
 * Run the select server until Terminate() is called.
 */
void SelectServer::Run() {
  if (m_is_running) {
    OLA_FATAL << "SelectServer::Run() called recursively";
    return;
  }

  m_is_running = true;
  m_terminate = false;
  while (!m_terminate) {
    // false indicates an error in CheckForEvents();
    if (!CheckForEvents(m_poll_interval))
      break;
  }
  m_is_running = false;
}

/*
 * Run one iteration of the select server
 */
void SelectServer::RunOnce(unsigned int delay_sec,
                           unsigned int delay_usec) {
  m_is_running = true;
  CheckForEvents(TimeInterval(delay_sec, delay_usec));
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

bool SelectServer::RemoveReadDescriptor(ReadFileDescriptor *descriptor) {
  bool removed = m_poller->RemoveReadDescriptor(descriptor);
  if (removed && m_export_map) {
    (*m_export_map->GetIntegerVar(
        PollerInterface::K_READ_DESCRIPTOR_VAR))--;
  }
  return removed;
}

bool SelectServer::RemoveReadDescriptor(ConnectedDescriptor *descriptor) {
  bool removed = m_poller->RemoveReadDescriptor(descriptor);
  if (removed && m_export_map) {
    (*m_export_map->GetIntegerVar(
        PollerInterface::K_CONNECTED_DESCRIPTORS_VAR))--;
  }
  return removed;
}

bool SelectServer::AddWriteDescriptor(WriteFileDescriptor *descriptor) {
  bool added = m_poller->AddWriteDescriptor(descriptor);
  if (added && m_export_map) {
    (*m_export_map->GetIntegerVar(PollerInterface::K_WRITE_DESCRIPTOR_VAR))++;
  }
  return added;
}

bool SelectServer::RemoveWriteDescriptor(WriteFileDescriptor *descriptor) {
  bool removed = m_poller->RemoveWriteDescriptor(descriptor);
  if (removed && m_export_map) {
    (*m_export_map->GetIntegerVar(PollerInterface::K_WRITE_DESCRIPTOR_VAR))--;
  }
  return removed;
}

/*
 * Register a repeating timeout function. Returning 0 from the closure will
 * cancel this timeout.
 * @param seconds the delay between function calls
 * @param closure the closure to call when the event triggers. Ownership is
 * given up to the select server - make sure nothing else uses this closure.
 * @returns the identifier for this timeout, this can be used to remove it
 * later.
 */
timeout_id SelectServer::RegisterRepeatingTimeout(
    unsigned int ms,
    ola::Callback0<bool> *closure) {
  return m_timeout_manager->RegisterRepeatingTimeout(
      TimeInterval(ms / 1000, ms % 1000 * 1000),
      closure);
}

/*
 * Register a repeating timeout function. Returning 0 from the closure will
 * cancel this timeout.
 * @param TimeInterval the delay before the closure will be run.
 * @param closure the closure to call when the event triggers. Ownership is
 * given up to the select server - make sure nothing else uses this closure.
 * @returns the identifier for this timeout, this can be used to remove it
 * later.
 */
timeout_id SelectServer::RegisterRepeatingTimeout(
    const TimeInterval &interval,
    ola::Callback0<bool> *closure) {
  return m_timeout_manager->RegisterRepeatingTimeout(interval, closure);
}

/*
 * Register a single use timeout function.
 * @param seconds the delay between function calls
 * @param closure the closure to call when the event triggers
 * @returns the identifier for this timeout, this can be used to remove it
 * later.
 */
timeout_id SelectServer::RegisterSingleTimeout(
    unsigned int ms,
    ola::SingleUseCallback0<void> *closure) {
  return m_timeout_manager->RegisterSingleTimeout(
      TimeInterval(ms / 1000, ms % 1000 * 1000),
      closure);
}

/*
 * Register a single use timeout function.
 * @param interval the delay between function calls
 * @param closure the closure to call when the event triggers
 * @returns the identifier for this timeout, this can be used to remove it
 * later.
 */
timeout_id SelectServer::RegisterSingleTimeout(
    const TimeInterval &interval,
    ola::SingleUseCallback0<void> *closure) {
  return m_timeout_manager->RegisterSingleTimeout(interval, closure);
}

/*
 * Remove a previously registered timeout
 * @param timeout_id the id of the timeout
 */
void SelectServer::RemoveTimeout(timeout_id id) {
  return m_timeout_manager->CancelTimeout(id);
}

/*
 * Add a closure to be run every loop iteration. The closure is run after any
 * i/o and timeouts have been handled.
 * Ownership is transferred to the select server.
 */
void SelectServer::RunInLoop(Callback0<void> *closure) {
  m_loop_closures.insert(closure);
}

/**
 * Execute this callback in the main select thread. This method can be called
 * from any thread. The callback will never execute immediately, this can be
 * used to perform delayed deletion of objects.
 */
void SelectServer::Execute(ola::BaseCallback0<void> *closure) {
  {
    ola::thread::MutexLocker locker(&m_incoming_mutex);
    m_incoming_queue.push(closure);
  }

  // kick select(), we do this even if we're in the same thread as select() is
  // called. If we don't do this there is a race condition because a callback
  // may be added just prior to select(). Without this kick, select() will
  // sleep for the poll_interval before executing the callback.
  uint8_t wake_up = 'a';
  m_incoming_descriptor.Send(&wake_up, sizeof(wake_up));
}

/*
 * One iteration of the select() loop.
 * @return false on error, true on success.
 */
bool SelectServer::CheckForEvents(const TimeInterval &poll_interval) {
  LoopClosureSet::iterator loop_iter;
  for (loop_iter = m_loop_closures.begin(); loop_iter != m_loop_closures.end();
       ++loop_iter)
    (*loop_iter)->Run();

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

  while (true) {
    ola::BaseCallback0<void> *callback = NULL;
    {
      thread::MutexLocker lock(&m_incoming_mutex);
      if (m_incoming_queue.empty())
        break;
      callback = m_incoming_queue.front();
      m_incoming_queue.pop();
    }
    if (callback)
      callback->Run();
  }
}
}  // namespace io
}  // namespace ola
