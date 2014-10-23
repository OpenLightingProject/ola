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
 * Copyright (C) 2010 Simon Newton
 */
//! [Tutorial Example] NOLINT(whitespace/comments)
#include <stdlib.h>
#include <unistd.h>
#include <ola/DmxBuffer.h>
#include <ola/Logging.h>
#include <ola/client/StreamingClient.h>

#include <iostream>

using std::cout;
using std::endl;

int main(int, char *[]) {
  unsigned int universe = 1;  // universe to use for sending data

  // turn on OLA logging
  ola::InitLogging(ola::OLA_LOG_WARN, ola::OLA_LOG_STDERR);

  ola::DmxBuffer buffer;  // A DmxBuffer to hold the data.
  buffer.Blackout();  // Set all channels to 0

  // Create a new client.
  ola::client::StreamingClient ola_client(
      (ola::client::StreamingClient::Options()));

  // Setup the client, this connects to the server
  if (!ola_client.Setup()) {
    std::cerr << "Setup failed" << endl;
    exit(1);
  }

  // Send 100 frames to the server. Increment slot (channel) 0 each time a
  // frame is sent.
  for (unsigned int i = 0; i < 100; i++) {
    buffer.SetChannel(0, i);
    if (!ola_client.SendDmx(universe, buffer)) {
      cout << "Send DMX failed" << endl;
      exit(1);
    }
    usleep(25000);   // sleep for 25ms between frames.
  }
  return 0;
}
//! [Tutorial Example] NOLINT(whitespace/comments)
