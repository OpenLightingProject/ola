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
 * MultiCallback.h
 * A callback which can be executed multiple times. When a pre-defined limit
 * is reached, then the underlying callback is executed.
 * Copyright (C) 2011 Simon Newton
 */

/**
 * @addtogroup callback_helpers
 * @{
 * @file MultiCallback.h
 * @brief A callback which can be executed multiple times. When a pre-defined
 * limit is reached, then the underlying callback is executed.
 *
 * @examplepara
 *   @code
 *   // Calls DoSomething() for each Port and runs the on_complete callback once
 *   // each port's callback has run.
 *   void DoSomethingForAllPorts(const vector<OutputPort> &ports,
 *                               SomethingCalback *on_complete) {
 *     // This will call on_complete once it itself has been Run ports.size()
 *     // times.
 *     BaseCallback0<void> *multi_callback = NewMultiCallback( ports.size(),
 *         NewSingleCallback(this, &SomethingComplete,
 *         on_complete));
 *
 *     vector<OutputPort*>::iterator iter;
 *     for (iter = output_ports.begin(); iter != output_ports.end(); ++iter) {
 *       (*iter)->DoSomething(multi_callback);
 *     }
 *   }
 *   @endcode
 * @}
 */

#ifndef INCLUDE_OLA_MULTICALLBACK_H_
#define INCLUDE_OLA_MULTICALLBACK_H_

#include <ola/Callback.h>

namespace ola {

/**
 * @addtogroup callback_helpers
 * @{
 */

/**
 * @brief The MultiCallback class takes a limit & a callback. When the Run()
 * method is called limit times, the callback is executed and the MultiCallback
 * object deleted.
 *
 * @note MultiCallback is <b>NOT</b> thread safe.
 *
 * @note If limit is 0, the callback is executed immediately.
 */
class MultiCallback: public BaseCallback0<void> {
 public:
    /**
     * @brief Constructor
     * @param limit after limit the object is deleted
     * @param callback is a BaseCallback0<void> to be executed after Run is
     * called limit times
     */
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

    /**
     * @brief Executes the callback passed in during creation after limit
     * calls. Then MultiCallback deletes itself.
     */
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

/**
 * @brief A helper function to create a new MultiCallback
 * @param limit the number of times to run before calling callback
 * @param callback the callback to call after limit times
 * @return a pointer to a MultiCallback object
 */
inline BaseCallback0<void>* NewMultiCallback(
    unsigned int limit,
    BaseCallback0<void> *callback) {
  return new MultiCallback(limit, callback);
}
/**@}*/
}  // namespace ola
#endif  // INCLUDE_OLA_MULTICALLBACK_H_
