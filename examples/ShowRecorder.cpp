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
#include <ola/OlaDevice.h>
#include <ola/StringUtils.h>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "examples/ShowRecorder.h"

using std::vector;
using std::string;
using ola::DmxBuffer;


ShowRecorder::ShowRecorder(const string &filename,
                           const vector<unsigned int> &universes)
    : m_saver(filename),
      m_universes(universes),
      m_frame_count(0) {
}


ShowRecorder::~ShowRecorder() {
}


/**
 * Init the ShowRecorder
 */
int ShowRecorder::Init() {
  if (!m_client.Setup()) {
    OLA_FATAL << "Client Setup failed";
    return EX_UNAVAILABLE;
  }

  if (!m_saver.Open())
    return EX_CANTCREAT;

  m_client.GetClient()->SetDmxCallback(
      ola::NewCallback(this, &ShowRecorder::NewFrame));

  vector<unsigned int>::const_iterator iter = m_universes.begin();
  for (; iter != m_universes.end(); ++iter) {
    m_client.GetClient()->RegisterUniverse(
        *iter,
        ola::REGISTER,
        ola::NewSingleCallback(this, &ShowRecorder::RegisterComplete));
  }

  return EX_OK;
}


/**
 * Record the show.
 */
int ShowRecorder::Record() {
  m_client.GetSelectServer()->Run();
  return EX_OK;
}


/**
 * Stop recording
 */
void ShowRecorder::Stop() {
  m_client.GetSelectServer()->Terminate();
}


/**
 * Record the new frame
 */
void ShowRecorder::NewFrame(unsigned int universe,
                            const DmxBuffer &data,
                            const string &error) {
  if (!error.empty()) {
    OLA_WARN << error;
    return;
  }

  ola::TimeStamp now;
  m_clock.CurrentTime(&now);
  m_saver.NewFrame(now, universe, data);
  m_frame_count++;
}


void ShowRecorder::RegisterComplete(const string &error) {
  OLA_INFO << "register complete";
  (void) error;
}
