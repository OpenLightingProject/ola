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
 *  ShowSaver.h
 *  Writes show data to a file.
 *  Copyright (C) 2011 Simon Newton
 */

#include <ola/Clock.h>
#include <ola/DmxBuffer.h>

#include <string>
#include <fstream>

using std::string;

#ifndef SRC_SHOWSAVER_H
#define SRC_SHOWSAVER_H

/**
 * Write show data to a file.
 */
class ShowSaver {
  public:
    ShowSaver(const string &filename);
    ~ShowSaver();

    bool Open();
    void Close();

    bool NewFrame(const ola::TimeStamp &arrival_time,
                  unsigned int universe,
                  const ola::DmxBuffer &data);

  private:
    const string m_filename;
    std::ofstream m_show_file;
    ola::TimeStamp m_last_frame;

    static const char OLA_SHOW_HEADER[];
};
#endif  // SRC_SHOWSAVER_H
