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
 * SchedulingExecutorInterface.h
 * An interface that implements both the Scheduling & Executor methods.
 * Copyright (C) 2011 Simon Newton
 */

#ifndef INCLUDE_OLA_THREAD_SCHEDULINGEXECUTORINTERFACE_H_
#define INCLUDE_OLA_THREAD_SCHEDULINGEXECUTORINTERFACE_H_

#include <ola/thread/ExecutorInterface.h>
#include <ola/thread/SchedulerInterface.h>

namespace ola {
namespace thread {

class SchedulingExecutorInterface: public ExecutorInterface,
                                   public SchedulerInterface {
 public:
    virtual ~SchedulingExecutorInterface() {}
};
}  // namespace thread
}  // namespace ola
#endif  // INCLUDE_OLA_THREAD_SCHEDULINGEXECUTORINTERFACE_H_
