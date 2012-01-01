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
 *  ShowRecorder.h
 *  A simple show playback system.
 *  Copyright (C) 2011 Simon Newton
 */

#include <ola/Clock.h>
#include <ola/DmxBuffer.h>
#include <ola/OlaClientWrapper.h>
#include <string>
#include <fstream>
#include <vector>

#include "examples/ShowSaver.h"

using std::string;


#ifndef EXAMPLES_SHOWRECORDER_H_
#define EXAMPLES_SHOWRECORDER_H_

/**
 * The show player class
 */
class ShowRecorder {
  public:
    ShowRecorder(const string &filename,
                 const std::vector<unsigned int> &universes);
    ~ShowRecorder();

    int Init();
    int Record();

  private:
    ola::OlaCallbackClientWrapper m_client;
    ShowSaver m_saver;
    std::vector<unsigned int> m_universes;
    ola::Clock m_clock;

    void NewFrame(unsigned int universe,
                  const ola::DmxBuffer &data,
                  const string &error);
    void RegisterComplete(const string &error);
};
#endif  // EXAMPLES_SHOWRECORDER_H_
