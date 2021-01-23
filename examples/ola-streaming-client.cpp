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
 * ola-streaming-client.cpp
 * The streaming client example program.
 * Copyright (C) 2005 Simon Newton
 */

#include <stdlib.h>
#include <ola/DmxBuffer.h>
#include <ola/Logging.h>
#include <ola/client/StreamingClient.h>
#include <ola/StringUtils.h>
#include <ola/base/Flags.h>
#include <ola/base/Init.h>
#include <ola/base/SysExits.h>
#include <ola/dmx/SourcePriorities.h>

#include <iostream>
#include <string>

using std::cout;
using std::endl;
using std::string;
using ola::client::StreamingClient;

DEFINE_s_string(dmx, d, "", "Comma separated DMX values to send, e.g. "
                            "0,255,128 sets first channel to 0, second "
                            "channel to 255 and third channel to 128.");
DEFINE_s_uint32(universe, u, 1, "The universe to send data for");
DEFINE_uint8(priority, ola::dmx::SOURCE_PRIORITY_DEFAULT,
             "The source priority to send data at");
DEFINE_s_default_bool(universe_from_stdin, s, false,
                      "Also read the destination universe number from STDIN "
                      "when reading DMX data from STDIN. The universe number "
                      "must precede the channel values, and be delimited by "
                      "whitespace, e.g. 1 0,255,128 2 0,255,127");

bool terminate = false;

bool SendDataFromString(StreamingClient *client,
                        unsigned int universe,
                        const string &data) {
  StreamingClient::SendArgs args;
  args.priority = FLAGS_priority;

  ola::DmxBuffer buffer;
  bool status = buffer.SetFromString(data);

  if (!status || buffer.Size() == 0) {
    return false;
  }

  if (!client->SendDMX(universe, buffer, args)) {
    cout << "Send DMX failed" << endl;
    terminate = true;
    return false;
  }
  return true;
}

int main(int argc, char *argv[]) {
  ola::AppInit(&argc, argv, "--dmx <dmx_data> --universe <universe_id>",
               "Send DMX512 data to OLA. If DMX512 data isn't provided, it "
               "will read from STDIN.");

  StreamingClient ola_client;
  if (!ola_client.Setup()) {
    OLA_FATAL << "Setup failed";
    exit(ola::EXIT_SOFTWARE);
  }

  if (FLAGS_dmx.str().empty()) {
    string input;
    bool have_universe = false;
    unsigned int universe = FLAGS_universe;

    while (!terminate && std::cin >> input) {
      if (!have_universe && FLAGS_universe_from_stdin) {
        if (!ola::StringToInt(input, &universe, true)) {
          OLA_FATAL << "Could not convert universe number, read " << input;
          exit(ola::EXIT_DATAERR);
        }

        have_universe = true;
        continue;
      }

      if (have_universe || !FLAGS_universe_from_stdin) {
        SendDataFromString(&ola_client, universe, input);
        have_universe = false;
      }
    }
  } else {
    if (FLAGS_universe_from_stdin) {
      OLA_FATAL << "Not reading from STDIN. Use -u to specify universe.";
      exit(ola::EXIT_USAGE);
    }
    SendDataFromString(&ola_client, FLAGS_universe, FLAGS_dmx.str());
  }
  ola_client.Stop();
  return ola::EXIT_OK;
}
