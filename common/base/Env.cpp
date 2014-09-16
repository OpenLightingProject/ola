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
 * Env.cpp
 * Get / Set Environment variables.
 */

#include <stdlib.h>
#include <string>

#include "ola/base/Env.h"

using std::string;

namespace ola {

bool GetEnv(const string &var, string *value) {
  char *v = NULL;
#ifdef HAVE_SECURE_GETENV
  v = secure_getenv(var.c_str());
#else
  v = getenv(var.c_str());
#endif
  if (v) {
    value->assign(v);
    return true;
  }
  return false;
}

}  // namespace ola
