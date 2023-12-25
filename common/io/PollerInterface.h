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
 * PollerInterface.h
 * A poller provides the select loop.
 * Copyright (C) 2013 Simon Newton
 */

#ifndef COMMON_IO_POLLERINTERFACE_H_
#define COMMON_IO_POLLERINTERFACE_H_

#include <ola/Clock.h>
#include <ola/io/Descriptor.h>

#include "common/io/TimeoutManager.h"

namespace ola {
namespace io {

/**
 * @class PollerInterface
 * @brief The interface for the Poller classes.
 *
 * This forms the basis for the low level event management. The SelectServer
 * will add / remove descriptors as required and then call Poll() with a
 * timeout. The Poll() method is responsible for checking for timeouts (via the
 * TimeoutManager) and then blocking until the descriptors are ready or a
 * timeout event occurs. This blocking is done with select(), poll(),
 * epoll() or kevent(), depending on the implementation.
 *
 * Once the blocking wait returns. any ready descriptors should the appropriate
 * method called: ola::io::ReadFileDescriptor::PerformRead(),
 * ola::io::WriteFileDescriptor::PerformWrite() or the callback set in
 * ola::io::ConnectedDescriptor::SetOnClose(). Once all
 * descriptors and any new timeouts have been handled, Poll() returns.
 *
 * @warning
 * It's absolutely critical that implementations of PollerInterface be
 * reentrant. Calling any of the read / write / close actions may in turn add /
 * remove descriptors, including the descriptor the method was itself called
 * on. There are tests in SelectServerTest.cpp to exercise some of these cases
 * but implementers need to be careful.
 *
 * @warning
 * For example, if Poll() iterates over a set of Descriptors and calls
 * PerformRead() when appropriate, the RemoveReadDescriptor() method can not
 * simply call erase on the set, since doing so would invalidate the
 * iterator held in Poll(). The solution is to either use a data structure that
 * does not invalidate iterators on erase or use a double-lookup and set the
 * pointer to NULL to indicate erasure.
 *
 * @warning
 * It's also important to realize that after a RemoveReadDescriptor() or
 * RemoveWriteDescriptor() is called, neither the FD number nor the pointer to
 * the Destructor can be used again as a unique identifier. This is because
 * either may be reused immediately following the call to remove.
 */
class PollerInterface {
 public :
  /**
   * @brief Destructor
   */
  virtual ~PollerInterface() {}

  /**
   * @brief Register a ReadFileDescriptor for read events.
   * @param descriptor the ReadFileDescriptor to register. The OnData() method
   * will be called when there is data available for reading.
   * @returns true if the descriptor was registered, false otherwise.
   */
  virtual bool AddReadDescriptor(ReadFileDescriptor *descriptor) = 0;

  /**
   * @brief Register a ConnectedDescriptor for read events.
   * @param descriptor the ConnectedDescriptor to register. The OnData() method
   * will be called when there is data available for reading. Additionally,
   * OnClose() will be called if the other end closes the connection.
   * @param delete_on_close controls whether the descriptor is deleted when
   * it's closed.
   * @returns true if the descriptor was registered, false otherwise.
   */
  virtual bool AddReadDescriptor(ConnectedDescriptor *descriptor,
                                 bool delete_on_close) = 0;

  /**
   * @brief Unregister a ReadFileDescriptor for read events.
   * @param descriptor the ReadFileDescriptor to unregister.
   * @returns true if unregistered successfully, false otherwise.
   *
   * @pre descriptor->ReadFileDescriptor() is valid.
   */
  virtual bool RemoveReadDescriptor(ReadFileDescriptor *descriptor) = 0;

  /**
   * @brief Unregister a ConnectedDescriptor for read events.
   * @param descriptor the ConnectedDescriptor to unregister.
   * @returns true if unregistered successfully, false otherwise.
   *
   * @pre descriptor->ReadFileDescriptor() is valid.
   */
  virtual bool RemoveReadDescriptor(ConnectedDescriptor *descriptor) = 0;

  /**
   * @brief Register a WriteFileDescriptor to receive ready-to-write events.
   * @param descriptor the WriteFileDescriptor to register. The PerformWrite()
   * method will be called when the descriptor is ready for writing.
   * @returns true if the descriptor was registered, false otherwise.
   */
  virtual bool AddWriteDescriptor(WriteFileDescriptor *descriptor) = 0;

  /**
   * @brief Unregister a WriteFileDescriptor for write events.
   * @param descriptor the WriteFileDescriptor to unregister.
   * @returns true if unregistered successfully, false otherwise.
   *
   * @pre descriptor->WriteFileDescriptor() is valid.
   */
  virtual bool RemoveWriteDescriptor(WriteFileDescriptor *descriptor) = 0;

  virtual const TimeStamp *WakeUpTime() const = 0;

  /**
   * @brief Poll the Descriptors for events and execute any callbacks.
   * @param timeout_manager the TimeoutManager to use for timer events.
   * @param poll_interval the maximum time to block for.
   * @returns false if any errors occurred, true if events were handled.
   */
  virtual bool Poll(TimeoutManager *timeout_manager,
                    const TimeInterval &poll_interval) = 0;

  static const char K_READ_DESCRIPTOR_VAR[];
  static const char K_WRITE_DESCRIPTOR_VAR[];
  static const char K_CONNECTED_DESCRIPTORS_VAR[];

 protected:
  static const char K_LOOP_TIME[];
  static const char K_LOOP_COUNT[];
};
}  // namespace io
}  // namespace ola
#endif  // COMMON_IO_POLLERINTERFACE_H_
