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
 * ShowPlayer.cpp
 * A simple show playback system.
 * Copyright (C) 2011 Simon Newton
 *
 * The data file is in the form:
 * universe-number channel1,channel2,channel3
 * delay-in-ms
 * universe-number channel1,channel2,channel3
 */

#include <errno.h>
#include <string.h>
#include <ola/Callback.h>
#include <ola/DmxBuffer.h>
#include <ola/Logging.h>
#include <ola/StringUtils.h>
#include <ola/base/SysExits.h>
#include <ola/client/ClientWrapper.h>
#include <ola/client/OlaClient.h>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "examples/ShowPlayer.h"

using std::vector;
using std::string;
using ola::DmxBuffer;


ShowPlayer::ShowPlayer(const string &filename)
    : m_loader(filename),
      m_infinite_loop(false),
      m_iteration_remaining(0),
      m_loop_delay(0) {
}

ShowPlayer::~ShowPlayer() {}

int ShowPlayer::Init() {
  if (!m_client.Setup()) {
    OLA_FATAL << "Client Setup failed";
    return ola::EXIT_UNAVAILABLE;
  }

  if (!m_loader.Load()) {
    return ola::EXIT_NOINPUT;
  }

  return ola::EXIT_OK;
}

int ShowPlayer::Playback(unsigned int iterations,
                         unsigned int duration,
                         unsigned int delay) {
  m_infinite_loop = iterations == 0 || duration != 0;
  m_iteration_remaining = iterations;
  m_loop_delay = delay;
  SendNextFrame();

  ola::io::SelectServer *ss = m_client.GetSelectServer();

  if (duration != 0) {
    ss->RegisterSingleTimeout(
        duration * 1000,
        ola::NewSingleCallback(ss, &ola::io::SelectServer::Terminate));
  }
  ss->Run();
  return ola::EXIT_OK;
}

void ShowPlayer::SendNextFrame() {
  DmxBuffer buffer;
  unsigned int universe;
  ShowLoader::State state = m_loader.NextFrame(&universe, &buffer);
  switch (state) {
    case ShowLoader::END_OF_FILE:
      HandleEndOfFile();
      return;
    case ShowLoader::INVALID_LINE:
      m_client.GetSelectServer()->Terminate();
      return;
    default:
      {}
  }

  state = RegisterNextTimeout();

  OLA_INFO << "Universe: " << universe << ": " << buffer.ToString();
  ola::client::SendDMXArgs args;
  m_client.GetClient()->SendDMX(universe, buffer, args);

  switch (state) {
    case ShowLoader::END_OF_FILE:
      HandleEndOfFile();
      return;
    case ShowLoader::INVALID_LINE:
      m_client.GetSelectServer()->Terminate();
      return;
    default:
      {}
  }
}


/**
 * Get the next time offset
 */
ShowLoader::State ShowPlayer::RegisterNextTimeout() {
  unsigned int timeout;
  ShowLoader::State state = m_loader.NextTimeout(&timeout);
  if (state != ShowLoader::OK) {
    return state;
  }

  OLA_INFO << "Registering timeout for " << timeout << "ms";
  m_client.GetSelectServer()->RegisterSingleTimeout(
      timeout,
      ola::NewSingleCallback(this, &ShowPlayer::SendNextFrame));
  return state;
}


/**
 * Handle the case where we reach the end of file
 */
void ShowPlayer::HandleEndOfFile() {
  m_iteration_remaining--;
  if (m_infinite_loop || m_iteration_remaining > 0) {
    m_loader.Reset();
    m_client.GetSelectServer()->RegisterSingleTimeout(
        m_loop_delay,
        ola::NewSingleCallback(this, &ShowPlayer::SendNextFrame));
    return;
  } else {
    // stop the show
    m_client.GetSelectServer()->Terminate();
  }
}
