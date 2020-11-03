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
 * ShowSaver.cpp
 * Writes show data to a file.
 * Copyright (C) 2011 Simon Newton
 *
 * The data file is in the form:
 * universe-number channel1,channel2,channel3
 * delay-in-ms
 * universe-number channel1,channel2,channel3
 */

#include <errno.h>
#include <string.h>
#include <ola/DmxBuffer.h>
#include <ola/Logging.h>
#include <fstream>
#include <iostream>
#include <string>

#include "examples/ShowLoader.h"
#include "examples/ShowSaver.h"

using std::string;
using ola::DmxBuffer;
using std::endl;


ShowSaver::ShowSaver(const string &filename)
    : m_filename(filename) {
}


ShowSaver::~ShowSaver() {
  Close();
}


/**
 * Open the show file for writing.
 * @returns true if we could open the file, false otherwise.
 */
bool ShowSaver::Open() {
  m_show_file.open(m_filename.data());
  if (!m_show_file.is_open()) {
    OLA_FATAL << "Can't open " << m_filename << ": " << strerror(errno);
    return false;
  }

  m_show_file << GetHeader() << endl;
  return true;
}


/**
 * Close the show file
 */
void ShowSaver::Close() {
  if (m_show_file.is_open()) {
    m_show_file.close();
  }
}


ShowSaverV1::ShowSaverV1(const string &filename)
    : ShowSaver(filename) {
}


bool ShowSaverV1::NewFrame(const ola::TimeStamp &arrival_time,
                           unsigned int universe,
                           const ola::DmxBuffer &data) {
  // TODO(simon): add much better error handling here
  if (m_last_frame.IsSet()) {
    // this is not the first frame so write the delay in ms
    const ola::TimeInterval delta = arrival_time - m_last_frame;

    m_show_file << delta.InMilliSeconds() << endl;
  }
  m_last_frame = arrival_time;
  m_show_file << universe << " " << data.ToString() << endl;
  return true;
}


string ShowSaverV1::GetHeader() {
  return string(OLA_SHOW_HEADER_V1);
}


ShowSaverV2::ShowSaverV2(const string &filename) 
    : ShowSaver(filename) {
}


bool ShowSaverV2::NewFrame(const ola::TimeStamp &arrival_time,
                           unsigned int universe,
                           const ola::DmxBuffer &data) {
  // TODO(shenghao): add much better error handling here
  if (m_first_frame.IsSet()) {
    const ola::TimeInterval delta = arrival_time - m_first_frame;

    struct timeval tv;
    delta.AsTimeval(&tv);
    m_show_file << tv.tv_sec << " " << tv.tv_usec << endl;
  } else {
    m_first_frame = arrival_time;
  }

  m_show_file << universe << " " << data.ToString() << endl;
  return true;
}

string ShowSaverV2::GetHeader() {
  return string(OLA_SHOW_HEADER_V2);
}
