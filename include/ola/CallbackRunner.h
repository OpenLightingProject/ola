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

/**
 * @defgroup callback_helpers Callback Helpers
 * @brief A series a Callback helpers that simplify complex behavior.
 * @note Please see CallbackRunner.h and MultiCallback.h for examples of each.
 *
 * @addtogroup callback_helpers
 * @{
 * @file CallbackRunner.h
 * @brief Automatically execute a 0-arg callback when it goes out of scope.
 * @examplepara
 *   @code
 *   void Foo(MyCallback *on_complete) {
 *     CallbackRunner runner(on_complete);
 *     // do work here, which may contain return statements, on_complete will
 *     // always be executed.
 *   }
 *   @endcode
 * @}
 */

#ifndef INCLUDE_OLA_CALLBACKRUNNER_H_
#define INCLUDE_OLA_CALLBACKRUNNER_H_

#include <ola/base/Macro.h>

namespace ola {

/**
 * @addtogroup callback_helpers
 * @{
 */

/**
 * @class CallbackRunner <ola/CallbackRunner.h>
 * @brief Automatically execute a callback when it goes out of scope.
 * @tparam CallbackClass An class which has a Run() method.
 *
 * This is useful if the function or method has multiple return points and
 * you need to ensure that the callback is always executed before the function
 * returns. It's most useful for handling RPCs.
 *
 * @examplepara
 *   @code
 *   int Foo(MyCallback *callback) {
 *     CallbackRunner runner(callback);
 *     if (...) {
 *        // do something here.
 *        return 1;
 *     } else {
 *       if (...) {
 *         // do something here.
 *         return 2;
 *       } else {
 *         // do something here.
 *         return 3;
 *       }
 *     }
 *   }
 *   @endcode
 */
template <typename CallbackClass>
class CallbackRunner {
 public:
    /**
     * Construct a new CallbackRunner.
     * @param callback the Callback to execute.
     */
    explicit CallbackRunner(CallbackClass *callback)
        : m_callback(callback) {
    }

    /**
     * Destructor, this executes the callback.
     */
    ~CallbackRunner() {
      m_callback->Run();
    }

 private:
    CallbackClass *m_callback;

    DISALLOW_COPY_AND_ASSIGN(CallbackRunner);
};

/**
 * @}
 */
}  // namespace ola
#endif  // INCLUDE_OLA_CALLBACKRUNNER_H_
