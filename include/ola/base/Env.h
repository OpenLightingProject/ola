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
 * Env.h
 * Get / Set Environment variables.
 */

#ifndef INCLUDE_OLA_BASE_ENV_H_
#define INCLUDE_OLA_BASE_ENV_H_

#include <string>

namespace ola {

/**
 * @brief Get the value of an environment variable
 * @param var the name of the variable to get
 * @param[out] value the of the variable
 * @returns true if the variable was defined, false otherwise.
 *
 * This prefers secure_getenv so it will return false if the environment is
 * untrusted.
 */
bool GetEnv(const std::string &var, std::string *value);

}  // namespace ola
#endif  // INCLUDE_OLA_BASE_ENV_H_
