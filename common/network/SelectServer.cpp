/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
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
#include "ola/network/SelectServer.h"
#include "ola/network/Socket.h"


namespace ola {
namespace network {

// # of sockets registered
const char SelectServer::K_SOCKET_VAR[] = "ss-sockets";
// # of sockets registered for writing
const char SelectServer::K_WRITE_SOCKET_VAR[] = "ss-write-sockets";
// # of connected sockets registered
const char SelectServer::K_CONNECTED_SOCKET_VAR[] = "ss-connections";
// # of timer functions registered
const char SelectServer::K_TIMER_VAR[] = "ss-timers";
// time spent processing events/timeouts in microseconds
const char SelectServer::K_LOOP_TIME[] = "ss-loop-time";
// iterations through the select server
const char SelectServer::K_LOOP_COUNT[] = "ss-loop-count";

using std::max;
using std::set;
using ola::ExportMap;
using ola::Callback0;

/*
 * Constructor
 * @param export_map an ExportMap to update
 * @param wake_up_time a TimeStamp which is updated with the current wake up
 * time.
 */
SelectServer::SelectServer(ExportMap *export_map,
                           TimeStamp *wake_up_time)
    : m_terminate(false),
      m_free_wake_up_time(false),
      m_poll_interval(POLL_INTERVAL_SECOND, POLL_INTERVAL_USECOND),
      m_export_map(export_map),
      m_loop_iterations(NULL),
      m_loop_time(NULL),
      m_wake_up_time(wake_up_time) {

  if (m_export_map) {
    m_export_map->GetIntegerVar(K_SOCKET_VAR);
    m_export_map->GetIntegerVar(K_TIMER_VAR);
    m_loop_time = m_export_map->GetCounterVar(K_LOOP_TIME);
    m_loop_iterations = m_export_map->GetCounterVar(K_LOOP_COUNT);
  }

  if (!m_wake_up_time) {
    m_wake_up_time = new TimeStamp();
    m_free_wake_up_time = true;
  }
}


/*
 * Clean up
 */
SelectServer::~SelectServer() {
  UnregisterAll();
  if (m_free_wake_up_time)
    delete m_wake_up_time;
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
  while (!m_terminate) {
    // false indicates an error in CheckForEvents();
    if (!CheckForEvents(m_poll_interval))
      break;
  }
}


/*
 * Run one iteration of the select server
 */
void SelectServer::RunOnce(unsigned int delay_sec,
                           unsigned int delay_usec) {
  CheckForEvents(TimeInterval(delay_sec, delay_usec));
}


/*
 * Register a Socket with the select server.
 * @param Socket the socket to register. The OnData closure of this socket will
 *   be called when there is data available for reading.
 * @return true on success, false on failure.
 */
bool SelectServer::AddSocket(Socket *socket) {
  if (socket->ReadDescriptor() == Socket::INVALID_SOCKET) {
    OLA_WARN << "AddSocket failed, fd: " << socket->ReadDescriptor();
    return false;
  }

  set<Socket*>::const_iterator iter = m_sockets.find(socket);
  if (iter != m_sockets.end())
    return false;

  m_sockets.insert(socket);
  if (m_export_map)
    (*m_export_map->GetIntegerVar(K_SOCKET_VAR))++;
  return true;
}


/*
 * Register a ConnectedSocket with the select server.
 * @param socket the socket to register. The OnData method will be called when
 * there is data available for reading. Additionally, OnClose will be called
 *   if the other end closes the connection
 * @param delete_on_close controls whether the select server deletes the socket
 *   once it's closed.
 * @return true on success, false on failure.
 */
bool SelectServer::AddSocket(ConnectedSocket *socket,
                             bool delete_on_close) {
  if (socket->ReadDescriptor() == Socket::INVALID_SOCKET) {
    OLA_WARN << "AddSocket failed, fd: " << socket->ReadDescriptor();
    return false;
  }

  connected_socket_t registered_socket;
  registered_socket.socket = socket;
  registered_socket.delete_on_close = delete_on_close;

  set<connected_socket_t>::const_iterator iter;
  for (iter = m_connected_sockets.begin(); iter != m_connected_sockets.end();
       ++iter) {
    if (iter->socket->ReadDescriptor() == socket->ReadDescriptor())
      return false;
  }

  m_connected_sockets.insert(registered_socket);
  if (m_export_map)
    (*m_export_map->GetIntegerVar(K_CONNECTED_SOCKET_VAR))++;
  return true;
}


/*
 * Unregister a Socket with the select server
 * @param socket the Socket to remove
 * @return true if removed successfully, false otherwise
 */
bool SelectServer::RemoveSocket(Socket *socket) {
  if (socket->ReadDescriptor() == Socket::INVALID_SOCKET)
    OLA_WARN << "Removing a closed socket: " << socket->ReadDescriptor();

  set<Socket*>::iterator iter = m_sockets.find(socket);
  if (iter != m_sockets.end()) {
    m_sockets.erase(iter);
    if (m_export_map)
      (*m_export_map->GetIntegerVar(K_SOCKET_VAR))--;
    return true;
  }
  return false;
}


/*
 * Unregister a ConnectedSocket with the select server
 * @param socket the Socket to remove
 * @return true if removed successfully, false otherwise
 */
bool SelectServer::RemoveSocket(ConnectedSocket *socket) {
  if (socket->ReadDescriptor() == Socket::INVALID_SOCKET)
    OLA_WARN << "Removing a closed socket: " << socket;

  set<connected_socket_t>::iterator iter;
  for (iter = m_connected_sockets.begin(); iter != m_connected_sockets.end();
       ++iter) {
    if (iter->socket->ReadDescriptor() == socket->ReadDescriptor()) {
      m_connected_sockets.erase(iter);
      if (m_export_map)
        (*m_export_map->GetIntegerVar(K_CONNECTED_SOCKET_VAR))--;
      return true;
    }
  }
  return false;
}


/*
 * Register a socket to receive ready-to-write event notifications
 * @param socket the socket to register. The PerformWrite method will be called
 * when the socket is ready for writing.
 * @return true on success, false on failure.
 */
bool SelectServer::RegisterWriteSocket(class BidirectionalSocket *socket) {
  if (socket->WriteDescriptor() == Socket::INVALID_SOCKET) {
    OLA_WARN << "AddSocket failed, fd: " << socket->WriteDescriptor();
    return false;
  }

  set<BidirectionalSocket*>::const_iterator iter =
    m_write_sockets.find(socket);
  if (iter != m_write_sockets.end())
    return false;

  m_write_sockets.insert(socket);
  if (m_export_map)
    (*m_export_map->GetIntegerVar(K_WRITE_SOCKET_VAR))++;
  return true;
}


/*
 * UnRegister a socket from receiving ready-to-write event notifications
 * @param socket the socket to register.
 * @return true on success, false on failure.
 */
bool SelectServer::UnRegisterWriteSocket(class BidirectionalSocket *socket) {
  if (socket->WriteDescriptor() == Socket::INVALID_SOCKET)
    OLA_WARN << "Removing a closed socket: " << socket->WriteDescriptor();

  set<BidirectionalSocket*>::iterator iter = m_write_sockets.find(socket);
  if (iter != m_write_sockets.end()) {
    m_write_sockets.erase(iter);
    if (m_export_map)
      (*m_export_map->GetIntegerVar(K_WRITE_SOCKET_VAR))--;
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
  if (!closure)
    return INVALID_TIMEOUT;

  if (m_export_map)
    (*m_export_map->GetIntegerVar(K_TIMER_VAR))++;

  Event *event = new RepeatingEvent(ms, closure);
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
  if (!closure)
    return INVALID_TIMEOUT;

  if (m_export_map)
    (*m_export_map->GetIntegerVar(K_TIMER_VAR))++;

  Event *event = new SingleEvent(ms, closure);
  m_events.push(event);
  return event;
}


/*
 * Remove a previously registered timeout
 * @param timeout_id the id of the timeout
 */
void SelectServer::RemoveTimeout(timeout_id id) {
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


/*
 * One iteration of the select() loop.
 * @return false on error, true on success.
 */
bool SelectServer::CheckForEvents(const TimeInterval &poll_interval) {
  int maxsd;
  fd_set r_fds, w_fds;
  TimeStamp now;
  struct timeval tv;

  set<Callback0<void>*>::iterator loop_iter;
  for (loop_iter = m_loop_closures.begin(); loop_iter != m_loop_closures.end();
       ++loop_iter)
    (*loop_iter)->Run();

  maxsd = 0;
  FD_ZERO(&r_fds);
  FD_ZERO(&w_fds);
  Clock::CurrentTime(&now);
  now = CheckTimeouts(now);

  // adding sockets should be the last thing we do, they may have changed due
  // to timeouts above.
  AddSocketsToSet(&r_fds, &w_fds, &maxsd);

  if (m_wake_up_time->IsSet()) {
    TimeInterval loop_time = now - *m_wake_up_time;
    OLA_DEBUG << "ss process time was " << loop_time.ToString();
    if (m_loop_time)
      (*m_loop_time) += loop_time.AsInt();
    if (m_loop_iterations)
      (*m_loop_iterations)++;
  }

  if (m_terminate)
    return true;

  poll_interval.AsTimeval(&tv);
  if (!m_events.empty()) {
    TimeInterval interval = m_events.top()->NextTime() - now;
    if (interval < poll_interval)
      interval.AsTimeval(&tv);
  }

  switch (select(maxsd + 1, &r_fds, &w_fds, NULL, &tv)) {
    case 0:
      // timeout
      Clock::CurrentTime(m_wake_up_time);
      return true;
    case -1:
      if (errno == EINTR)
        return true;
      OLA_WARN << "select() error, " << strerror(errno);
      return false;
    default:
      Clock::CurrentTime(m_wake_up_time);
      CheckTimeouts(*m_wake_up_time);
      CheckSockets(&r_fds, &w_fds);
      Clock::CurrentTime(m_wake_up_time);
      CheckTimeouts(*m_wake_up_time);
  }
  return true;
}


/*
 * Add all the read sockets to the FD_SET
 */
void SelectServer::AddSocketsToSet(fd_set *r_set,
                                   fd_set *w_set,
                                   int *max_sd) {
  set<Socket*>::iterator iter = m_sockets.begin();
  while (iter != m_sockets.end()) {
    if ((*iter)->ReadDescriptor() == Socket::INVALID_SOCKET) {
      // The socket was probably closed without removing it from the select
      // server
      if (m_export_map)
        (*m_export_map->GetIntegerVar(K_SOCKET_VAR))--;
      m_sockets.erase(iter++);
      OLA_WARN << "Removed a disconnected socket from the select server";
    } else {
      *max_sd = max(*max_sd, (*iter)->ReadDescriptor());
      FD_SET((*iter)->ReadDescriptor(), r_set);
      iter++;
    }
  }

  set<connected_socket_t>::iterator con_iter = m_connected_sockets.begin();
  while (con_iter != m_connected_sockets.end()) {
    if (con_iter->socket->ReadDescriptor() == Socket::INVALID_SOCKET) {
      // The socket was closed without removing it from the select server
      if (con_iter->delete_on_close)
        delete con_iter->socket;
      if (m_export_map)
        (*m_export_map->GetIntegerVar(K_CONNECTED_SOCKET_VAR))--;
      m_connected_sockets.erase(con_iter++);
      OLA_WARN << "Removed a disconnected socket from the select server";
    } else {
      *max_sd = max(*max_sd, con_iter->socket->ReadDescriptor());
      FD_SET(con_iter->socket->ReadDescriptor(), r_set);
      con_iter++;
    }
  }

  set<BidirectionalSocket*>::iterator write_iter = m_write_sockets.begin();
  while (write_iter != m_write_sockets.end()) {
    if ((*write_iter)->WriteDescriptor() == Socket::INVALID_SOCKET) {
      // The socket was probably closed without removing it from the select
      // server
      if (m_export_map)
        (*m_export_map->GetIntegerVar(K_WRITE_SOCKET_VAR))--;
      m_write_sockets.erase(write_iter++);
      OLA_WARN << "Removed a disconnected socket from the select server";
    } else {
      *max_sd = max(*max_sd, (*write_iter)->WriteDescriptor());
      FD_SET((*write_iter)->WriteDescriptor(), w_set);
      write_iter++;
    }
  }
}


/*
 * Check all the registered sockets:
 *  - Execute the callback for sockets with data
 *  - Excute OnClose if a remote end closed the connection
 */
void SelectServer::CheckSockets(fd_set *r_set, fd_set *w_set) {
  // Because the callbacks can add or remove sockets from the select server, we
  // have to call them after we've used the iterators.
  std::queue<Callback0<void>*> read_ready_queue;
  std::queue<Callback0<void>*> write_ready_queue;

  set<Socket*>::iterator iter;
  for (iter = m_sockets.begin(); iter != m_sockets.end(); ++iter) {
    if (FD_ISSET((*iter)->ReadDescriptor(), r_set)) {
      if ((*iter)->OnData())
        read_ready_queue.push((*iter)->OnData());
      else
        OLA_FATAL << "Socket " << (*iter)->ReadDescriptor() <<
          "is ready but no handler attached, this is bad!";
    }
  }

  set<connected_socket_t>::iterator con_iter = m_connected_sockets.begin();
  while (con_iter != m_connected_sockets.end()) {
    if (FD_ISSET(con_iter->socket->ReadDescriptor(), r_set)) {
      if (con_iter->socket->CheckIfClosed()) {
        if (con_iter->delete_on_close)
          delete con_iter->socket;
        if (m_export_map)
          (*m_export_map->GetIntegerVar(K_CONNECTED_SOCKET_VAR))--;
        m_connected_sockets.erase(con_iter++);
        continue;
      } else {
        if (con_iter->socket->OnData())
          read_ready_queue.push(con_iter->socket->OnData());
        else
          OLA_FATAL << "Socket " << con_iter->socket->ReadDescriptor() <<
            "is ready but no handler attached, this is bad!";
      }
    }
    con_iter++;
  }

  set<BidirectionalSocket*>::iterator write_iter = m_write_sockets.begin();
  for (;write_iter != m_write_sockets.end(); write_iter++) {
    if (FD_ISSET((*write_iter)->WriteDescriptor(), w_set)) {
      if ((*write_iter)->PerformWrite())
        write_ready_queue.push((*write_iter)->PerformWrite());
      else
        OLA_FATAL << "Socket " << (*write_iter)->WriteDescriptor() <<
          "is ready but no write handler attached, this is bad!";
    }
  }

  while (!read_ready_queue.empty()) {
    Callback0<void> *closure = read_ready_queue.front();
    closure->Run();
    read_ready_queue.pop();
  }

  while (!write_ready_queue.empty()) {
    Callback0<void> *closure = write_ready_queue.front();
    closure->Run();
    write_ready_queue.pop();
  }
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
    Clock::CurrentTime(&now);
  }
  return now;
}


/*
 * Remove all registrations.
 */
void SelectServer::UnregisterAll() {
  set<connected_socket_t>::iterator iter = m_connected_sockets.begin();
  for (; iter != m_connected_sockets.end(); ++iter) {
    if (iter->delete_on_close) {
      delete iter->socket;
    }
  }
  m_sockets.clear();
  m_connected_sockets.clear();
  m_write_sockets.clear();
  m_removed_timeouts.clear();

  while (!m_events.empty()) {
    delete m_events.top();
    m_events.pop();
  }

  set<Callback0<void>*>::iterator loop_iter;
  for (loop_iter = m_loop_closures.begin(); loop_iter != m_loop_closures.end();
       ++loop_iter)
    delete *loop_iter;
  m_loop_closures.clear();
}
}  // network
}  // ola
