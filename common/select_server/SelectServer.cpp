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

#include <lla/Logging.h>
#include <lla/select_server/SelectServer.h>
#include <lla/select_server/Socket.h>

using namespace lla::select_server;

const string SelectServer::K_FD_VAR = "ss-fd-registered";
const string SelectServer::K_LOOP_VAR = "ss-loop-functions";
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
    var = m_export_map->GetIntegerVar(K_LOOP_VAR);
    var = m_export_map->GetIntegerVar(K_TIMER_VAR);
  }
}


/*
 * Run the select server until Termiate() is called.
 */
int SelectServer::Run() {
  while (!m_terminate) {
    int ret = CheckForEvents();
    if (ret)
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
 */
int SelectServer::AddSocket(Socket *socket,
                            SocketManager *manager,
                            bool delete_on_close) {
  if (socket->ReadDescriptor() < 0) {
    LLA_WARN << "AddSocket failed, fd: " << socket->ReadDescriptor();
    return -1;
  }

  registered_socket_t registered_socket;
  registered_socket.socket = socket;
  registered_socket.manager = manager;
  registered_socket.delete_on_close = delete_on_close;

  vector<registered_socket_t>::const_iterator iter;
  for (iter = m_read_sockets.begin(); iter != m_read_sockets.end(); ++iter) {
    if (iter->socket->ReadDescriptor() == socket->ReadDescriptor())
      return 0;
  }

  m_read_sockets.push_back(registered_socket);
  if (m_export_map) {
    lla::IntegerVariable *var = m_export_map->GetIntegerVar(K_FD_VAR);
    var->Increment();
  }
  return 0;
}


/*
 * Register a socket with the select server
 */
int SelectServer::RemoveSocket(class Socket *socket) {
  if (socket->ReadDescriptor() < 0) {
    LLA_WARN << "RemoveSocket failed, fd: " << socket->ReadDescriptor();
    return 0;
  }

  vector<registered_socket_t>::iterator iter;
  for (iter = m_read_sockets.begin(); iter != m_read_sockets.end(); ++iter) {
    if (iter->socket->ReadDescriptor() == socket->ReadDescriptor()) {
      m_read_sockets.erase(iter);
      if (m_export_map) {
        lla::IntegerVariable *var = m_export_map->GetIntegerVar(K_FD_VAR);
        var->Decrement();
      }
      return 0;
    }
  }
  return 0;
}


/*
 * Register a file descriptor.
 *
 * @param  fd    the file descriptor to register
 * @param  dir    LLA_FD_RD (read) or LLA_FD_WR (write)
 * @param  fh    the function to invoke
 * @param  data  data passed to the handler
 *
 * @return 0 on sucess, -1 on failure
 */
int SelectServer::RegisterFD(int fd,
                             SelectServer::Direction direction,
                             FDListener *listener,
                             FDManager *manager) {
  listener_t listener_struct;
  vector<listener_t>::iterator iter;

  listener_struct.fd = fd;
  listener_struct.listener = listener;
  listener_struct.manager = manager;

  vector<listener_t> &listeners = direction == SelectServer::READ ?
    m_rhandlers_vect : m_whandlers_vect;

  for (iter = listeners.begin(); iter != listeners.end(); ++iter) {
    if (iter->fd == fd)
      return 0;
  }
  listeners.push_back(listener_struct);
  return 0;
}


/*
 * Unregister a file descriptor
 *
 * @param  fh  the file descriptor to unregister
 * @return 0 on sucess, -1 on failure
 */
int SelectServer::UnregisterFD(int fd, SelectServer::Direction direction) {
  if (direction == SelectServer::READ)
    RemoveFDListener(m_rhandlers_vect, fd);
  else
    RemoveFDListener(m_whandlers_vect, fd);
  return 0;
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
 * register a listener to be called on each iteration through the select loop
 */
int SelectServer::RegisterLoopCallback(FDListener *listener) {
  if (listener) {
    m_loop_listeners.push_back(listener);
    if (m_export_map) {
      lla::IntegerVariable *var = m_export_map->GetIntegerVar(K_LOOP_VAR);
      var->Increment();
    }
  }
  return 0;
}


#define max(a,b) a>b?a:b
/*
 * One iteration of the select() loop.
 *
 * @return -1 on error, 0 on timeout or interrupt, else the number of bytes read
 */
int SelectServer::CheckForEvents() {
  vector<FDListener*>::iterator iter;
  int maxsd, ret;
  unsigned int i;
  fd_set r_fds, w_fds;
  struct timeval tv;
  struct timeval now;

  maxsd = AddFDListenersToSet(m_rhandlers_vect, r_fds);
  maxsd = max(maxsd, AddFDListenersToSet(m_whandlers_vect, w_fds));
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
      return 0;
    case -1:
      if (errno == EINTR) {
        return 0;
      }
      LLA_WARN << "select error: " << strerror(errno);
      return -1;
    default:
      CheckTimeouts();
      CheckSockets(r_fds);
      CheckFDListeners(m_rhandlers_vect, r_fds);
      CheckFDListeners(m_whandlers_vect, r_fds);
  }

  for (iter = m_loop_listeners.begin(); iter != m_loop_listeners.end(); ++iter)
    (*iter)->FDReady();
  return 0;
}


/*
 * Add all listeners to the fd_set
 *
 * @returns the max fd in the set
 */
int SelectServer::AddFDListenersToSet(vector<listener_t> &listeners,
                                      fd_set &set) const {
  int max_sd = -1;
  FD_ZERO(&set);
  for (int i=0; i < listeners.size(); i++) {
    FD_SET(listeners[i].fd, &set);
    max_sd = max(max_sd, listeners[i].fd);
  }
  return max_sd;
}


void  SelectServer::AddSocketsToSet(fd_set &set, int &max_sd) const {
  vector<registered_socket_t>::const_iterator iter;
  for (iter = m_read_sockets.begin(); iter != m_read_sockets.end(); ++iter) {
    max_sd = max(max_sd, iter->socket->ReadDescriptor());
    FD_SET(iter->socket->ReadDescriptor(), &set);
  }
}


/*
 * Check if any of listeners are have data pending, invoke the callback.
 */
void SelectServer::CheckFDListeners(vector<listener_t> &listeners,
                                    fd_set &set) const {
  for (int i=0; i < listeners.size(); i++) {
    if (FD_ISSET(listeners[i].fd, &set) && listeners[i].listener) {
      int ret = listeners[i].listener->FDReady();
      if (ret < 0 && listeners[i].manager) {
        listeners[i].manager->fd_error(ret, listeners[i].listener);
      }
    }
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
 * Remove an fd from the list of listeners
 */
void SelectServer::RemoveFDListener(vector<listener_t> &listeners, int fd) {
  vector<listener_t>::iterator iter;
  for (iter = listeners.begin(); iter != listeners.end(); ++iter) {
    if (iter->fd == fd) {
      listeners.erase(iter);
      break;
    }
  }
}


/*
 * Check for expired timeouts and call them.
 *
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
  m_rhandlers_vect.clear();
  m_whandlers_vect.clear();
  m_read_sockets.clear();
  m_loop_listeners.clear();
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
