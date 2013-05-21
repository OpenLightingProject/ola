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
 * StdinHandler.h
 * Enables reading input from stdin one character at a time. Useful if you want
 * to create a simple interactive interface for programs.
 * Copyright (C) 2012 Simon Newton
 */

#ifndef INCLUDE_OLA_IO_STDINHANDLER_H_
#define INCLUDE_OLA_IO_STDINHANDLER_H_

#include <termios.h>
#include <ola/Callback.h>
#include <ola/io/Descriptor.h>
#include <ola/io/SelectServerInterface.h>
#include <memory>


namespace ola {
namespace io {

class StdinHandler {
  public :
    typedef ola::Callback1<void, char> InputCallback;

    explicit StdinHandler(SelectServerInterface *ss, InputCallback *callback);
    ~StdinHandler();

  private:
    UnmanagedFileDescriptor m_stdin_descriptor;
    termios m_old_tc;
    SelectServerInterface *m_ss;
    std::auto_ptr<InputCallback> m_callback;

    // stdin
    void HandleData();
};
}  // namespace io
}  // namespace ola
#endif  // INCLUDE_OLA_IO_STDINHANDLER_H_
