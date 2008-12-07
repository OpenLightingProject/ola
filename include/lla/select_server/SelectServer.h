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
 * SelectServer.h
 * The select server interface
 * Copyright (C) 2005-2008 Simon Newton
 */

#ifndef LLA_SELECTSERVER_H
#define LLA_SELECTSERVER_H

#include <sys/time.h>
#include <vector>
#include <queue>

#include <lla/select_server/FdListener.h>
#include <lla/select_server/FdManager.h>
#include <lla/select_server/TimeoutListener.h>

namespace lla {
namespace select_server {

using namespace std;

class SelectServer {
  public :
    enum Direction{READ, WRITE};

    SelectServer() : m_terminate(false) {};
    ~SelectServer() { UnregisterAll(); }
    int Run();
    void Terminate() { m_terminate = true; }
    void Restart() { m_terminate = false; }

    int AddSocket(class Socket *socket, class SocketManager *manager=NULL);
    int RemoveSocket(class Socket *socket);
    int RegisterFD(int fd,
                   SelectServer::Direction dir,
                   FDListener *listener,
                   FDManager *manager);
    int UnregisterFD(int fd, SelectServer::Direction dir);
    int RegisterTimeout(int ms,
                        TimeoutListener *listener,
                        bool recurring=true,
                        bool free_after_run=false);
    int RegisterLoopCallback(FDListener *l);
    void UnregisterAll();

  private :
    // This represents a FD listener
    typedef struct {
      int fd;
      FDListener *listener;
      FDManager *manager;
    } listener_t;

    typedef struct {
      class Socket *socket;
      class SocketManager *manager;
    } registered_socket_t;

    SelectServer(const SelectServer&);
    SelectServer operator=(const SelectServer&);
    int CheckForEvents();
    void CheckSockets(fd_set &set);
    void CheckFDListeners(vector<listener_t> &listeners, fd_set &set) const;
    void AddSocketsToSet(fd_set &set, int &max_sd) const;
    int AddFDListenersToSet(vector<listener_t> &listeners, fd_set &set) const;
    void RemoveFDListener(vector<listener_t> &listeners, int fd);
    struct timeval CheckTimeouts();

    static const int MS_IN_SECOND = 1000;
    static const int US_IN_SECOND = 1000000;

    // This is a timer event
    typedef struct {
      struct timeval next;
      int interval; // non 0 if this event is recurring
      bool free_after_run; // true if we delete the listener once we're done
      TimeoutListener *listener;
    } event_t;

    struct ltevent {
      bool operator()(const event_t &e1, const event_t &e2) const {
        return timercmp(&e1.next, &e2.next, >);
      }
    };

    bool m_terminate;
    vector<listener_t> m_rhandlers_vect;
    vector<listener_t> m_whandlers_vect;
    vector<registered_socket_t> m_read_sockets;
    vector<FDListener*> m_loop_listeners;

    typedef priority_queue<event_t, vector<event_t>, ltevent> event_queue_t;
    event_queue_t m_event_cbs;
};

} // select_server
} // lla
#endif
