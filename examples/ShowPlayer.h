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
 *  ShowPlayer.h
 *  A simple show playback system.
 *  Copyright (C) 2011 Simon Newton
 */

#include <ola/DmxBuffer.h>
#include <ola/OlaClientWrapper.h>

#include <string>
#include <fstream>

#include "examples/ShowLoader.h"


using std::string;


#ifndef EXAMPLES_SHOWPLAYER_H_
#define EXAMPLES_SHOWPLAYER_H_

/**
 * The show player class
 */
class ShowPlayer {
  public:
    explicit ShowPlayer(const string &filename);
    ~ShowPlayer();

    int Init();
    int Playback(unsigned int iterations,
                 unsigned int delay);

    void SendNextFrame();

  private:
    ola::OlaCallbackClientWrapper m_client;
    ShowLoader m_loader;
    bool m_infinte_loop;
    unsigned int m_iteration_remaining;
    unsigned int m_loop_delay;

    ShowLoader::State RegisterNextTimeout();
    bool ReadNextFrame(unsigned int *universe, ola::DmxBuffer *data);
    void HandleEndOfFile();
};
#endif  // EXAMPLES_SHOWPLAYER_H_
