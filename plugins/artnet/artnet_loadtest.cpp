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
 * artnet_loadtest.cpp
 * A simple Art-Net load tester
 * Copyright (C) 2013 Simon Newton
 */

#include <stdlib.h>
#include <algorithm>
#include <memory>
#include <string>
#include "ola/Callback.h"
#include "ola/DmxBuffer.h"
#include "ola/Logging.h"
#include "ola/base/Flags.h"
#include "ola/base/Init.h"
#include "ola/io/SelectServer.h"
#include "ola/network/InterfacePicker.h"
#include "plugins/artnet/ArtNetNode.h"

using ola::DmxBuffer;
using ola::NewCallback;
using ola::io::SelectServer;
using ola::network::Interface;
using ola::network::InterfacePicker;
using ola::plugin::artnet::ArtNetNode;
using ola::plugin::artnet::ArtNetNodeOptions;
using std::auto_ptr;
using std::cout;
using std::endl;
using std::min;

DEFINE_s_uint32(fps, f, 10, "Frames per second per universe [1 - 1000]");
DEFINE_s_uint16(universes, u, 1, "Number of universes to send");
DEFINE_string(iface, "", "The interface to send from");

/**
 * Send N DMX frames using Art-Net, where N is given by number_of_universes.
 */
bool SendFrames(ArtNetNode *node, DmxBuffer *buffer,
                uint16_t number_of_universes) {
  for (uint16_t i = 0; i < number_of_universes; i++) {
    node->SendDMX(i, *buffer);
  }
  return true;
}

int main(int argc, char* argv[]) {
  ola::AppInit(&argc, argv, "", "Run the E1.31 load test.");

  if (FLAGS_universes == 0 || FLAGS_fps == 0) {
    return -1;
  }

  unsigned int fps = min(1000u, static_cast<unsigned int>(FLAGS_fps));
  uint16_t universes = FLAGS_universes;

  DmxBuffer output;
  output.Blackout();

  Interface iface;
  {
    auto_ptr<InterfacePicker> picker(InterfacePicker::NewPicker());

    if (!picker->ChooseInterface(&iface, FLAGS_iface.str())) {
      return -1;
    }
  }

  ArtNetNodeOptions options;
  options.always_broadcast = true;

  SelectServer ss;
  ArtNetNode node(iface, &ss, options);

  for (uint16_t i = 0; i < universes; i++) {
    if (!node.SetInputPortUniverse(i, i)) {
      OLA_WARN << "Failed to set port";
    }
  }

  if (!node.Start()) {
    return -1;
  }

  ss.RegisterRepeatingTimeout(
      1000 / fps,
      NewCallback(&SendFrames, &node, &output, universes));
  cout << "Starting loadtester: " << universes << " universe(s), " << fps
       << " fps" << endl;
  ss.Run();
}
