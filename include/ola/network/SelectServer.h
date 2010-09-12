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
#include <ola/network/Socket.h>  // NOLINT

namespace ola {
namespace network {

using std::string;
using std::priority_queue;
using ola::ExportMap;


/*
 * These are timer events, they are used inside the SelectServer class
 */
class Event {
  public:
    explicit Event(unsigned int ms):
      m_interval(ms * 1000) {
      TimeStamp now;
      Clock::CurrentTime(&now);
      m_next = now + m_interval;
    }
    virtual ~Event() {}
    virtual bool Trigger() = 0;

    void UpdateTime(const TimeStamp &now) {
      m_next = now + m_interval;
    }

    TimeStamp NextTime() const { return m_next; }

  private:
    TimeInterval m_interval;
    TimeStamp m_next;
};


// An event that only happens once
class SingleEvent: public Event {
  public:
    SingleEvent(unsigned int ms, ola::BaseClosure<void> *closure):
      Event(ms),
      m_closure(closure) {
    }

    virtual ~SingleEvent() {
      if (m_closure)
        delete m_closure;
    }

    bool Trigger() {
      if (m_closure) {
        m_closure->Run();
        // it's deleted itself at this point
        m_closure = NULL;
      }
      return false;
    }

  private:
    ola::BaseClosure<void> *m_closure;
};


/*
 * An event that occurs more than once. The closure can return false to
 * indicate that it should not be called again.
 */
class RepeatingEvent: public Event {
  public:
    RepeatingEvent(unsigned int ms, ola::BaseClosure<bool> *closure):
      Event(ms),
      m_closure(closure) {
    }
    ~RepeatingEvent() {
      delete m_closure;
    }
    bool Trigger() {
      if (!m_closure)
        return false;
      return m_closure->Run();
    }

  private:
    ola::BaseClosure<bool> *m_closure;
};


typedef Event* timeout_id;
static const timeout_id INVALID_TIMEOUT = 0;


struct ltevent {
  bool operator()(Event *e1, Event *e2) const {
    return e1->NextTime() > e2->NextTime();
  }
};


class SelectServer {
  public :
    enum Direction {READ, WRITE};

    SelectServer(ExportMap *export_map = NULL,
                 TimeStamp *wake_up_time = NULL);
    ~SelectServer();

    void SetDefaultInterval(const TimeInterval &poll_interval);
    void Run();
    void RunOnce(unsigned int delay_sec = POLL_INTERVAL_SECOND,
                 unsigned int delay_usec = POLL_INTERVAL_USECOND);
    void Terminate() { m_terminate = true; }
    void Restart() { m_terminate = false; }

    bool AddSocket(class Socket *socket);
    bool AddSocket(class ConnectedSocket *socket,
                   bool delete_on_close = false);
    bool RemoveSocket(class Socket *socket);
    bool RemoveSocket(class ConnectedSocket *socket);

    bool RegisterWriteSocket(class BidirectionalSocket *socket);
    bool UnRegisterWriteSocket(class BidirectionalSocket *socket);

    timeout_id RegisterRepeatingTimeout(unsigned int ms,
                                        ola::Closure<bool> *closure);
    timeout_id RegisterSingleTimeout(unsigned int ms,
                                     ola::SingleUseClosure<void> *closure);
    void RemoveTimeout(timeout_id id);
    const TimeStamp *WakeUpTime() const { return m_wake_up_time; }

    void RunInLoop(ola::Closure<void> *closure);

    static const char K_SOCKET_VAR[];
    static const char K_WRITE_SOCKET_VAR[];
    static const char K_CONNECTED_SOCKET_VAR[];
    static const char K_TIMER_VAR[];
    static const char K_LOOP_TIME[];
    static const char K_LOOP_COUNT[];

  private :
    typedef struct {
      ConnectedSocket *socket;
      bool delete_on_close;
    } connected_socket_t;

    struct connected_socket_t_lt {
      bool operator()(const connected_socket_t &c1,
                      const connected_socket_t &c2) const {
        return c1.socket->ReadDescriptor() < c2.socket->ReadDescriptor();
      }
    };

    SelectServer(const SelectServer&);
    SelectServer operator=(const SelectServer&);
    bool CheckForEvents(const TimeInterval &poll_interval);
    void CheckSockets(fd_set *r_set, fd_set *w_set);
    void AddSocketsToSet(fd_set *r_set, fd_set *w_set, int *max_sd);
    TimeStamp CheckTimeouts(const TimeStamp &now);
    void UnregisterAll();

    static const int K_MS_IN_SECOND = 1000;
    static const int K_US_IN_SECOND = 1000000;
    static const unsigned int POLL_INTERVAL_SECOND = 1;
    static const unsigned int POLL_INTERVAL_USECOND = 0;

    bool m_terminate;
    bool m_free_wake_up_time;
    TimeInterval m_poll_interval;
    unsigned int m_next_id;
    std::set<class Socket*> m_sockets;
    std::set<connected_socket_t, connected_socket_t_lt> m_connected_sockets;
    std::set<BidirectionalSocket*> m_write_sockets;
    std::set<timeout_id> m_removed_timeouts;
    ExportMap *m_export_map;

    typedef priority_queue<Event*, vector<Event*>, ltevent> event_queue_t;
    event_queue_t m_events;
    CounterVariable *m_loop_iterations;
    CounterVariable *m_loop_time;
    TimeStamp *m_wake_up_time;
    std::set<ola::Closure<void>*> m_loop_closures;
};
}  // network
}  // ola
#endif  // INCLUDE_OLA_NETWORK_SELECTSERVER_H_
