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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * SelectServer.h
 * The select server interface
 * Copyright (C) 2005 Simon Newton
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
#include <set>
#include <vector>

class SelectServerTest;

namespace ola {
namespace io {

/**
 * @brief A single threaded I/O event management system.
 *
 * SelectServer is the core of the event driven system. It's responsible
 * for invoking Callbacks when certain events occur.
 *
 * @examplepara
 * The following snippet shows how a SelectServer can be used with a listening
 * UDP socket. The ReceiveMessage function will be called whenever there is
 * new data on the UDP socket.
 *
 * @snippet udp_server.cpp UDP Server
 *
 * The SelectServer has a number of different implementations depending on the
 * platform. On systems with epoll, the flag \--no-use-epoll will disable the
 * use of epoll(), reverting to select(). The PollerInterface defines the
 * contract between the SelectServer and the lower level, platform dependent
 * Poller classes.
 *
 * All methods except Execute() and Terminate() must be called from the thread
 * that Run() was called in.
 */
class SelectServer: public SelectServerInterface {
 public :
  struct Options {
   public:
    Options()
        : force_select(false),
          export_map(NULL),
          clock(NULL) {
    }

    /**
     * @brief Fall back to the select() implementation even if the flags are
     * set for kqueue/epoll.
     */
    bool force_select;

    /**
     * @brief The export map to use.
     */
    ola::ExportMap *export_map;

    /**
     * @brief The Clock to use.
     */
    Clock *clock;
  };

  /**
   * @brief Create a new SelectServer
   * @param export_map the ExportMap to use for stats
   * @param clock the Clock to use to keep time.
   */
  SelectServer(ola::ExportMap *export_map = NULL,
               Clock *clock = NULL);

  /**
   * @brief Create a new SelectServer
   * @param options additional options.
   */
  explicit SelectServer(const Options &options);

  /**
   * @brief Clean up.
   *
   * The SelectServer should be terminated before it's deleted.
   */
  ~SelectServer();

  /**
   * @brief Checks if the SelectServer is running.
   * @returns true if the SelectServer is in the Run() method.
   */
  bool IsRunning() const { return m_is_running; }

  const TimeStamp *WakeUpTime() const;

  /**
   * @brief Exit from the Run() loop.
   *
   * Terminate() may be called from any thread.
   */
  void Terminate();

  /**
   * @brief Set the duration to block for.
   * @param block_interval the interval to block for.
   *
   * This controls the upper bound on the duration between callbacks added with
   * RunInLoop().
   */
  void SetDefaultInterval(const TimeInterval &block_interval);

  /**
   * @brief Enter the event loop.
   *
   * Run() will return once Terminate() has been called.
   */
  void Run();

  /**
   * @brief Do a single pass through the event loop. Does not block.
   */
  void RunOnce();

  /**
   * @brief Do a single pass through the event loop.
   * @param block_interval The maximum time to block if there is no I/O or
   *   timer events.
   */
  void RunOnce(const TimeInterval &block_interval);

  bool AddReadDescriptor(ReadFileDescriptor *descriptor);
  bool AddReadDescriptor(ConnectedDescriptor *descriptor,
                         bool delete_on_close = false);
  void RemoveReadDescriptor(ReadFileDescriptor *descriptor);
  void RemoveReadDescriptor(ConnectedDescriptor *descriptor);

  bool AddWriteDescriptor(WriteFileDescriptor *descriptor);
  void RemoveWriteDescriptor(WriteFileDescriptor *descriptor);

  ola::thread::timeout_id RegisterRepeatingTimeout(
      unsigned int ms,
      ola::Callback0<bool> *callback);
  ola::thread::timeout_id RegisterRepeatingTimeout(
      const ola::TimeInterval &interval,
      ola::Callback0<bool> *callback);

  ola::thread::timeout_id RegisterSingleTimeout(
      unsigned int ms,
      ola::SingleUseCallback0<void> *callback);
  ola::thread::timeout_id RegisterSingleTimeout(
      const ola::TimeInterval &interval,
      ola::SingleUseCallback0<void> *callback);
  void RemoveTimeout(ola::thread::timeout_id id);

  /**
   * @brief Execute a callback on every event loop.
   * @param callback the Callback to execute. Ownership is transferrred to the
   *   SelectServer.
   *
   * Be very cautious about using this, it's almost certainly not what you
   * want.
   *
   * There is no way to remove a Callback added with this method.
   */
  void RunInLoop(ola::Callback0<void> *callback);

  void Execute(ola::BaseCallback0<void> *callback);

  void DrainCallbacks();

 private:
  typedef std::vector<ola::BaseCallback0<void>*> Callbacks;
  typedef std::set<ola::Callback0<void>*> LoopClosureSet;

  ExportMap *m_export_map;
  bool m_terminate, m_is_running;
  TimeInterval m_poll_interval;
  std::auto_ptr<class TimeoutManager> m_timeout_manager;
  std::auto_ptr<class PollerInterface> m_poller;

  Clock *m_clock;
  bool m_free_clock;
  LoopClosureSet m_loop_callbacks;
  Callbacks m_incoming_callbacks;
  ola::thread::Mutex m_incoming_mutex;
  LoopbackDescriptor m_incoming_descriptor;

  void Init(const Options &options);
  bool CheckForEvents(const TimeInterval &poll_interval);
  void DrainAndExecute();
  void RunCallbacks(Callbacks *callbacks);
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
