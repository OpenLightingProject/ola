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
 * SelectServerInterface.h
 * An interface to a SelectServer that enforces clean separation.
 * Copyright (C) 2010 Simon Newton
 */

#ifndef INCLUDE_OLA_IO_SELECTSERVERINTERFACE_H_
#define INCLUDE_OLA_IO_SELECTSERVERINTERFACE_H_

#include <ola/Callback.h>
#include <ola/Clock.h>
#include <ola/io/Descriptor.h>
#include <ola/thread/SchedulingExecutorInterface.h>

namespace ola {
namespace io {

/**
 * @brief The interface for the SelectServer.
 *
 * The SelectServerInterface is used to register Descriptors for events. It's
 * the core of the event manager system, and should really be called IOManager.
 *
 * SelectServerInterface implementations are required to be reentrant.
 * Descriptors may be added / removed and timeouts set / canceled from within
 * callbacks executed by the SelectServer.
 */
class SelectServerInterface: public ola::thread::SchedulingExecutorInterface {
 public :
  SelectServerInterface() {}
  virtual ~SelectServerInterface() {}

  /**
   * @brief Register a ReadFileDescriptor for read-events.
   * @param descriptor the ReadFileDescriptor to add.
   * @returns true if the descriptor was added, false if the descriptor was
   *   previously added.
   *
   * When the descriptor is ready for reading, PerformRead() will be called.
   */
  virtual bool AddReadDescriptor(class ReadFileDescriptor *descriptor) = 0;

  /**
   * @brief Register a ConnectedDescriptor for read-events.
   * @param descriptor the ConnectedDescriptor to add.
   * @param delete_on_close if true, ownership of the ConnectedDescriptor is
   *   transferred to the SelectServer.
   * @returns true if the descriptor was added, false if the descriptor was
   *   previously added.
   *
   * When the descriptor is ready for reading, PerformRead() will be called.
   * Prior to PerformRead(), IsClosed() is called. If this returns true, and
   * delete_on_close was set, the descriptor will be deleted.
   */
  virtual bool AddReadDescriptor(class ConnectedDescriptor *descriptor,
                                 bool delete_on_close = false) = 0;

  /**
   * @brief Remove a ReadFileDescriptor for read-events.
   * @param descriptor the descriptor to remove.
   *
   * @warning Descriptors must be removed from the SelectServer before they are
   * closed. Not doing so will result in hard to debug failures.
   */
  virtual void RemoveReadDescriptor(
      class ReadFileDescriptor *descriptor) = 0;

  /**
   * @brief Remove a ConnectedDescriptor for read-events.
   * @param descriptor the descriptor to remove.
   *
   * @warning Descriptors must be removed from the SelectServer before they are
   * closed. Not doing so will result in hard to debug failures.
   */
  virtual void RemoveReadDescriptor(class ConnectedDescriptor *descriptor) = 0;

  /**
   * @brief Register a WriteFileDescriptor for write-events.
   * @param descriptor the WriteFileDescriptor to add.
   *
   * When the descriptor is writeable, PerformWrite() is called.
   */
  virtual bool AddWriteDescriptor(
      class WriteFileDescriptor *descriptor) = 0;

  /**
   * @brief Remove a WriteFileDescriptor for write-events.
   * @param descriptor the descriptor to remove.
   *
   * @warning Descriptors must be removed from the SelectServer before they are
   * closed. Not doing so will result in hard to debug failures.
   */
  virtual void RemoveWriteDescriptor(
      class WriteFileDescriptor *descriptor) = 0;

  virtual ola::thread::timeout_id RegisterRepeatingTimeout(
      unsigned int ms,
      Callback0<bool> *closure) = 0;
  virtual ola::thread::timeout_id RegisterRepeatingTimeout(
      const ola::TimeInterval &interval,
      ola::Callback0<bool> *closure) = 0;

  virtual ola::thread::timeout_id RegisterSingleTimeout(
      unsigned int ms,
      SingleUseCallback0<void> *closure) = 0;
  virtual ola::thread::timeout_id RegisterSingleTimeout(
      const ola::TimeInterval &interval,
      SingleUseCallback0<void> *closure) = 0;

  virtual void RemoveTimeout(ola::thread::timeout_id id) = 0;

  /**
   * @brief The time when this SelectServer was woken up.
   * @returns The TimeStamp of when the SelectServer was woken up.
   *
   * If running within the same thread as the SelectServer, this is a efficient
   * way to get the current time.
   */
  virtual const TimeStamp *WakeUpTime() const = 0;
};
}  // namespace io
}  // namespace ola
#endif  // INCLUDE_OLA_IO_SELECTSERVERINTERFACE_H_
