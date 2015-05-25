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
//! [Client Disconnect Example] NOLINT(whitespace/comments)
#include <stdint.h>
#include <ola/DmxBuffer.h>
#include <ola/io/SelectServer.h>
#include <ola/Logging.h>
#include <ola/client/ClientWrapper.h>
#include <ola/Callback.h>

using std::cout;
using std::endl;

// Called when the connection to olad is closed.
void ConnectionClosed(ola::io::SelectServer *ss) {
  std::cerr << "Connection to olad was closed" << endl;
  ss->Terminate();  // terminate the program.
}

bool SendData(ola::client::OlaClientWrapper *wrapper) {
  static unsigned int universe = 1;
  static uint8_t i = 0;
  ola::DmxBuffer buffer;
  buffer.Blackout();
  buffer.SetChannel(0, i++);

  wrapper->GetClient()->SendDMX(universe, buffer, ola::client::SendDMXArgs());
  return true;
}

int main(int, char *[]) {
  ola::InitLogging(ola::OLA_LOG_WARN, ola::OLA_LOG_STDERR);
  ola::client::OlaClientWrapper wrapper;

  if (!wrapper.Setup()) {
    std::cerr << "Setup failed" << endl;
    exit(1);
  }

  // Create a timeout and register it with the SelectServer
  ola::io::SelectServer *ss = wrapper.GetSelectServer();
  ss->RegisterRepeatingTimeout(25, ola::NewCallback(&SendData, &wrapper));

  // Register the on-close handler
  wrapper.GetClient()->SetCloseHandler(
      ola::NewSingleCallback(ConnectionClosed, ss));

  // Start the main loop
  ss->Run();
}
//! [Client Disconnect Example] NOLINT(whitespace/comments)
