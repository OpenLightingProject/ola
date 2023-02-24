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
 * Utils.h
 * Helper functions for threads.
 * Copyright (C) 2014 Simon Newton
 */

#ifndef INCLUDE_OLA_THREAD_UTILS_H_
#define INCLUDE_OLA_THREAD_UTILS_H_

#ifdef _WIN32
// On MinGW, pthread.h pulls in Windows.h, which in turn pollutes the global
// namespace. We define VC_EXTRALEAN and WIN32_LEAN_AND_MEAN to reduce this.
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#endif  // _WIN32
#include <pthread.h>
#include <string>

namespace ola {
namespace thread {

/**
 * @brief Convert a scheduling policy to a string.
 * @param policy the scheduling policy.
 * @returns The name of the policy or "unknown".
 */
std::string PolicyToString(int policy);

/**
 * @brief Wraps pthread_setschedparam().
 * @param thread The thread id.
 * @param policy the new policy.
 * @param param the scheduling parameters.
 * @returns True if the call succeeded, false otherwise.
 */
bool SetSchedParam(pthread_t thread, int policy,
                   const struct sched_param &param);

}  // namespace thread
}  // namespace ola
#endif  // INCLUDE_OLA_THREAD_UTILS_H_
