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
 * CallbackRunner.h
 * Automatically execute a 0-arg callback when this object goes out of scope.
 * Copyright (C) 2011 Simon Newton
 */

#ifndef INCLUDE_OLA_CALLBACKRUNNER_H_
#define INCLUDE_OLA_CALLBACKRUNNER_H_

namespace ola {

template <typename CallbackClass>
class CallbackRunner {
  public:
    explicit CallbackRunner(CallbackClass *callback)
        : m_callback(callback) {
    }
    ~CallbackRunner() {
      m_callback->Run();
    }
  private:
    CallbackClass *m_callback;
};
}  // namespace ola
#endif  // INCLUDE_OLA_CALLBACKRUNNER_H_
