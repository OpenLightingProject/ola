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

#include <lla/select_server/SelectServer.h>
#include <lla/select_server/Socket.h>

using namespace lla::select_server;

const string SelectServer::K_FD_VAR = "ss-fd-registered";
const string SelectServer::K_LOOP_VAR = "ss-loop-functions";
const string SelectServer::K_TIMER_VAR = "ss-timer-functions";

using lla::ExportMap;

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
 * Register a socket with the select server
 */
int SelectServer::AddSocket(Socket *socket,
                            SocketManager *manager,
                            bool delete_on_close) {
  if (socket->ReadDescriptor() < 0) {
    printf("AddSocket failed, fd: %d\n", socket->ReadDescriptor());
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
    printf("RemoveSocket failed, fd: %d\n", socket->ReadDescriptor());
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
 * Register a timeout function
 *
 * @param seconds the delay between function calls
 */
int SelectServer::RegisterTimeout(int ms,
                                  TimeoutListener *listener,
                                  bool recurring,
                                  bool free_after_run) {
  if (!listener)
    return -1;

  event_t event;
  event.listener = listener;
  event.interval.tv_sec = ms / K_MS_IN_SECOND;
  event.interval.tv_usec = K_MS_IN_SECOND * (ms % K_MS_IN_SECOND);
  event.repeat = recurring;
  event.free_after_run = free_after_run;

  gettimeofday(&event.next, NULL);
  timeradd(&event.next, &event.interval, &event.next);
  m_event_cbs.push(event);

  if (m_export_map) {
    lla::IntegerVariable *var = m_export_map->GetIntegerVar(K_TIMER_VAR);
    var->Increment();
  }
  return 0;
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

  if (m_event_cbs.empty()) {
    tv.tv_sec = 1;
    tv.tv_usec = 0;
  } else {
    struct timeval next = m_event_cbs.top().next;
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
      printf("select error: %s", strerror(errno));
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


void SelectServer::CheckSockets(fd_set &set) {
  for (int i=0; i < m_read_sockets.size(); i++) {
    Socket *socket = m_read_sockets[i].socket;
    if (FD_ISSET(socket->ReadDescriptor(), &set)) {
      if (socket->IsClosed()) {
        RemoveSocket(socket);
        if (m_read_sockets[i].manager)
          m_read_sockets[i].manager->SocketClosed(socket);
        socket->Close();
        if (m_read_sockets[i].delete_on_close)
          delete socket;
      } else {
        int ret = socket->SocketReady();
      }
    }
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
      printf("Unregistered fd %d\n", fd);
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
  if (m_event_cbs.empty())
    return now;

  for (e = m_event_cbs.top(); !m_event_cbs.empty() && timercmp(&e.next, &now, <); e = m_event_cbs.top()) {
    if (e.listener != NULL)
      e.listener->Timeout();
    m_event_cbs.pop();

    if (e.repeat) {
      e.next = now;
      timeradd(&e.next, &e.interval, &e.next);
      m_event_cbs.push(e);
    }

    if (!e.repeat && e.free_after_run) {
      delete e.listener;

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
 * Remove all registrations
 */
void SelectServer::UnregisterAll() {
  m_rhandlers_vect.clear();
  m_whandlers_vect.clear();
  m_read_sockets.clear();
  m_loop_listeners.clear();
  while (!m_event_cbs.empty()) {
    event_t event = m_event_cbs.top();
    if (event.free_after_run)
      delete event.listener;
    m_event_cbs.pop();
  }
}
