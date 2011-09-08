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
 *  ShowLoader.cpp
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
#include <ola/StringUtils.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "examples/ShowLoader.h"

using std::vector;
using std::string;
using ola::DmxBuffer;


const char ShowLoader::OLA_SHOW_HEADER[] = "OLA Show";

ShowLoader::ShowLoader(const string &filename)
    : m_filename(filename),
      m_line(0) {
}


ShowLoader::~ShowLoader() {
  if (m_show_file.is_open()) {
    m_show_file.close();
  }
}


/**
 * Load the show file.
 * @returns true if we could open the file, false otherwise.
 */
bool ShowLoader::Load() {
  m_show_file.open(m_filename.data());
  if (!m_show_file.is_open()) {
    OLA_FATAL << "Can't open " << m_filename << ": " << strerror(errno);
    return false;
  }

  string line;
  ReadLine(&line);
  if (line != OLA_SHOW_HEADER) {
    OLA_WARN << "Invalid show file";
    return false;
  }
  return true;
}


/**
 * Get the next time offset
 * @param timeout a pointer to the timeout in ms
 */
bool ShowLoader::NextTimeout(unsigned int *timeout) {
  string line;
  ReadLine(&line);
  if (line.empty())
    return false;

  if (!ola::StringToInt(line, timeout, true)) {
    OLA_WARN << "Line " << m_line << ": Invalid timeout: " << line;
    return false;
  }
  return true;
}


/**
 * Read the next DMX frame.
 * @param universe the universe to send on
 * @param data the DMX data
 */
bool ShowLoader::NextFrame(unsigned int *universe, DmxBuffer *data) {
  string line;
  ReadLine(&line);

  if (line.empty())
    return false;

  vector<string> inputs;
  ola::StringSplit(line, inputs);

  if (inputs.size() != 2) {
    OLA_WARN << "Line " << m_line << " invalid: " << line;
    return false;
  }

  if (!ola::StringToInt(inputs[0], universe, true)) {
    OLA_WARN << "Line " << m_line << " invalid: " << line;
    return false;
  }

  return data->SetFromString(inputs[1]);
}


void ShowLoader::ReadLine(string *line) {
  getline(m_show_file, *line);
  m_line++;
}
