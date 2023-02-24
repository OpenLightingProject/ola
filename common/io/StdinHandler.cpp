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
 * StdinHandler.cpp
 * Enables reading input from stdin one character at a time. Useful if you want
 * to create a simple interactive interface for programs.
 * Copyright (C) 2012 Simon Newton
 */

#ifndef _WIN32
#include <termios.h>
#endif  // _WIN32
#include <stdio.h>

#include <ola/Callback.h>
#include <ola/io/StdinHandler.h>

namespace ola {
namespace io {

StdinHandler::StdinHandler(SelectServerInterface *ss,
                           InputCallback *callback)
#ifdef _WIN32
    : m_stdin_descriptor(0),
#else
    : m_stdin_descriptor(STDIN_FILENO),
#endif  // _WIN32
      m_ss(ss),
      m_callback(callback) {
  m_stdin_descriptor.SetOnData(
    ola::NewCallback(this, &StdinHandler::HandleData));
  // turn off buffering
#ifdef _WIN32
  setbuf(stdin, NULL);
#else
  tcgetattr(STDIN_FILENO, &m_old_tc);
  termios new_tc = m_old_tc;
  new_tc.c_lflag &= static_cast<tcflag_t>(~ICANON & ~ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &new_tc);
#endif  // _WIN32

  // Add to the SelectServer
  m_ss->AddReadDescriptor(&m_stdin_descriptor);
}


StdinHandler::~StdinHandler() {
  m_ss->RemoveReadDescriptor(&m_stdin_descriptor);
#ifndef _WIN32
  tcsetattr(STDIN_FILENO, TCSANOW, &m_old_tc);
#endif  // !_WIN32
}


void StdinHandler::HandleData() {
  int c = getchar();
  if (m_callback.get()) {
    m_callback->Run(c);
  }
}
}  // namespace io
}  // namespace ola
