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
 * Result.h
 * Copyright (C) 2013 Simon Newton
 */

/**
 * @file
 * @brief The Result object passed to OLA client callbacks.
 */

#ifndef INCLUDE_OLA_CLIENT_RESULT_H_
#define INCLUDE_OLA_CLIENT_RESULT_H_

#include <ola/base/Macro.h>
#include <string>

namespace ola {
namespace client {

/**
 * @class Result ola/client/Result.h
 * @brief Indicates the result of a OLA API call.
 *
 * Result objects are the first argument passed to an API callback function.
 * They indicate if the action succeeded.
 *
 * @examplepara
 * @code
     void ActionCallback(const Result &result, ...) {
       if (!result.Success()) {
         LOG(WARN) << result.Error();
         return;
       }
       // handle data
     };
   @endcode
 */
class Result {
 public:
  /**
   * @param error the text description of the error. An empty string means
   * the action succeeded.
   */
  explicit Result(const std::string &error)
      : m_error(error) {
  }

  /**
   * @brief Indicates the status of the action.
   * If the action failed Error() can be used to fetch the error message.
   * @return true if the action succeeded, false otherwise.
   */
  bool Success() const { return m_error.empty(); }

  /**
   * @brief Returns the error message if the action failed.
   * @return the error message.
   */
  const std::string& Error() const { return m_error; }

 private:
  const std::string m_error;

  DISALLOW_COPY_AND_ASSIGN(Result);
};
}  // namespace client
}  // namespace ola
#endif  // INCLUDE_OLA_CLIENT_RESULT_H_
