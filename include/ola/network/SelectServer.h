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

#ifndef INCLUDE_OLA_NETWORK_SELECTSERVER_H_
#define INCLUDE_OLA_NETWORK_SELECTSERVER_H_

#include <queue>
#include <set>
#include <string>
#include <vector>

#include <ola/Closure.h>  // NOLINT
#include <ola/Clock.h>  // NOLINT
#include <ola/ExportMap.h>  // NOLINT

namespace ola {
namespace network {

using std::string;
using std::priority_queue;
using ola::ExportMap;

typedef unsigned int timeout_id;
static const timeout_id INVALID_TIMEOUT = 0;

class SelectServer {
  public :
    enum Direction {READ, WRITE};

    SelectServer(ExportMap *export_map = NULL,
                 TimeStamp *wake_up_time = NULL);
    ~SelectServer();

    void Run();
    void RunOnce(unsigned int delay_sec = 1,
                 unsigned int delay_usec = 0);
    void Terminate() { m_terminate = true; }
    void Restart() { m_terminate = false; }

    bool AddSocket(class Socket *socket);
    bool AddSocket(class ConnectedSocket *socket,
                   bool delete_on_close = false);
    bool RemoveSocket(class Socket *socket);
    bool RemoveSocket(class ConnectedSocket *socket);
    timeout_id RegisterRepeatingTimeout(int ms, ola::Closure *closure);
    timeout_id RegisterSingleTimeout(int ms, ola::SingleUseClosure *closure);
    void RemoveTimeout(timeout_id id);
    const TimeStamp *WakeUpTime() const { return m_wake_up_time; }

    static const char K_SOCKET_VAR[];
    static const char K_CONNECTED_SOCKET_VAR[];
    static const char K_TIMER_VAR[];
    static const char K_LOOP_TIME[];
    static const char K_LOOP_COUNT[];

  private :
    typedef struct {
      class ConnectedSocket *socket;
      bool delete_on_close;
    } connected_socket_t;

    SelectServer(const SelectServer&);
    SelectServer operator=(const SelectServer&);
    timeout_id RegisterTimeout(int ms, ola::BaseClosure *closure,
                               bool repeating);
    bool CheckForEvents(unsigned int delay_sec,
                        unsigned int delay_usec);
    void CheckSockets(fd_set *set);
    void AddSocketsToSet(fd_set *set, int *max_sd);
    TimeStamp CheckTimeouts(const TimeStamp &now);
    void UnregisterAll();

    static const int K_MS_IN_SECOND = 1000;
    static const int K_US_IN_SECOND = 1000000;

    // This is a timer event
    typedef struct {
      timeout_id id;
      TimeStamp next;
      TimeInterval interval;
      bool repeating;
      ola::BaseClosure *closure;
    } event_t;

    struct ltevent {
      bool operator()(const event_t &e1, const event_t &e2) const {
        return e1.next > e2.next;
      }
    };

    bool m_terminate;
    bool m_free_wake_up_time;
    unsigned int m_next_id;
    vector<class Socket*> m_sockets;
    vector<connected_socket_t> m_connected_sockets;
    vector<Closure*> m_ready_queue;
    std::set<timeout_id> m_removed_timeouts;
    ExportMap *m_export_map;

    typedef priority_queue<event_t, vector<event_t>, ltevent> event_queue_t;
    event_queue_t m_events;
    CounterVariable *m_loop_iterations;
    CounterVariable *m_loop_time;
    TimeStamp *m_wake_up_time;
};
}  // network
}  // ola
#endif  // INCLUDE_OLA_NETWORK_SELECTSERVER_H_
