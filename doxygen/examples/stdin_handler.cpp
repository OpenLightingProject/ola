/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Copyright (C) 2015 Simon Newton
 */
//! [Example] NOLINT(whitespace/comments)
#include <ola/base/Init.h>
#include <ola/io/SelectServer.h>
#include <ola/io/StdinHandler.h>
#include <iostream>

/**
 * @brief An example StdinHandler.
 */
class ExampleStdinHandler {
 public:
  ExampleStdinHandler()
    : m_stdin_handler(&m_ss,
                      ola::NewCallback(this, &ExampleStdinHandler::Input)) {
  }

  void Run() { m_ss.Run(); }

 private:
  ola::io::SelectServer m_ss;
  ola::io::StdinHandler m_stdin_handler;

  void Input(int c);
};

void ExampleStdinHandler::Input(int c) {
  switch (c) {
    case 'q':
      m_ss.Terminate();
      break;
    default:
      std::cout << "Got " << static_cast<char>(c) << " - " << c << std::endl;
      break;
  }
}

int main(int argc, char* argv[]) {
  ola::AppInit(&argc, argv, "", "Example Stdin Handler");

  ExampleStdinHandler handler;
  handler.Run();
}
//! [Example] NOLINT(whitespace/comments)
