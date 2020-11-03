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
 * ShowLoader.h
 * A simple show playback system.
 * Copyright (C) 2011 Simon Newton
 */

#include <ola/DmxBuffer.h>

#include <memory>
#include <string>
#include <fstream>

#ifndef EXAMPLES_SHOWLOADER_H_
#define EXAMPLES_SHOWLOADER_H_

/**
 * Show header for V1 showfiles.
 *
 * - Relative frame times (up to ms resolution).
 * - DMX data in human-readable form.
 */
extern const char OLA_SHOW_HEADER_V1[];

/**
 * Show header for V2 showfiles.
 *
 * - Absolute frame times (up to ms resolution).
 * - DMX data in human-readable form.
 */
extern const char OLA_SHOW_HEADER_V2[];


/**
 * Holds a single entry in the show file
 */
struct ShowEntry {
  unsigned int universe;
  ola::DmxBuffer buffer;
  unsigned int next_wait;
};

/**
 * Loads a show file and reads the DMX data.
 */
class ShowLoader {
 public:
  explicit ShowLoader(const std::string &filename);
  virtual ~ShowLoader();

  typedef enum {
    OK,
    INVALID_LINE,
    END_OF_FILE,
  } State;

  bool Load();
  virtual void Reset();
  unsigned int GetCurrentLineNumber() const;

  State NextEntry(ShowEntry *entry);

 protected:
  void ReadLine(std::string *line);

 private:
  const std::string m_filename;
  std::ifstream m_show_file;
  unsigned int m_line;

  virtual bool VerifyVersion() = 0;
  virtual State NextTimeout(unsigned int *timeout) = 0;
  virtual State NextFrame(unsigned int *universe, ola::DmxBuffer *data);
};

/**
 * LoadShow return an appropriate \c ShowLoader based on the format of the
 * showfile.
 *
 * @param filename name of file
 * @return Pointer to show loader. \c NULL if no show loader can be found.
 */
std::auto_ptr<ShowLoader> LoadShow(const std::string &filename);

#endif  // EXAMPLES_SHOWLOADER_H_
