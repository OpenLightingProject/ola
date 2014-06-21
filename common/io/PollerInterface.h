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
 * @brief The interface for the Poller classes
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
   */
  virtual bool RemoveReadDescriptor(ReadFileDescriptor *descriptor) = 0;

  /**
   * @brief Unregister a ConnectedDescriptor for read events.
   * @param descriptor the ConnectedDescriptor to unregister.
   * @returns true if unregistered successfully, false otherwise.
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
   */
  virtual bool RemoveWriteDescriptor(WriteFileDescriptor *descriptor) = 0;

  virtual const TimeStamp *WakeUpTime() const = 0;

  /**
   * @brief Poll the Descriptors for events.
   * @param timeout_manager the TimeoutManager to use for timer events.
   * @param poll_interval the maximum time to block for.
   * @returns false if any errors occured, true if events were handled.
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
