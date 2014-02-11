/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * SelectServer.h
 * The select server interface
 * Copyright (C) 2005-2008 Simon Newton
 */

#ifndef INCLUDE_OLA_IO_SELECTSERVER_H_
#define INCLUDE_OLA_IO_SELECTSERVER_H_

#include <ola/Callback.h>
#include <ola/Clock.h>
#include <ola/ExportMap.h>
#include <ola/io/Descriptor.h>
#include <ola/io/SelectServerInterface.h>
#include <ola/network/Socket.h>
#include <ola/thread/Thread.h>

#include <memory>
#include <queue>
#include <set>

class SelectServerTest;

namespace ola {
namespace io {

/**
 * This is the core of the event driven system. The SelectServer is responsible
 * for invoking Callbacks when events occur. All methods except Execute() and
 * Terminate() must be called from the thread that Run() was called in.
 */
class SelectServer: public SelectServerInterface {
 public :
  enum Direction {READ, WRITE};

  SelectServer(ola::ExportMap *export_map = NULL,
               Clock *clock = NULL);
  ~SelectServer();

  bool IsRunning() const { return !m_terminate; }
  const TimeStamp *WakeUpTime() const;

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

  ola::thread::timeout_id RegisterRepeatingTimeout(
      unsigned int ms,
      ola::Callback0<bool> *closure);
  ola::thread::timeout_id RegisterRepeatingTimeout(
      const ola::TimeInterval &interval,
      ola::Callback0<bool> *closure);

  ola::thread::timeout_id RegisterSingleTimeout(
      unsigned int ms,
      ola::SingleUseCallback0<void> *closure);
  ola::thread::timeout_id RegisterSingleTimeout(
      const ola::TimeInterval &interval,
      ola::SingleUseCallback0<void> *closure);
  void RemoveTimeout(ola::thread::timeout_id id);

  void RunInLoop(ola::Callback0<void> *closure);

  void Execute(ola::BaseCallback0<void> *closure);

 private :
  typedef std::set<ola::Callback0<void>*> LoopClosureSet;

  ExportMap *m_export_map;
  bool m_terminate, m_is_running;
  TimeInterval m_poll_interval;
  std::auto_ptr<class TimeoutManager> m_timeout_manager;
  std::auto_ptr<class PollerInterface> m_poller;

  Clock *m_clock;
  bool m_free_clock;
  LoopClosureSet m_loop_closures;
  std::queue<ola::BaseCallback0<void>*> m_incoming_queue;
  ola::thread::Mutex m_incoming_mutex;
  LoopbackDescriptor m_incoming_descriptor;

  bool CheckForEvents(const TimeInterval &poll_interval);
  void DrainAndExecute();
  void SetTerminate() { m_terminate = true; }

  // the maximum time we'll wait in the select call
  static const unsigned int POLL_INTERVAL_SECOND = 10;
  static const unsigned int POLL_INTERVAL_USECOND = 0;

  static const TimeStamp empty_time;

  friend class ::SelectServerTest;

  DISALLOW_COPY_AND_ASSIGN(SelectServer);
};
}  // namespace io
}  // namespace ola
#endif  // INCLUDE_OLA_IO_SELECTSERVER_H_
