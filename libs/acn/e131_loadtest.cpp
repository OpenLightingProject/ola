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
 * e131_loadtest.cpp
 * A simple E1.31 load tester
 * Copyright (C) 2013 Simon Newton
 */

#include <stdlib.h>
#include <algorithm>
#include <string>
#include "ola/Callback.h"
#include "ola/DmxBuffer.h"
#include "ola/Logging.h"
#include "ola/base/Flags.h"
#include "ola/base/Init.h"
#include "ola/io/SelectServer.h"
#include "libs/acn/E131Node.h"

using ola::DmxBuffer;
using ola::io::SelectServer;
using ola::acn::E131Node;
using ola::NewCallback;
using std::min;

DEFINE_s_uint32(fps, s, 10, "Frames per second per universe [1 - 40]");
DEFINE_s_uint16(universes, u, 1, "Number of universes to send");

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

int main(int argc, char* argv[]) {
  ola::AppInit(&argc, argv, "", "Run the E1.31 load test.");

  if (FLAGS_universes == 0 || FLAGS_fps == 0) {
    return -1;
  }

  unsigned int fps = min(40u, static_cast<unsigned int>(FLAGS_fps));
  uint16_t universes = FLAGS_universes;

  DmxBuffer output;
  output.Blackout();
  SelectServer ss;

  E131Node node(&ss, "", E131Node::Options());
  if (!node.Start()) {
    return -1;
  }

  ss.AddReadDescriptor(node.GetSocket());
  ss.RegisterRepeatingTimeout(
      1000 / fps,
      NewCallback(&SendFrames, &node, &output, universes));
  OLA_INFO << "Starting loadtester...";
  ss.Run();
}
