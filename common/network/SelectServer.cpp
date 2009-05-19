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

#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

#include <algorithm>

#include <lla/Logging.h>
#include <lla/network/SelectServer.h>
#include <lla/network/Socket.h>

using namespace lla::network;

const string SelectServer::K_FD_VAR = "ss-fd-registered";
const string SelectServer::K_TIMER_VAR = "ss-timer-functions";

using lla::ExportMap;
using lla::LlaClosure;

/*
 * Constructor
 */
SelectServer::SelectServer(ExportMap *export_map):
  m_terminate(false),
  m_export_map(export_map) {

  if (m_export_map) {
    lla::IntegerVariable *var = m_export_map->GetIntegerVar(K_FD_VAR);
    var = m_export_map->GetIntegerVar(K_TIMER_VAR);
  }
}


/*
 * Run the select server until Termiate() is called.
 */
int SelectServer::Run() {
  while (!m_terminate) {
    // false indicates an error in CheckForEvents();
    if (!CheckForEvents())
      break;
  }
}


/*
 * Register a socket with the select server.
 * @param socket the socket to register
 * @param manager the manager to call when the socket is closed
 * @param delete_on_close controlls whether the select server calls Close() and
 * deletes the socket once it's closed. You should probably set this as false
 * if you're using a manager.
 * @return true on sucess, false on failure.
 */
bool SelectServer::AddSocket(Socket *socket,
                             SocketManager *manager,
                             bool delete_on_close) {
  if (socket->ReadDescriptor() < 0) {
    LLA_WARN << "AddSocket failed, fd: " << socket->ReadDescriptor();
    return false;
  }

  registered_socket_t registered_socket;
  registered_socket.socket = socket;
  registered_socket.manager = manager;
  registered_socket.delete_on_close = delete_on_close;

  vector<registered_socket_t>::const_iterator iter;
  for (iter = m_read_sockets.begin(); iter != m_read_sockets.end(); ++iter) {
    if (iter->socket->ReadDescriptor() == socket->ReadDescriptor()) {
      LLA_WARN << "While trying add to add " << socket->ReadDescriptor() <<
        ", fd already exists in the list of read fds";
      return false;
    }
  }

  m_read_sockets.push_back(registered_socket);
  if (m_export_map) {
    lla::IntegerVariable *var = m_export_map->GetIntegerVar(K_FD_VAR);
    var->Increment();
  }
  return true;
}


/*
 * Register a socket with the select server
 * @param socket the Socket to remove
 * @return true if removed successfully, false otherwise
 */
bool SelectServer::RemoveSocket(class Socket *socket) {
  if (socket->ReadDescriptor() < 0) {
    LLA_WARN << "RemoveSocket failed, fd: " << socket->ReadDescriptor();
    return false;
  }

  vector<registered_socket_t>::iterator iter;
  for (iter = m_read_sockets.begin(); iter != m_read_sockets.end(); ++iter) {
    if (iter->socket->ReadDescriptor() == socket->ReadDescriptor()) {
      m_read_sockets.erase(iter);
      if (m_export_map) {
        lla::IntegerVariable *var = m_export_map->GetIntegerVar(K_FD_VAR);
        var->Decrement();
      }
      return true;
    }
  }
  LLA_WARN << "Socket " << socket->ReadDescriptor() << " not found in list";
  return false;
}


/*
 * Register a timeout function. Returning 0 means you don't want it to repeat
 * anymore.
 *
 * @param seconds the delay between function calls
 */
bool SelectServer::RegisterTimeout(int ms,
                                   LlaClosure *closure,
                                   bool recurring) {
  if (!closure)
    return false;

  if (recurring && closure->SingleUse()) {
    LLA_WARN << "Adding a single use closure as a repeating event," <<
       "I'm turning repeat off";
    recurring = false;
  } else if (!recurring && !closure->SingleUse())
    LLA_WARN << "Adding a non-repeating, non single use closure," <<
       "this is probably going to leak memory\n";

  event_t event;
  event.closure = closure;
  event.interval.tv_sec = ms / K_MS_IN_SECOND;
  event.interval.tv_usec = K_MS_IN_SECOND * (ms % K_MS_IN_SECOND);
  event.repeat = recurring;

  gettimeofday(&event.next, NULL);
  timeradd(&event.next, &event.interval, &event.next);
  m_events.push(event);

  if (m_export_map) {
    lla::IntegerVariable *var = m_export_map->GetIntegerVar(K_TIMER_VAR);
    var->Increment();
  }
  return true;
}


/*
 * One iteration of the select() loop.
 * @return false on error, true on success.
 */
bool SelectServer::CheckForEvents() {
  int maxsd, ret;
  unsigned int i;
  fd_set r_fds, w_fds;
  struct timeval tv;
  struct timeval now;

  maxsd = 0;
  FD_ZERO(&r_fds);
  FD_ZERO(&w_fds);
  AddSocketsToSet(r_fds, maxsd);
  now = CheckTimeouts();

  if (m_events.empty()) {
    tv.tv_sec = 1;
    tv.tv_usec = 0;
  } else {
    struct timeval next = m_events.top().next;
    long long now_l = (long long) now.tv_sec * K_US_IN_SECOND + now.tv_usec;
    long long next_l = (long long) next.tv_sec * K_US_IN_SECOND + next.tv_usec;
    long rem = next_l - now_l;
    tv.tv_sec = rem / K_US_IN_SECOND;
    tv.tv_usec = rem % K_US_IN_SECOND;
  }

  switch (select(maxsd+1, &r_fds, &w_fds, NULL, &tv)) {
    case 0:
      // timeout
      return true;
    case -1:
      if (errno == EINTR)
        return true;
      LLA_WARN << "select() error, " << strerror(errno);
      return false;
    default:
      CheckTimeouts();
      CheckSockets(r_fds);
  }
  return true;
}


/*
 * Add all the read sockets to the FD_SET
 */
void SelectServer::AddSocketsToSet(fd_set &set, int &max_sd) const {
  vector<registered_socket_t>::const_iterator iter;
  for (iter = m_read_sockets.begin(); iter != m_read_sockets.end(); ++iter) {
    max_sd = max(max_sd, iter->socket->ReadDescriptor());
    FD_SET(iter->socket->ReadDescriptor(), &set);
  }
}


/*
 * Check all the registered sockets:
 *  - Call SocketReady() is there is new data
 *  - Handle the case when the socket gets closed
 */
void SelectServer::CheckSockets(fd_set &set) {
  vector<Socket*> ready_queue;

  vector<registered_socket_t>::iterator iter;
  for (iter = m_read_sockets.begin(); iter != m_read_sockets.end(); ++iter) {
    if (FD_ISSET(iter->socket->ReadDescriptor(), &set)) {
      if (iter->socket->IsClosed()) {
        if (iter->manager)
          iter->manager->SocketClosed(iter->socket);
        if (iter->delete_on_close) {
          iter->socket->Close();
          delete iter->socket;
        }
        if (m_export_map)
          m_export_map->GetIntegerVar(K_FD_VAR)->Decrement();
        iter = m_read_sockets.erase(iter);
        iter--;
      } else {
        ready_queue.push_back(iter->socket);
      }
    }
  }

  vector<Socket*>::iterator socket_iter;
  for (socket_iter = ready_queue.begin(); socket_iter != ready_queue.end();
      ++socket_iter) {
    (*socket_iter)->SocketReady();
  }
}


/*
 * Check for expired timeouts and call them.
 * @returns a struct timeval of the time up to where we checked.
 */
struct timeval SelectServer::CheckTimeouts() {
  struct timeval now;
  gettimeofday(&now, NULL);

  event_t e;
  if (m_events.empty())
    return now;

  for (e = m_events.top(); !m_events.empty() && timercmp(&e.next, &now, <);
       e = m_events.top()) {
    int return_code = 1;
    bool single_use = false;
    if (e.closure) {
      return_code = e.closure->Run();
      single_use = e.closure->SingleUse();
    }
    m_events.pop();

    if (e.repeat && !return_code) {
      e.next = now;
      timeradd(&e.next, &e.interval, &e.next);
      m_events.push(e);
    } else {
      // if we were repeating, and we're not a single use closure and we
      // returned an error we need to call delete
      if (e.repeat && !return_code && !single_use)
        delete e.closure;

      if (m_export_map) {
        lla::IntegerVariable *var = m_export_map->GetIntegerVar(K_TIMER_VAR);
        var->Increment();
      }
    }
    gettimeofday(&now, NULL);
  }
  return now;
}


/*
 * Remove all registrations.
 */
void SelectServer::UnregisterAll() {
  m_read_sockets.clear();
  while (!m_events.empty()) {
    event_t event = m_events.top();
    // We don't actually have enough info to decide what to do here because
    // something else may be using this closure. In this case we err on the
    // side of not deleting.
    // TODO(simonn): use scoped_ptr which avoids all of this.
    if (event.closure->SingleUse())
      // delete single use events because we own them.
      delete event.closure;
    m_events.pop();
  }
}
