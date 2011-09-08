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
 *  ShowSaver.cpp
 *  A class that reads OLA show files
 *  Copyright (C) 2011 Simon Newton
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

#include "examples/ShowSaver.h"

using std::string;
using ola::DmxBuffer;
using std::endl;


const char ShowSaver::OLA_SHOW_HEADER[] = "OLA Show";

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

  m_show_file << OLA_SHOW_HEADER << endl;
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


/**
 * Write a new frame
 */
bool ShowSaver::NewFrame(const ola::TimeStamp &arrival_time,
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
