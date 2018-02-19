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
 * Utils.cpp
 * Helper functions for threads.
 * Copyright (C) 2014 Simon Newton
 */

#include "ola/thread/Utils.h"

#if HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H

#ifdef _WIN32
// On MinGW, pthread.h pulls in Windows.h, which in turn pollutes the global
// namespace. We define VC_EXTRALEAN and WIN32_LEAN_AND_MEAN to reduce this.
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#endif  // _WIN32
#include <pthread.h>
#include <string.h>
#include <string>
#include "ola/Logging.h"
#include "ola/thread/Thread.h"

namespace ola {
namespace thread {

std::string PolicyToString(int policy) {
  switch (policy) {
    case SCHED_FIFO:
      return "SCHED_FIFO";
    case SCHED_RR:
      return "SCHED_RR";
    case SCHED_OTHER:
      return "SCHED_OTHER";
    default:
      return "unknown";
  }
}

bool SetSchedParam(pthread_t thread, int policy,
                   const struct sched_param &param) {
  int r = pthread_setschedparam(thread, policy, &param);
  if (r != 0) {
    OLA_FATAL << "Unable to set thread scheduling parameters for thread: "
#ifdef _WIN32
            << thread.x << ": " << strerror(r);
#else
            << thread << ": " << strerror(r);
#endif  // _WIN32
    return false;
  }
  return true;
}
}  // namespace thread
}  // namespace ola
