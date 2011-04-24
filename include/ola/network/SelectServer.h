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

#include <ola/Clock.h>  // NOLINT
#include <ola/Callback.h>  // NOLINT
#include <ola/ExportMap.h>  // NOLINT
#include <ola/network/SelectServerInterface.h>  // NOLINT
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
    SingleEvent(unsigned int ms, ola::BaseCallback0<void> *closure):
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
    ola::BaseCallback0<void> *m_closure;
};


/*
 * An event that occurs more than once. The closure can return false to
 * indicate that it should not be called again.
 */
class RepeatingEvent: public Event {
  public:
    RepeatingEvent(unsigned int ms, ola::BaseCallback0<bool> *closure):
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
    ola::BaseCallback0<bool> *m_closure;
};


struct ltevent {
  bool operator()(Event *e1, Event *e2) const {
    return e1->NextTime() > e2->NextTime();
  }
};


/**
 * This is the core of the event driven system. The SelectServer is responsible
 * for invoking Callbacks when events occur.
 */
class SelectServer: public SelectServerInterface {
  public :
    enum Direction {READ, WRITE};

    SelectServer(ExportMap *export_map = NULL,
                 TimeStamp *wake_up_time = NULL);
    ~SelectServer();

    bool IsRunning() const { return !m_terminate; }
    const TimeStamp *WakeUpTime() const { return m_wake_up_time; }

    void Terminate() { m_terminate = true; }
    void Restart() { m_terminate = false; }

    void SetDefaultInterval(const TimeInterval &poll_interval);
    void Run();
    void RunOnce(unsigned int delay_sec = POLL_INTERVAL_SECOND,
                 unsigned int delay_usec = POLL_INTERVAL_USECOND);

    bool AddSocket(Socket *socket);
    bool AddSocket(ConnectedSocket *socket, bool delete_on_close = false);
    bool RemoveSocket(Socket *socket);
    bool RemoveSocket(ConnectedSocket *socket);
    bool RegisterWriteSocket(BidirectionalSocket *socket);
    bool UnRegisterWriteSocket(BidirectionalSocket *socket);

    timeout_id RegisterRepeatingTimeout(unsigned int ms,
                                        ola::Callback0<bool> *closure);
    timeout_id RegisterSingleTimeout(unsigned int ms,
                                     ola::SingleUseCallback0<void> *closure);
    void RemoveTimeout(timeout_id id);

    void RunInLoop(ola::Callback0<void> *closure);

    // these are pubic so that the tests can access them
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

    typedef std::set<Socket*> SocketSet;
    typedef std::set<BidirectionalSocket*> BidirectionalSocketSet;
    typedef std::set<connected_socket_t, connected_socket_t_lt>
      ConnectedSocketSet;
    typedef std::set<ola::Callback0<void>*> LoopClosureSet;

    bool m_terminate;
    bool m_free_wake_up_time;
    TimeInterval m_poll_interval;
    unsigned int m_next_id;
    SocketSet m_sockets;
    ConnectedSocketSet m_connected_sockets;
    BidirectionalSocketSet m_write_sockets;
    std::set<timeout_id> m_removed_timeouts;
    ExportMap *m_export_map;

    typedef priority_queue<Event*, vector<Event*>, ltevent> event_queue_t;
    event_queue_t m_events;
    CounterVariable *m_loop_iterations;
    CounterVariable *m_loop_time;
    TimeStamp *m_wake_up_time;
    LoopClosureSet m_loop_closures;

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
};
}  // network
}  // ola
#endif  // INCLUDE_OLA_NETWORK_SELECTSERVER_H_
