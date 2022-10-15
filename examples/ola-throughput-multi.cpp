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
 * ola-throughput-multi.cpp
 * Send a bunch of frames quickly on multiple universes to load test the server.
 * Copyright (C) 2005 Simon Newton
 */

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <ola/base/Flags.h>
#include <ola/base/Init.h>
#include <ola/client/ClientWrapper.h>
#include <ola/io/SelectServer.h>
#include <ola/DmxBuffer.h>
#include <ola/Logging.h>
#include <ola/StreamingClient.h>
#include <ola/StringUtils.h>

#include <chrono>  // NOLINT
#include <iostream>
#include <string>
#include <vector>

using std::cout;
using std::endl;
using std::string;
using ola::client::OlaClientWrapper;
using ola::StreamingClient;
using std::chrono::milliseconds;

const unsigned int MICROSECONDS_PER_MILLISECOND = 1000;

DEFINE_s_uint32(universes, u, 24, "The number of universes to send data on");
DEFINE_s_uint32(sleep, s, 40000, "Time between DMX updates in micro-seconds");
DEFINE_s_default_bool(oscillate_data, d, false,
                      "Flip all channels in each universe between 0 and 255 "
                      "for each frame. CAUTION: This will produce rapid "
                      "strobing on any connected outputs!");
DEFINE_s_default_bool(advanced, a, false, "Use the advanced ClientWrapper API "
                      "instead of the StreamingClient API");

void oscillateData(std::vector<ola::DmxBuffer> &buffers) {
  static bool channelsOnNext = true;
  for (size_t i = 0; i < FLAGS_universes - 1; i++) {
    if (channelsOnNext) {
      for (size_t j = 0; j < 512; j++) {
        buffers[i].SetChannel(j, 255);
      }
      channelsOnNext = false;
    } else {
      buffers[i].Blackout();
      channelsOnNext = true;
    }
  }
}

void AdvancedConnectionClosed(ola::io::SelectServer *ss) {
  std::cerr << "Connection to olad was closed" << endl;
  ss->Terminate();  // terminate the program.
}

bool AdvancedSendData(ola::client::OlaClientWrapper *wrapper,
                      std::vector<ola::DmxBuffer> *buffers) {
  if (FLAGS_oscillate_data) {
    oscillateData(*buffers);
  }

  auto startTime = std::chrono::high_resolution_clock::now();
  for (size_t i = 0; i < FLAGS_universes - 1; i++) {
    wrapper->GetClient()->SendDMX(
      i + 1,
      (*buffers)[i],
      ola::client::SendDMXArgs());
  }
  auto endTime = (std::chrono::high_resolution_clock::now() - startTime);
  auto endTimeMs = (
    std::chrono::duration_cast<milliseconds>(endTime).count());
  printf("frame time: %04ld ms\n", endTimeMs);  // fast write
  return true;
}

/*
 * Main
 */
int main(int argc, char *argv[]) {
  ola::AppInit(&argc, argv, "[options]", "Send DMX512 data to OLA.");

  OlaClientWrapper wrapper;
  StreamingClient ola_client;
  if (FLAGS_advanced) {
    if (!wrapper.Setup()) {
      OLA_FATAL << "Setup failed";
      exit(1);
    }
  } else {
    if (!ola_client.Setup()) {
      OLA_FATAL << "Setup failed";
      exit(1);
    }
  }

  std::vector<ola::DmxBuffer> buffers;
  for (size_t i = 0; i < FLAGS_universes - 1; i++) {
    buffers.push_back(ola::DmxBuffer());
    buffers[i].Blackout();
  }

  if (FLAGS_advanced) {
    ola::io::SelectServer *ss = wrapper.GetSelectServer();
    ss->RegisterRepeatingTimeout(
      FLAGS_sleep / MICROSECONDS_PER_MILLISECOND,
      ola::NewCallback(&AdvancedSendData, &wrapper, &buffers));

    wrapper.GetClient()->SetCloseHandler(
    ola::NewSingleCallback(AdvancedConnectionClosed, ss));

    ss->Run();
  } else {
    while (1) {
      usleep(FLAGS_sleep);

      if (FLAGS_oscillate_data) {
        oscillateData(buffers);
      }

      auto startTime = std::chrono::high_resolution_clock::now();
      for (size_t i = 0; i < FLAGS_universes - 1; i++) {
        if (!ola_client.SendDmx(i + 1, buffers[i])) {
          cout << "Send DMX failed" << endl;
          exit(1);
        }
      }
      auto endTime = (std::chrono::high_resolution_clock::now() - startTime);
      auto endTimeMs = (
        std::chrono::duration_cast<milliseconds>(endTime).count());
      printf("frame time: %04ld ms\n", endTimeMs);  // fast write
    }
  }
  return 0;
}
