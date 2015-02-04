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
 * Deleter.h
 * Asynchronous deletion of pointers.
 */

#ifndef INCLUDE_OLA_UTIL_DELETER_H_
#define INCLUDE_OLA_UTIL_DELETER_H_

#include <ola/Callback.h>

namespace ola {

/**
 * @brief Delete a pointer.
 * @tparam T any type.
 * @param t The pointer to delete. Ownership is transferred.
 */
template <typename T>
void Deleter(T* t) {
  delete t;
}

/**
 * @brief Create a callback that deletes the object.
 * @tparam T any type.
 * @param t The pointer to delete. Ownership is transferred.
 * @returns A SingleUseCallback which will delete the pointer when run.
 * @sa ExecutorThread
 */
template <typename T>
SingleUseCallback0<void>* DeletePointerCallback(T* t) {
  return NewSingleCallback<void>(Deleter, t);
}

}  // namespace ola
#endif  // INCLUDE_OLA_UTIL_DELETER_H_
