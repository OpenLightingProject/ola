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
 *  ShowLoader.h
 *  A simple show playback system.
 *  Copyright (C) 2011 Simon Newton
 */

#include <ola/DmxBuffer.h>

#include <string>
#include <fstream>

using std::string;

#ifndef EXAMPLES_SHOWLOADER_H_
#define EXAMPLES_SHOWLOADER_H_

/**
 * Loads a show file and reads the DMX data.
 */
class ShowLoader {
  public:
    explicit ShowLoader(const string &filename);
    ~ShowLoader();

    bool Load();

    bool NextTimeout(unsigned int *timeout);
    bool NextFrame(unsigned int *universe, ola::DmxBuffer *data);

  private:
    const string m_filename;
    std::ifstream m_show_file;
    unsigned int m_line;

    static const char OLA_SHOW_HEADER[];

    void ReadLine(string *line);
};
#endif  // EXAMPLES_SHOWLOADER_H_
