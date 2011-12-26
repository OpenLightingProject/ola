/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * MultiCallback.h
 * A callback which can be exectuted multiple times. When a pre-defined limit
 * is reached, then the underlying callback is exectued.
 * Copyright (C) 2011 Simon Newton
 */

#ifndef INCLUDE_OLA_MULTICALLBACK_H_
#define INCLUDE_OLA_MULTICALLBACK_H_

#include <ola/Callback.h>

namespace ola {


/**
 * The MultiCallback class takes a limit & a callback. When the Run() method is
 * called limit times, the callback is executed and the MultiCallback object
 * deleted.
 *
 * MultiCallback is NOT thread safe.
 *
 * If limit is 0, the callback is exectuted immediately.
 */
class MultiCallback: public BaseCallback0<void> {
  public:
    MultiCallback(unsigned int limit,
                  BaseCallback0<void> *callback)
      : m_count(0),
        m_limit(limit),
        m_callback(callback) {
      if (!limit) {
        callback->Run();
        delete this;
      }
    }

    void Run() {
      if (++m_count == m_limit) {
        m_callback->Run();
        delete this;
      }
    }
  private:
    unsigned int m_count;
    unsigned int m_limit;
    BaseCallback0<void> *m_callback;
};

inline BaseCallback0<void>* NewMultiCallback(
    unsigned int limit,
    BaseCallback0<void> *callback) {
  return new MultiCallback(limit, callback);
}
}  // ola
#endif  // INCLUDE_OLA_MULTICALLBACK_H_
