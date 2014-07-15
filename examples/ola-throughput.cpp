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
 * ola-throughput.cpp
 * Send a bunch of frames quickly to load test the server.
 * Copyright (C) 2005 Simon Newton
 */

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <ola/base/Flags.h>
#include <ola/base/Init.h>
#include <ola/DmxBuffer.h>
#include <ola/Logging.h>
#include <ola/StreamingClient.h>
#include <ola/StringUtils.h>

#include <iostream>
#include <string>

using std::cout;
using std::endl;
using std::string;
using ola::StreamingClient;

DEFINE_s_uint32(universe, u, 1, "The universe to send data on");
DEFINE_s_uint32(sleep, s, 40000, "Time between DMX updates in micro-seconds");

/*
 * Main
 */
int main(int argc, char *argv[]) {
  ola::AppInit(&argc, argv, "[options]", "Send DMX512 data to OLA.");

  StreamingClient ola_client;
  if (!ola_client.Setup()) {
    OLA_FATAL << "Setup failed";
    exit(1);
  }

  ola::DmxBuffer buffer;
  buffer.Blackout();

  while (1) {
    usleep(FLAGS_sleep);
    if (!ola_client.SendDmx(FLAGS_universe, buffer)) {
      cout << "Send DMX failed" << endl;
      return false;
    }
  }
  return 0;
}
