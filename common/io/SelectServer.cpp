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
 * Copyright (C) 2005-2008 Simon Newton
 */

#ifdef WIN32
#include <winsock2.h>
#else
#include <sys/select.h>
#endif

#include <string.h>
#include <errno.h>

#include <algorithm>
#include <queue>
#include <set>
#include <vector>

#include "ola/Logging.h"
#include "ola/io/Descriptor.h"
#include "ola/io/SelectServer.h"
#include "ola/network/Socket.h"


namespace ola {
namespace io {

// # of descriptors registered
const char SelectServer::K_READ_DESCRIPTOR_VAR[] = "ss-read-descriptors";
// # of descriptors registered for writing
const char SelectServer::K_WRITE_DESCRIPTOR_VAR[] = "ss-write-descriptor";
// # of connected descriptors registered
const char SelectServer::K_CONNECTED_DESCRIPTORS_VAR[] =
    "ss-connected-descriptors";
// # of timer functions registered
const char SelectServer::K_TIMER_VAR[] = "ss-timers";
// time spent processing events/timeouts in microseconds
const char SelectServer::K_LOOP_TIME[] = "ss-loop-time";
// iterations through the select server
const char SelectServer::K_LOOP_COUNT[] = "ss-loop-count";

using ola::Callback0;
using ola::ExportMap;
using ola::thread::INVALID_TIMEOUT;
using ola::thread::timeout_id;
using std::max;





/*
 * Constructor
 * @param export_map an ExportMap to update
 * @param wake_up_time a TimeStamp which is updated with the current wake up
 * time.
 */
SelectServer::SelectServer(ExportMap *export_map,
                           Clock *clock)
    : m_terminate(false),
      m_is_running(false),
      m_poll_interval(POLL_INTERVAL_SECOND, POLL_INTERVAL_USECOND),
      m_export_map(export_map),
      m_loop_iterations(NULL),
      m_loop_time(NULL),
      m_clock(clock),
      m_free_clock(false) {

  if (m_export_map) {
    m_export_map->GetIntegerVar(K_READ_DESCRIPTOR_VAR);
    m_export_map->GetIntegerVar(K_TIMER_VAR);
    m_loop_time = m_export_map->GetCounterVar(K_LOOP_TIME);
    m_loop_iterations = m_export_map->GetCounterVar(K_LOOP_COUNT);
  }

  if (!m_clock) {
    m_clock = new Clock;
    m_free_clock = true;
  }

  // TODO(simon): this should really be in an Init() method.
  if (!m_incoming_descriptor.Init())
    OLA_FATAL << "Failed to init LoopbackDescriptor, Execute() won't work!";
  m_incoming_descriptor.SetOnData(
      ola::NewCallback(this, &SelectServer::DrainAndExecute));
}


/*
 * Clean up
 */
SelectServer::~SelectServer() {
  UnregisterAll();
  if (m_free_clock)
    delete m_clock;
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


/*
 * Register a ReadFileDescriptor with the select server.
 * @param descriptor the ReadFileDescriptor to register. The OnData method of
 * this descriptor will be called when there is data available for reading.
 * @return true on success, false on failure.
 */
bool SelectServer::AddReadDescriptor(ReadFileDescriptor *descriptor) {
  if (!descriptor->ValidReadDescriptor()) {
    OLA_WARN << "AddReadDescriptor called with invalid descriptor";
    return false;
  }

  if (m_read_descriptors.find(descriptor) != m_read_descriptors.end())
    return false;

  m_read_descriptors.insert(descriptor);
  if (m_export_map)
    (*m_export_map->GetIntegerVar(K_READ_DESCRIPTOR_VAR))++;
  return true;
}


/*
 * Register a ConnectedDescriptor with the select server for read events.
 * @param descriptor the ConnectedDescriptor to register. The OnData method
 * will be called when there is data available for reading. Additionally,
 * OnClose will be called if the other end closes the connection.
 * @param delete_on_close controls whether the select server deletes the
 *   descriptor once it's closed.
 * @return true on success, false on failure.
 */
bool SelectServer::AddReadDescriptor(ConnectedDescriptor *descriptor,
                                     bool delete_on_close) {
  if (!descriptor->ValidReadDescriptor()) {
    OLA_WARN << "AddReadDescriptor called with invalid descriptor";
    return false;
  }

  ConnectedDescriptorSet::const_iterator iter =
      m_connected_read_descriptors.begin();
  for (; iter != m_connected_read_descriptors.end(); ++iter) {
    if (iter->descriptor == descriptor)
      return false;
  }

  connected_descriptor_t registered_descriptor;
  registered_descriptor.descriptor = descriptor;
  registered_descriptor.delete_on_close = delete_on_close;

  m_connected_read_descriptors.insert(registered_descriptor);
  if (m_export_map)
    (*m_export_map->GetIntegerVar(K_CONNECTED_DESCRIPTORS_VAR))++;
  return true;
}


/*
 * Unregister a ReadFileDescriptor with the select server
 * @param descriptor the ReadFileDescriptor to remove
 * @return true if removed successfully, false otherwise
 */
bool SelectServer::RemoveReadDescriptor(ReadFileDescriptor *descriptor) {
  if (!descriptor->ValidReadDescriptor())
    OLA_WARN << "Removing a invalid file descriptor";

  ReadDescriptorSet::iterator iter = m_read_descriptors.find(descriptor);
  if (iter != m_read_descriptors.end()) {
    m_read_descriptors.erase(iter);
    if (m_export_map)
      (*m_export_map->GetIntegerVar(K_READ_DESCRIPTOR_VAR))--;
    return true;
  }
  return false;
}


/*
 * Unregister a ConnectedDescriptor with the select server
 * @param descriptor the ConnectedDescriptor to remove
 * @return true if removed successfully, false otherwise
 */
bool SelectServer::RemoveReadDescriptor(ConnectedDescriptor *descriptor) {
  if (!descriptor->ValidReadDescriptor())
    OLA_WARN << "Removing a invalid file descriptor";

  ConnectedDescriptorSet::iterator iter =
      m_connected_read_descriptors.begin();
  for (; iter != m_connected_read_descriptors.end(); ++iter) {
    if (iter->descriptor == descriptor) {
      m_connected_read_descriptors.erase(iter);
      if (m_export_map)
        (*m_export_map->GetIntegerVar(K_CONNECTED_DESCRIPTORS_VAR))--;
      return true;
    }
  }
  return false;
}


/*
 * Register a WriteFileDescriptor to receive ready-to-write event notifications
 * @param descriptor the WriteFileDescriptor to register. The PerformWrite
 * method will be called when the descriptor is ready for writing.
 * @return true on success, false on failure.
 */
bool SelectServer::AddWriteDescriptor(WriteFileDescriptor *descriptor) {
  if (!descriptor->ValidWriteDescriptor()) {
    OLA_WARN << "AddWriteDescriptor called with invalid descriptor";
    return false;
  }

  if (m_write_descriptors.find(descriptor) != m_write_descriptors.end())
    return false;

  m_write_descriptors.insert(descriptor);
  if (m_export_map)
    (*m_export_map->GetIntegerVar(K_WRITE_DESCRIPTOR_VAR))++;
  return true;
}


/*
 * UnRegister a WriteFileDescriptor from receiving ready-to-write event
 * notifications
 * @param descriptor the WriteFileDescriptor to register.
 * @return true on success, false on failure.
 */
bool SelectServer::RemoveWriteDescriptor(WriteFileDescriptor *descriptor) {
  if (!descriptor->ValidWriteDescriptor())
    OLA_WARN << "Removing a closed descriptor";

  WriteDescriptorSet::iterator iter = m_write_descriptors.find(descriptor);
  if (iter != m_write_descriptors.end()) {
    m_write_descriptors.erase(iter);
    if (m_export_map)
      (*m_export_map->GetIntegerVar(K_WRITE_DESCRIPTOR_VAR))--;
    return true;
  }
  return false;
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
  int64_t useconds = static_cast<int64_t>(ms) * 1000;
  return RegisterRepeatingTimeout(TimeInterval(useconds), closure);
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
  if (!closure)
    return INVALID_TIMEOUT;

  if (m_export_map)
    (*m_export_map->GetIntegerVar(K_TIMER_VAR))++;

  Event *event = new RepeatingEvent(interval, m_clock, closure);
  m_events.push(event);
  return event;
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
  int64_t useconds = static_cast<int64_t>(ms) * 1000;
  return RegisterSingleTimeout(TimeInterval(useconds), closure);
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
  if (!closure)
    return INVALID_TIMEOUT;

  if (m_export_map)
    (*m_export_map->GetIntegerVar(K_TIMER_VAR))++;

  Event *event = new SingleEvent(interval, m_clock, closure);
  m_events.push(event);
  return event;
}


/*
 * Remove a previously registered timeout
 * @param timeout_id the id of the timeout
 */
void SelectServer::RemoveTimeout(timeout_id id) {
  if (id == INVALID_TIMEOUT)
    return;

  if (!m_removed_timeouts.insert(id).second)
    OLA_WARN << "timeout " << id << " already in remove set";
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
  int maxsd;
  fd_set r_fds, w_fds;
  TimeStamp now;
  TimeInterval sleep_interval = poll_interval;
  struct timeval tv;

  LoopClosureSet::iterator loop_iter;
  for (loop_iter = m_loop_closures.begin(); loop_iter != m_loop_closures.end();
       ++loop_iter)
    (*loop_iter)->Run();

  maxsd = 0;
  FD_ZERO(&r_fds);
  FD_ZERO(&w_fds);
  m_clock->CurrentTime(&now);
  now = CheckTimeouts(now);

  // adding descriptors should be the last thing we do, they may have changed
  // due to timeouts above.
  bool closed_descriptors = AddDescriptorsToSet(&r_fds, &w_fds, &maxsd);

  // take care of stats accounting
  if (m_wake_up_time.IsSet()) {
    TimeInterval loop_time = now - m_wake_up_time;
    OLA_DEBUG << "ss process time was " << loop_time.ToString();
    if (m_loop_time)
      (*m_loop_time) += loop_time.AsInt();
    if (m_loop_iterations)
      (*m_loop_iterations)++;
  }

  if (!m_events.empty()) {
    TimeInterval interval = m_events.top()->NextTime() - now;
    sleep_interval = std::min(interval, sleep_interval);
  }

  // if we've already been told to terminate OR there are closed descriptors,
  // set the timeout to something
  // very small (1ms). This ensures we at least make a pass through the
  // descriptors.
  if (m_terminate || closed_descriptors)
    sleep_interval = std::min(sleep_interval, TimeInterval(0, 1000));

  sleep_interval.AsTimeval(&tv);
  switch (select(maxsd + 1, &r_fds, &w_fds, NULL, &tv)) {
    case 0:
      // timeout
      m_clock->CurrentTime(&m_wake_up_time);
      CheckTimeouts(m_wake_up_time);

      if (closed_descriptors) {
        // there were closed descriptors before the select() we need to deal
        // with them.
        FD_ZERO(&r_fds);
        FD_ZERO(&w_fds);
        CheckDescriptors(&r_fds, &w_fds);
      }
      return true;
    case -1:
      if (errno == EINTR)
        return true;
      OLA_WARN << "select() error, " << strerror(errno);
      return false;
    default:
      m_clock->CurrentTime(&m_wake_up_time);
      CheckDescriptors(&r_fds, &w_fds);
      m_clock->CurrentTime(&m_wake_up_time);
      CheckTimeouts(m_wake_up_time);
  }

  return true;
}


/*
 * Add all the descriptors to the FD_SET
 * @returns true if there are closed descriptors.
 */
bool SelectServer::AddDescriptorsToSet(fd_set *r_set,
                                       fd_set *w_set,
                                       int *max_sd) {
  bool closed_descriptors = false;

  ReadDescriptorSet::iterator iter = m_read_descriptors.begin();
  while (iter != m_read_descriptors.end()) {
    ReadDescriptorSet::iterator this_iter = iter;
    iter++;

    if ((*this_iter)->ValidReadDescriptor()) {
      *max_sd = max(*max_sd, (*this_iter)->ReadDescriptor());
      FD_SET((*this_iter)->ReadDescriptor(), r_set);
    } else {
      // The descriptor was probably closed without removing it from the select
      // server
      if (m_export_map)
        (*m_export_map->GetIntegerVar(K_READ_DESCRIPTOR_VAR))--;
      m_read_descriptors.erase(this_iter);
      OLA_WARN << "Removed a inactive descriptor from the select server";
    }
  }

  ConnectedDescriptorSet::iterator con_iter =
      m_connected_read_descriptors.begin();
  while (con_iter != m_connected_read_descriptors.end()) {
    ConnectedDescriptorSet::iterator this_iter = con_iter;
    con_iter++;

    if (this_iter->descriptor->ValidReadDescriptor()) {
      *max_sd = max(*max_sd, this_iter->descriptor->ReadDescriptor());
      FD_SET(this_iter->descriptor->ReadDescriptor(), r_set);
    } else {
      closed_descriptors = true;
    }
  }

  WriteDescriptorSet::iterator write_iter = m_write_descriptors.begin();
  while (write_iter != m_write_descriptors.end()) {
    WriteDescriptorSet::iterator this_iter = write_iter;
    write_iter++;

    if ((*this_iter)->ValidWriteDescriptor()) {
      *max_sd = max(*max_sd, (*this_iter)->WriteDescriptor());
      FD_SET((*this_iter)->WriteDescriptor(), w_set);
    } else {
      // The descriptor was probably closed without removing it from the select
      // server
      if (m_export_map)
        (*m_export_map->GetIntegerVar(K_WRITE_DESCRIPTOR_VAR))--;
      m_write_descriptors.erase(this_iter);
      OLA_WARN << "Removed a disconnected descriptor from the select server";
    }
  }

  // finally add the loopback descriptor
  FD_SET(m_incoming_descriptor.ReadDescriptor(), r_set);
  *max_sd = max(*max_sd, m_incoming_descriptor.ReadDescriptor());
  return closed_descriptors;
}


/*
 * Check all the registered descriptors:
 *  - Execute the callback for descriptors with data
 *  - Excute OnClose if a remote end closed the connection
 */
void SelectServer::CheckDescriptors(fd_set *r_set, fd_set *w_set) {
  // Because the callbacks can add or remove descriptors from the select
  // server, we have to call them after we've used the iterators.
  std::queue<ReadFileDescriptor*> read_ready_queue;
  std::queue<WriteFileDescriptor*> write_ready_queue;
  std::queue<connected_descriptor_t> closed_queue;

  ReadDescriptorSet::iterator iter = m_read_descriptors.begin();
  for (; iter != m_read_descriptors.end(); ++iter) {
    if (FD_ISSET((*iter)->ReadDescriptor(), r_set))
      read_ready_queue.push(*iter);
  }

  // check the read sockets
  ConnectedDescriptorSet::iterator con_iter =
      m_connected_read_descriptors.begin();
  while (con_iter != m_connected_read_descriptors.end()) {
    ConnectedDescriptorSet::iterator this_iter = con_iter;
    con_iter++;
    bool closed = false;
    if (!this_iter->descriptor->ValidReadDescriptor()) {
      closed = true;
    } else if (FD_ISSET(this_iter->descriptor->ReadDescriptor(), r_set)) {
      if (this_iter->descriptor->IsClosed())
        closed = true;
      else
        read_ready_queue.push(this_iter->descriptor);
    }

    if (closed) {
      closed_queue.push(*this_iter);
      m_connected_read_descriptors.erase(this_iter);
    }
  }

  // check the write sockets
  WriteDescriptorSet::iterator write_iter = m_write_descriptors.begin();
  for (; write_iter != m_write_descriptors.end(); write_iter++) {
    if (FD_ISSET((*write_iter)->WriteDescriptor(), w_set))
      write_ready_queue.push(*write_iter);
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

  while (!closed_queue.empty()) {
    const connected_descriptor_t &connected_descriptor = closed_queue.front();
    ConnectedDescriptor::OnCloseCallback *on_close =
      connected_descriptor.descriptor->TransferOnClose();
    if (on_close)
      on_close->Run();
    if (connected_descriptor.delete_on_close)
      delete connected_descriptor.descriptor;
    if (m_export_map)
      (*m_export_map->GetIntegerVar(K_CONNECTED_DESCRIPTORS_VAR))--;
    closed_queue.pop();
  }

  if (FD_ISSET(m_incoming_descriptor.ReadDescriptor(), r_set))
    DrainAndExecute();
}


/*
 * Check for expired timeouts and call them.
 * @returns a struct timeval of the time up to where we checked.
 */
TimeStamp SelectServer::CheckTimeouts(const TimeStamp &current_time) {
  TimeStamp now = current_time;

  Event *e;
  if (m_events.empty())
    return now;

  for (e = m_events.top(); !m_events.empty() && (e->NextTime() < now);
       e = m_events.top()) {
    m_events.pop();

    // if this was removed, skip it
    if (m_removed_timeouts.erase(e)) {
      delete e;
      if (m_export_map)
        (*m_export_map->GetIntegerVar(K_TIMER_VAR))--;
      continue;
    }

    if (e->Trigger()) {
      // true implies we need to run this again
      e->UpdateTime(now);
      m_events.push(e);
    } else {
      delete e;
      if (m_export_map)
        (*m_export_map->GetIntegerVar(K_TIMER_VAR))--;
    }
    m_clock->CurrentTime(&now);
  }
  return now;
}


/*
 * Remove all registrations.
 */
void SelectServer::UnregisterAll() {
  ConnectedDescriptorSet::iterator iter = m_connected_read_descriptors.begin();
  for (; iter != m_connected_read_descriptors.end(); ++iter) {
    if (iter->delete_on_close) {
      delete iter->descriptor;
    }
  }
  m_read_descriptors.clear();
  m_connected_read_descriptors.clear();
  m_write_descriptors.clear();
  m_removed_timeouts.clear();

  while (!m_events.empty()) {
    delete m_events.top();
    m_events.pop();
  }

  LoopClosureSet::iterator loop_iter;
  for (loop_iter = m_loop_closures.begin(); loop_iter != m_loop_closures.end();
       ++loop_iter)
    delete *loop_iter;
  m_loop_closures.clear();
}


void SelectServer::DrainAndExecute() {
  while (m_incoming_descriptor.DataRemaining()) {
    uint8_t message;
    unsigned int size;
    m_incoming_descriptor.Receive(&message, sizeof(message), size);
  }

  while (true) {
    m_incoming_mutex.Lock();
    if (m_incoming_queue.empty()) {
      m_incoming_mutex.Unlock();
      break;
    }

    ola::BaseCallback0<void> *callback = m_incoming_queue.front();
    m_incoming_queue.pop();
    m_incoming_mutex.Unlock();
    callback->Run();
  }
}
}  // io
}  // ola
