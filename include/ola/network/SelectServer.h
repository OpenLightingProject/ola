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

#include <ola/Callback.h>  // NOLINT
#include <ola/Clock.h>  // NOLINT
#include <ola/ExportMap.h>  // NOLINT
#include <ola/io/Descriptor.h>  // NOLINT
#include <ola/network/SelectServerInterface.h>  // NOLINT
#include <ola/network/Socket.h>  // NOLINT
#include <ola/thread/Thread.h>  // NOLINT

namespace ola {
namespace network {

using ola::ExportMap;
using ola::io::ConnectedDescriptor;
using ola::io::ReadFileDescriptor;
using ola::io::WriteFileDescriptor;
using ola::thread::timeout_id;
using std::priority_queue;
using std::string;


/**
 * This is the core of the event driven system. The SelectServer is responsible
 * for invoking Callbacks when events occur. All methods except Execute() and
 * Terminate() must be called from the thread that Run() was called in.
 */
class SelectServer: public SelectServerInterface {
  public :
    enum Direction {READ, WRITE};

    SelectServer(ExportMap *export_map = NULL,
                 Clock *clock = NULL);
    ~SelectServer();

    bool IsRunning() const { return !m_terminate; }
    const TimeStamp *WakeUpTime() const { return &m_wake_up_time; }

    void Terminate();

    void SetDefaultInterval(const TimeInterval &poll_interval);
    void Run();
    void RunOnce(unsigned int delay_sec = POLL_INTERVAL_SECOND,
                 unsigned int delay_usec = POLL_INTERVAL_USECOND);

    bool AddReadDescriptor(ReadFileDescriptor *descriptor);
    bool AddReadDescriptor(ConnectedDescriptor *descriptor,
                           bool delete_on_close = false);
    bool RemoveReadDescriptor(ReadFileDescriptor *descriptor);
    bool RemoveReadDescriptor(ConnectedDescriptor *descriptor);

    bool AddWriteDescriptor(WriteFileDescriptor *descriptor);
    bool RemoveWriteDescriptor(WriteFileDescriptor *descriptor);

    timeout_id RegisterRepeatingTimeout(unsigned int ms,
                                        ola::Callback0<bool> *closure);
    timeout_id RegisterRepeatingTimeout(const ola::TimeInterval &interval,
                                        ola::Callback0<bool> *closure);

    timeout_id RegisterSingleTimeout(unsigned int ms,
                                     ola::SingleUseCallback0<void> *closure);
    timeout_id RegisterSingleTimeout(const ola::TimeInterval &interval,
                                     ola::SingleUseCallback0<void> *closure);
    void RemoveTimeout(timeout_id id);

    void RunInLoop(ola::Callback0<void> *closure);

    void Execute(ola::BaseCallback0<void> *closure);

    // these are pubic so that the tests can access them
    static const char K_READ_DESCRIPTOR_VAR[];
    static const char K_WRITE_DESCRIPTOR_VAR[];
    static const char K_CONNECTED_DESCRIPTORS_VAR[];
    static const char K_TIMER_VAR[];
    static const char K_LOOP_TIME[];
    static const char K_LOOP_COUNT[];

  private :
    /*
     * These are timer events, they are used inside the SelectServer class
     */
    class Event {
      public:
        explicit Event(const TimeInterval &interval, const Clock *clock)
            : m_interval(interval) {
          TimeStamp now;
          clock->CurrentTime(&now);
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
        SingleEvent(const TimeInterval &interval,
                    const Clock *clock,
                    ola::BaseCallback0<void> *closure):
          Event(interval, clock),
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
        RepeatingEvent(const TimeInterval &interval,
                       const Clock *clock,
                       ola::BaseCallback0<bool> *closure):
          Event(interval, clock),
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


    typedef struct {
      ola::io::ConnectedDescriptor *descriptor;
      bool delete_on_close;
    } connected_descriptor_t;

    struct connected_descriptor_t_lt {
      bool operator()(const connected_descriptor_t &c1,
                      const connected_descriptor_t &c2) const {
        return c1.descriptor->ReadDescriptor() <
            c2.descriptor->ReadDescriptor();
      }
    };

    struct ltevent {
      bool operator()(Event *e1, Event *e2) const {
        return e1->NextTime() > e2->NextTime();
      }
    };

    typedef std::set<ReadFileDescriptor*> ReadDescriptorSet;
    typedef std::set<WriteFileDescriptor*> WriteDescriptorSet;
    typedef std::set<connected_descriptor_t, connected_descriptor_t_lt>
      ConnectedDescriptorSet;
    typedef std::set<ola::Callback0<void>*> LoopClosureSet;

    bool m_terminate, m_is_running;
    TimeInterval m_poll_interval;
    unsigned int m_next_id;
    ReadDescriptorSet m_read_descriptors;
    ConnectedDescriptorSet m_connected_read_descriptors;
    WriteDescriptorSet m_write_descriptors;
    std::set<timeout_id> m_removed_timeouts;
    ExportMap *m_export_map;

    typedef priority_queue<Event*, vector<Event*>, ltevent> event_queue_t;
    event_queue_t m_events;
    CounterVariable *m_loop_iterations;
    CounterVariable *m_loop_time;
    TimeStamp m_wake_up_time;
    Clock *m_clock;
    bool m_free_clock;
    LoopClosureSet m_loop_closures;
    std::queue<ola::BaseCallback0<void>*> m_incoming_queue;
    ola::thread::Mutex m_incoming_mutex;
    ola::io::LoopbackDescriptor m_incoming_descriptor;

    SelectServer(const SelectServer&);
    SelectServer operator=(const SelectServer&);
    bool CheckForEvents(const TimeInterval &poll_interval);
    void CheckDescriptors(fd_set *r_set, fd_set *w_set);
    void AddDescriptorsToSet(fd_set *r_set, fd_set *w_set, int *max_sd);
    TimeStamp CheckTimeouts(const TimeStamp &now);
    void UnregisterAll();
    void DrainAndExecute();
    void SetTerminate() { m_terminate = true; }

    static const int K_MS_IN_SECOND = 1000;
    static const int K_US_IN_SECOND = 1000000;
    // the maximum time we'll wait in the select call
    static const unsigned int POLL_INTERVAL_SECOND = 10;
    static const unsigned int POLL_INTERVAL_USECOND = 0;
};
}  // network
}  // ola
#endif  // INCLUDE_OLA_NETWORK_SELECTSERVER_H_
