/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *  ola-recorder.cpp
 *  A simple tool to record & playback shows.
 *  Copyright (C) 2011 Simon Newton
 *
 * The data file is in the form:
 * universe-number channel1,channel2,channel3
 * delay-in-ms
 * universe-number channel1,channel2,channel3
 */

#include <errno.h>
#include <string.h>
#include <sysexits.h>
#include <ola/Callback.h>
#include <ola/DmxBuffer.h>
#include <ola/Logging.h>
#include <ola/OlaCallbackClient.h>
#include <ola/OlaClientWrapper.h>
#include <ola/StringUtils.h>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "examples/ShowPlayer.h"

using std::vector;
using std::string;
using ola::DmxBuffer;


ShowPlayer::ShowPlayer(const string &filename)
    : m_loader(filename) {
}


ShowPlayer::~ShowPlayer() {
}


/**
 * Init the ShowPlayer
 */
int ShowPlayer::Init() {
  if (!m_client.Setup()) {
    OLA_FATAL << "Client Setup failed";
    return EX_UNAVAILABLE;
  }

  if (!m_loader.Load())
    return EX_NOINPUT;

  return EX_OK;
}


/**
 * Playback the show
 */
int ShowPlayer::Playback() {
  SendNextFrame();
  m_client.GetSelectServer()->Run();
  return EX_OK;
}



/**
 * Send the next dmx frame
 */
void ShowPlayer::SendNextFrame() {
  DmxBuffer buffer;
  unsigned int universe;
  bool ok = m_loader.NextFrame(&universe, &buffer);
  if (!ok) {
    m_client.GetSelectServer()->Terminate();
    return;
  }

  bool timeout_ok = RegisterNextTimeout();

  if (ok) {
    OLA_INFO << "Universe: " << universe << ": " << buffer.ToString();
    m_client.GetClient()->SendDmx(universe, buffer);
  }

  if (!timeout_ok)
    m_client.GetSelectServer()->Terminate();
}


/**
 * Get the next time offset
 */
bool ShowPlayer::RegisterNextTimeout() {
  unsigned int timeout;
  if (!m_loader.NextTimeout(&timeout))
    return false;

  OLA_INFO << "Registering timeout for " << timeout << "ms";
  m_client.GetSelectServer()->RegisterSingleTimeout(
      timeout,
      ola::NewSingleCallback(this, &ShowPlayer::SendNextFrame));
  return true;
}
