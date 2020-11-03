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
 * ShowSaver.h
 * Writes show data to a file.
 * Copyright (C) 2011 Simon Newton
 */

#include <ola/Clock.h>
#include <ola/DmxBuffer.h>

#include <string>
#include <fstream>

#ifndef EXAMPLES_SHOWSAVER_H_
#define EXAMPLES_SHOWSAVER_H_

/**
 * Write show data to a file.
 */
class ShowSaver {
 public:
  explicit ShowSaver(const std::string &filename);
  virtual ~ShowSaver();

  bool Open();
  void Close();

  virtual bool NewFrame(const ola::TimeStamp &arrival_time,
                        unsigned int universe,
                        const ola::DmxBuffer &data) = 0;
 protected:
  const std::string m_filename;
  std::ofstream m_show_file;

 private:
  virtual std::string GetHeader() = 0;
};


class ShowSaverV1: public ShowSaver {
 public:
  explicit ShowSaverV1(const std::string &filename);
  virtual bool NewFrame(const ola::TimeStamp &arrival_time,
                        unsigned int universe,
                        const ola::DmxBuffer &data);
 private:
  ola::TimeStamp m_last_frame;
  virtual std::string GetHeader();
};


  static const char OLA_SHOW_HEADER[];
};
#endif  // EXAMPLES_SHOWSAVER_H_
