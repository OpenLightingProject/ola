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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * e131_loadtest.cpp
 * A simple E1.31 load tester
 * Copyright (C) 2013 Simon Newton
 */

#include "plugins/e131/e131/E131Includes.h"  //  NOLINT, this has to be first
#include <getopt.h>
#include <stdlib.h>
#include <algorithm>
#include <string>
#include "ola/Callback.h"
#include "ola/DmxBuffer.h"
#include "ola/Logging.h"
#include "ola/io/SelectServer.h"
#include "plugins/e131/e131/E131Node.h"

using ola::DmxBuffer;
using ola::io::SelectServer;
using ola::plugin::e131::E131Node;
using ola::NewCallback;
using std::cout;
using std::min;
using std::max;
using std::endl;

/**
 * Send N DMX frames using E1.31, where N is given by number_of_universes.
 */
bool SendFrames(E131Node *node, DmxBuffer *buffer,
                uint16_t number_of_universes) {
  for (uint16_t i = 1; i < number_of_universes + 1; i++) {
    node->SendDMX(i, *buffer);
  }
  return true;
}

/**
 * Run the load test until we get a terminate signal
 */
void RunLoadTest(uint16_t number_of_universes, unsigned int frames_per_second) {
  DmxBuffer output;
  output.Blackout();

  E131Node node("");
  if (!node.Start())
    return;

  SelectServer ss;
  ss.AddReadDescriptor(node.GetSocket());
  ss.RegisterRepeatingTimeout(
      1000 / frames_per_second,
      NewCallback(&SendFrames, &node, &output, number_of_universes));
  OLA_INFO << "Starting loadtester...";
  ss.Run();
}

/*
 * Display the help message
 */
void DisplayHelp(const char *binary_name) {
  cout << "Usage: " << binary_name << "\n"
  "\n"
  "Run the E1.31 load test.\n"
  "\n"
  "  -h, --help          Display this help message and exit.\n"
  "  -s, --fps           Frames per second [1, 40].\n"
  "  -u, --universes     Number of universes to send.\n"
  << endl;
}

int main(int argc, char* argv[]) {
  int number_of_universes = 1;
  int frames_per_second = 10;

  ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);

  static struct option long_options[] = {
      {"fps", required_argument, 0, 'f'},
      {"help", no_argument, 0, 'h'},
      {"universes", required_argument, 0, 'u'},
      {0, 0, 0, 0}
    };

  int option_index = 0;

  while (1) {
    int c = getopt_long(argc, argv, "f:hu:", long_options, &option_index);

    if (c == -1)
      break;

    switch (c) {
      case 0:
        break;
      case 'f':
        frames_per_second = atoi(optarg);
        break;
      case 'h':
        DisplayHelp(argv[0]);
        return 0;
      case 'u':
        number_of_universes = atoi(optarg);
        break;
      case '?':
        break;
      default:
        break;
    }
  }

  if (number_of_universes <= 0 || frames_per_second <= 0)
    return -1;

  unsigned int fps = max(
      1u,
      min(40u, static_cast<unsigned int>(frames_per_second)));
  uint16_t universe_count = static_cast<uint16_t>(number_of_universes);
  RunLoadTest(universe_count, fps);
}
