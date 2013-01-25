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
 * SelectServerInterface.h
 * An interface to a SelectServer that enforces clean separation.
 * Copyright (C) 2010 Simon Newton
 */

#ifndef INCLUDE_OLA_IO_SELECTSERVERINTERFACE_H_
#define INCLUDE_OLA_IO_SELECTSERVERINTERFACE_H_

#include <ola/Clock.h>  // NOLINT
#include <ola/Callback.h>  // NOLINT
#include <ola/io/Descriptor.h>  // NOLINT
#include <ola/thread/SchedulingExecutorInterface.h>  // NOLINT

namespace ola {
namespace io {

class SelectServerInterface: public ola::thread::SchedulingExecutorInterface {
  public :
    SelectServerInterface() {}
    virtual ~SelectServerInterface() {}

    virtual bool AddReadDescriptor(class ReadFileDescriptor *descriptor) = 0;
    virtual bool AddReadDescriptor(class ConnectedDescriptor *socket,
                                   bool delete_on_close = false) = 0;
    virtual bool RemoveReadDescriptor(
        class ReadFileDescriptor *descriptor) = 0;
    virtual bool RemoveReadDescriptor(class ConnectedDescriptor *socket) = 0;

    virtual bool AddWriteDescriptor(
        class WriteFileDescriptor *descriptor) = 0;
    virtual bool RemoveWriteDescriptor(
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

    virtual const TimeStamp *WakeUpTime() const = 0;
};
}  // io
}  // ola
#endif  // INCLUDE_OLA_IO_SELECTSERVERINTERFACE_H_
