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
#include <vector>

#include "ola/Logging.h"
#include "ola/network/SelectServer.h"
#include "ola/network/Socket.h"


namespace ola {
namespace network {

// # of sockets registered
const char SelectServer::K_SOCKET_VAR[] = "ss-sockets";
// # of connected sockets registered
const char SelectServer::K_CONNECTED_SOCKET_VAR[] = "ss-connections";
// # of timer functions registered
const char SelectServer::K_TIMER_VAR[] = "ss-timers";
// time spent processing events/timeouts in microseconds
const char SelectServer::K_LOOP_TIME[] = "ss-loop-time";
// iterations through the select server
const char SelectServer::K_LOOP_COUNT[] = "ss-loop-count";

using std::max;
using ola::ExportMap;
using ola::Closure;

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
      m_next_id(INVALID_TIMEOUT + 1),
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
 * Run the select server until Terminate() is called.
 */
void SelectServer::Run() {
  while (!m_terminate) {
    // false indicates an error in CheckForEvents();
    if (!CheckForEvents())
      break;
  }
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

  vector<Socket*>::const_iterator iter;
  for (iter = m_sockets.begin(); iter != m_sockets.end(); ++iter) {
    if ((*iter)->ReadDescriptor() == socket->ReadDescriptor()) {
      OLA_WARN << "While trying add to add " << socket->ReadDescriptor() <<
        ", fd already exists in the list of read fds";
      return false;
    }
  }

  m_sockets.push_back(socket);
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
 * @return true on sucess, false on failure.
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

  vector<connected_socket_t>::const_iterator iter;
  for (iter = m_connected_sockets.begin(); iter != m_connected_sockets.end();
       ++iter) {
    if (iter->socket->ReadDescriptor() == socket->ReadDescriptor()) {
      OLA_WARN << "While trying add to add " << socket->ReadDescriptor() <<
        ", fd already exists in the list of read fds";
      return false;
    }
  }

  m_connected_sockets.push_back(registered_socket);
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

  vector<Socket*>::iterator iter;
  for (iter = m_sockets.begin(); iter != m_sockets.end(); ++iter) {
    if ((*iter)->ReadDescriptor() == socket->ReadDescriptor()) {
      m_sockets.erase(iter);
      if (m_export_map)
        (*m_export_map->GetIntegerVar(K_SOCKET_VAR))--;
      return true;
    }
  }
  OLA_WARN << "Socket " << socket->ReadDescriptor() << " not found in list";
  return false;
}


/*
 * Unregister a ConnectedSocket with the select server
 * @param socket the Socket to remove
 * @return true if removed successfully, false otherwise
 */
bool SelectServer::RemoveSocket(ConnectedSocket *socket) {
  if (socket->ReadDescriptor() == Socket::INVALID_SOCKET)
    OLA_WARN << "Removing a closed socket: " << socket->ReadDescriptor();

  vector<connected_socket_t>::iterator iter;
  for (iter = m_connected_sockets.begin(); iter != m_connected_sockets.end();
       ++iter) {
    if (iter->socket->ReadDescriptor() == socket->ReadDescriptor()) {
      m_connected_sockets.erase(iter);
      if (m_export_map)
        (*m_export_map->GetIntegerVar(K_CONNECTED_SOCKET_VAR))--;
      return true;
    }
  }
  OLA_WARN << "Socket " << socket->ReadDescriptor() << " not found in list";
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
timeout_id SelectServer::RegisterRepeatingTimeout(int ms,
                                                  ola::Closure *closure) {
  return RegisterTimeout(ms, closure, true);
}


/*
 * Register a single use timeout function.
 * @param seconds the delay between function calls
 * @param closure the closure to call when the event triggers
 * @returns the identifier for this timeout, this can be used to remove it
 * later.
 */
timeout_id SelectServer::RegisterSingleTimeout(
    int ms,
    ola::SingleUseClosure *closure) {
  return RegisterTimeout(ms, closure, false);
}


/*
 * Remove a previously registered timeout
 * @param timeout_id the id of the timeout
 */
void SelectServer::RemoveTimeout(timeout_id id) {
  if (!m_removed_timeouts.insert(id).second)
    OLA_WARN << "timeout " << id << " already in remove set";
}


timeout_id SelectServer::RegisterTimeout(int ms,
                                         BaseClosure *closure,
                                         bool repeating) {
  if (!closure)
    return INVALID_TIMEOUT;

  event_t event;
  event.id = m_next_id++;
  event.closure = closure;
  event.interval = ms * 1000;
  event.repeating = repeating;

  Clock::CurrentTime(event.next);
  event.next += event.interval;
  m_events.push(event);

  if (m_export_map)
    (*m_export_map->GetIntegerVar(K_TIMER_VAR))++;
  return event.id;
}


/*
 * One iteration of the select() loop.
 * @return false on error, true on success.
 */
bool SelectServer::CheckForEvents() {
  int maxsd;
  fd_set r_fds, w_fds;
  TimeStamp now;
  struct timeval tv;

  maxsd = 0;
  FD_ZERO(&r_fds);
  FD_ZERO(&w_fds);
  Clock::CurrentTime(now);
  now = CheckTimeouts(now);

  // adding sockets should be the last thing we do, they may have changed due
  // to timeouts above.
  AddSocketsToSet(&r_fds, &maxsd);

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

  if (m_events.empty()) {
    tv.tv_sec = 1;
    tv.tv_usec = 0;
  } else {
    TimeInterval interval = m_events.top().next - now;
    interval.AsTimeval(&tv);
  }

  switch (select(maxsd + 1, &r_fds, &w_fds, NULL, &tv)) {
    case 0:
      // timeout
      Clock::CurrentTime(*m_wake_up_time);
      return true;
    case -1:
      if (errno == EINTR)
        return true;
      OLA_WARN << "select() error, " << strerror(errno);
      return false;
    default:
      Clock::CurrentTime(*m_wake_up_time);
      CheckTimeouts(*m_wake_up_time);
      CheckSockets(&r_fds);
  }
  return true;
}


/*
 * Add all the read sockets to the FD_SET
 */
void SelectServer::AddSocketsToSet(fd_set *set, int *max_sd) const {
  vector<Socket*>::const_iterator iter;
  for (iter = m_sockets.begin(); iter != m_sockets.end(); ++iter) {
    if ((*iter)->ReadDescriptor() == Socket::INVALID_SOCKET) {
      // The socket was probably closed without removing it from the select
      // server
      OLA_WARN << "Not adding an invalid socket";
      continue;
    }
    *max_sd = max(*max_sd, (*iter)->ReadDescriptor());
    FD_SET((*iter)->ReadDescriptor(), set);
  }

  vector<connected_socket_t>::const_iterator con_iter;
  for (con_iter = m_connected_sockets.begin();
       con_iter != m_connected_sockets.end(); ++con_iter) {
    if (con_iter->socket->ReadDescriptor() == Socket::INVALID_SOCKET) {
      // The socket was probably closed without removing it from the select
      // server
      OLA_WARN << "Not adding an invalid socket";
      continue;
    }
    *max_sd = max(*max_sd, con_iter->socket->ReadDescriptor());
    FD_SET(con_iter->socket->ReadDescriptor(), set);
  }
}


/*
 * Check all the registered sockets:
 *  - Execute the callback for sockets with data
 *  - Excute OnClose if a remote end closed the connection
 */
void SelectServer::CheckSockets(fd_set *set) {
  // Because the callbacks can add or remove sockets from the select server, we
  // have to call them after we've used the iterators.
  m_ready_queue.clear();

  vector<Socket*>::iterator iter;
  for (iter = m_sockets.begin(); iter != m_sockets.end(); ++iter) {
    if (FD_ISSET((*iter)->ReadDescriptor(), set)) {
      if ((*iter)->OnData())
        m_ready_queue.push_back((*iter)->OnData());
      else
        OLA_FATAL << "Socket " << (*iter)->ReadDescriptor() <<
          "is ready but no handler attached, this is bad!";
    }
  }

  vector<connected_socket_t>::iterator con_iter;
  for (con_iter = m_connected_sockets.begin();
       con_iter != m_connected_sockets.end(); ++con_iter) {
    if (FD_ISSET(con_iter->socket->ReadDescriptor(), set)) {
      if (con_iter->socket->CheckIfClosed()) {
        if (con_iter->delete_on_close)
          delete con_iter->socket;
        if (m_export_map)
          (*m_export_map->GetIntegerVar(K_CONNECTED_SOCKET_VAR))--;
        con_iter = m_connected_sockets.erase(con_iter);
        con_iter--;
      } else {
        if (con_iter->socket->OnData())
          m_ready_queue.push_back(con_iter->socket->OnData());
        else
          OLA_FATAL << "Socket " << con_iter->socket->ReadDescriptor() <<
            "is ready but no handler attached, this is bad!";
      }
    }
  }

  vector<Closure*>::iterator socket_iter;
  for (socket_iter = m_ready_queue.begin(); socket_iter != m_ready_queue.end();
      ++socket_iter) {
    (*socket_iter)->Run();
  }
}


/*
 * Check for expired timeouts and call them.
 * @returns a struct timeval of the time up to where we checked.
 */
TimeStamp SelectServer::CheckTimeouts(const TimeStamp &current_time) {
  TimeStamp now = current_time;

  event_t e;
  if (m_events.empty())
    return now;

  for (e = m_events.top(); !m_events.empty() && (e.next < now);
       e = m_events.top()) {
    m_events.pop();

    // if this was removed, skip it
    if (m_removed_timeouts.erase(e.id)) {
      delete e.closure;
      if (m_export_map)
        (*m_export_map->GetIntegerVar(K_TIMER_VAR))--;
      continue;
    }

    int return_code = 1;
    if (e.closure)
      return_code = e.closure->Run();

    if (e.repeating && !return_code) {
      e.next = now;
      e.next += e.interval;
      m_events.push(e);
    } else {
      // if we were repeating and we returned an error we need to call delete
      if (e.repeating && !return_code)
        delete e.closure;

      if (m_export_map)
        (*m_export_map->GetIntegerVar(K_TIMER_VAR))--;
    }
    Clock::CurrentTime(now);
  }
  return now;
}


/*
 * Remove all registrations.
 */
void SelectServer::UnregisterAll() {
  vector<connected_socket_t>::iterator iter;
  for (iter = m_connected_sockets.begin(); iter != m_connected_sockets.end();
       ++iter) {
    if (iter->delete_on_close) {
      delete iter->socket;
    }
  }
  m_sockets.clear();
  m_connected_sockets.clear();

  while (!m_events.empty()) {
    event_t event = m_events.top();
    m_removed_timeouts.erase(event.id);
    delete event.closure;
    m_events.pop();
  }
}
}  // network
}  // ola
