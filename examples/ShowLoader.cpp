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
 * ShowLoader.cpp
 * A class that reads OLA show files
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
#include <ola/StringUtils.h>
#include <fstream>
#include <ios>
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
    OLA_WARN << "Invalid show file, expecting " << OLA_SHOW_HEADER << " got "
             << line;
    return false;
  }
  return true;
}


/**
 * Reset to the start of the show
 */
void ShowLoader::Reset() {
  m_show_file.clear();
  m_show_file.seekg(0, std::ios::beg);
  // skip over the first line
  string line;
  ReadLine(&line);
}


/**
 * @brief Get most recent line number read (1-indexed)
 */
unsigned int ShowLoader::GetCurrentLineNumber() const {
  return m_line;
}


/**
 * Get the next time offset
 * @param timeout a pointer to the timeout in ms
 */
ShowLoader::State ShowLoader::NextTimeout(unsigned int *timeout) {
  string line;
  ReadLine(&line);
  if (line.empty()) {
    return END_OF_FILE;
  }

  if (!ola::StringToInt(line, timeout, true)) {
    OLA_WARN << "Line " << m_line << ": Invalid timeout: " << line;
    return INVALID_LINE;
  }
  return OK;
}


/**
 * Read the next DMX frame.
 * @param universe the universe to send on
 * @param data the DMX data
 */
ShowLoader::State ShowLoader::NextFrame(unsigned int *universe,
                                        DmxBuffer *data) {
  string line;
  ReadLine(&line);

  if (line.empty()) {
    return END_OF_FILE;
  }

  vector<string> inputs;
  ola::StringSplit(line, &inputs);

  if (inputs.size() != 2) {
    OLA_WARN << "Line " << m_line << " invalid: " << line;
    return INVALID_LINE;
  }

  if (!ola::StringToInt(inputs[0], universe, true)) {
    OLA_WARN << "Line " << m_line << " invalid: " << line;
    return INVALID_LINE;
  }

  return (data->SetFromString(inputs[1]) ? OK : INVALID_LINE);
}


/**
 * Read the next show file entry
 * @param entry a ShowEntry to fill with data
 */
ShowLoader::State ShowLoader::NextEntry(ShowEntry *entry) {
  State state = NextFrame(&entry->universe, &entry->buffer);
  if (state != OK) {
    return state;
  }

  state = NextTimeout(&entry->next_wait);
  if (state == END_OF_FILE) {
    // Ensure the entry is whole before sending.
    entry->next_wait = 0;
  }

  return state;
}


void ShowLoader::ReadLine(string *line) {
  getline(m_show_file, *line);
  ola::StripSuffix(line, "\r");
  m_line++;
}
