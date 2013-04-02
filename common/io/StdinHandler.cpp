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

#include <termios.h>
#include <stdio.h>

#include <ola/Callback.h>
#include <ola/io/StdinHandler.h>

namespace ola {
namespace io {

StdinHandler::StdinHandler(SelectServerInterface *ss,
                           InputCallback *callback)
    : m_stdin_descriptor(STDIN_FILENO),
      m_ss(ss),
      m_callback(callback) {
  m_stdin_descriptor.SetOnData(
    ola::NewCallback(this, &StdinHandler::HandleData));
  // turn off buffering
  tcgetattr(STDIN_FILENO, &m_old_tc);
  termios new_tc = m_old_tc;
  new_tc.c_lflag &= static_cast<tcflag_t>(~ICANON & ~ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &new_tc);

  // Add to the SelectServer
  m_ss->AddReadDescriptor(&m_stdin_descriptor);
}


StdinHandler::~StdinHandler() {
  m_ss->RemoveReadDescriptor(&m_stdin_descriptor);
  tcsetattr(STDIN_FILENO, TCSANOW, &m_old_tc);
}


void StdinHandler::HandleData() {
  char c = getchar();
  if (m_callback.get())
    m_callback->Run(c);
}
}  // io
}  // ola
