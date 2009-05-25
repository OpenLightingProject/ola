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

#include <lla/Closure.h>
#include <lla/ExportMap.h>

namespace lla {
namespace network {

using namespace std;
using lla::ExportMap;

class SelectServer {
  public :
    enum Direction{READ, WRITE};

    SelectServer(ExportMap *export_map=NULL);
    ~SelectServer() { UnregisterAll(); }
    int Run();
    void Terminate() { m_terminate = true; }
    void Restart() { m_terminate = false; }

    bool AddSocket(class Socket *socket,
                   lla::Closure *on_data,
                   lla::SingleUseClosure *on_close=NULL,
                   bool delete_on_close=false);
    bool RemoveSocket(class Socket *socket);
    bool RegisterRepeatingTimeout(int ms, lla::Closure *closure);
    bool RegisterSingleTimeout(int ms, lla::SingleUseClosure *closure);

    static const string K_FD_VAR;
    static const string K_TIMER_VAR;

  private :
    typedef struct {
      class Socket *socket;
      lla::Closure *on_data;
      lla::SingleUseClosure *on_close;
      bool delete_on_close;
    } registered_socket_t;

    SelectServer(const SelectServer&);
    SelectServer operator=(const SelectServer&);
    bool RegisterTimeout(int ms, lla::BaseClosure *closure, bool repeating);
    bool CheckForEvents();
    void CheckSockets(fd_set &set);
    void AddSocketsToSet(fd_set &set, int &max_sd) const;
    struct timeval CheckTimeouts();
    void UnregisterAll();

    static const int K_MS_IN_SECOND = 1000;
    static const int K_US_IN_SECOND = 1000000;

    // This is a timer event
    typedef struct {
      struct timeval next;
      struct timeval interval;
      bool repeating;
      lla::BaseClosure *closure;
    } event_t;

    struct ltevent {
      bool operator()(const event_t &e1, const event_t &e2) const {
        return timercmp(&e1.next, &e2.next, >);
      }
    };

    bool m_terminate;
    vector<registered_socket_t> m_read_sockets;
    ExportMap *m_export_map;

    typedef priority_queue<event_t, vector<event_t>, ltevent> event_queue_t;
    event_queue_t m_events;
};

} // network
} // lla
#endif
