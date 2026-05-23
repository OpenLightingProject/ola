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
 * StdinHandler.h
 * Enables reading input from stdin one character at a time. Useful if you want
 * to create a simple interactive interface for programs.
 * Copyright (C) 2012 Simon Newton
 */

#ifndef INCLUDE_OLA_IO_STDINHANDLER_H_
#define INCLUDE_OLA_IO_STDINHANDLER_H_

#ifndef _WIN32
#include <termios.h>
#endif  // _WIN32
#include <ola/Callback.h>
#include <ola/io/Descriptor.h>
#include <ola/io/SelectServerInterface.h>
#include <memory>

/**
 * @defgroup stdin Stdin Handling
 * @brief Standard Input Handling.
 *
 * Code to read from stdin
 *
 * @examplepara
 *  @snippet stdin_handler.cpp Example
 *  When <tt>./stdin_handler</tt> is run, it will echo each key pressed except
 *  for q which will quit.
 */

/**
 * @addtogroup stdin
 * @{
 * @file StdinHandler.h
 * @brief The stdin handler
 */

namespace ola {
namespace io {

/**
 * @addtogroup stdin
 * @{
 */

class StdinHandler {
 public :
  typedef ola::Callback1<void, int> InputCallback;

  /**
   * @brief Handle data from stdin
   * @param ss The SelectServer to use
   * @param callback the callback to run when we get new data on stdin
   */
  explicit StdinHandler(SelectServerInterface *ss, InputCallback *callback);
  ~StdinHandler();

 private:
  UnmanagedFileDescriptor m_stdin_descriptor;
#ifndef _WIN32
  termios m_old_tc;
#endif  // _WIN32
  SelectServerInterface *m_ss;
  std::unique_ptr<InputCallback> m_callback;

  // stdin
  void HandleData();
};
}  // namespace io
}  // namespace ola
/** @}*/
#endif  // INCLUDE_OLA_IO_STDINHANDLER_H_
