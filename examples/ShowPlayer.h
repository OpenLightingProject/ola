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
 * ShowPlayer.h
 * A simple show playback system.
 * Copyright (C) 2011 Simon Newton
 */

#include <ola/DmxBuffer.h>
#include <ola/client/ClientWrapper.h>

#include <string>
#include <fstream>

#include "examples/ShowLoader.h"

#ifndef EXAMPLES_SHOWPLAYER_H_
#define EXAMPLES_SHOWPLAYER_H_

/**
 * @brief A class which plays back recorded show files.
 */
class ShowPlayer {
 public:
  /**
   * @brief Create a new ShowPlayer
   * @param filename the show file to play
   */
  explicit ShowPlayer(const std::string &filename);
  ~ShowPlayer();

  /**
   * @brief Initialize the show player.
   * @return EXIT_OK if successful.
   */
  int Init();

  /**
   * @brief Playback the show
   * @param iterations the number of iterations of the show to play.
   * @param duration the duration in seconds after which playback is stopped.
   * @param delay the hold time at the end of a show before playback starts
   * from the beginning again.
   */
  int Playback(unsigned int iterations,
               unsigned int duration,
               unsigned int delay);

 private:
  ola::client::OlaClientWrapper m_client;
  ShowLoader m_loader;
  bool m_infinite_loop;
  unsigned int m_iteration_remaining;
  unsigned int m_loop_delay;

  void SendNextFrame();
  ShowLoader::State RegisterNextTimeout();
  bool ReadNextFrame(unsigned int *universe, ola::DmxBuffer *data);
  void HandleEndOfFile();
};
#endif  // EXAMPLES_SHOWPLAYER_H_
