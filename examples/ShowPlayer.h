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

#include <map>
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
   * @param simulate if true, no connection will be made to olad and nothing
   * will be sent for output.
   * @return EXIT_OK if successful.
   */
  int Init(bool simulate = false);

  /**
   * @brief Playback the show
   * @param iterations the number of iterations of the show to play.
   * @param duration the duration (in seconds) after which playback is stopped.
   * @param delay the hold time (in milliseconds) at the end of a show before
   * playback starts from the beginning again.
   * @param start the time (in milliseconds) to start playback from
   * @param stop the time (in milliseconds) to stop playback from; 0 will run
   * until EOF.
   */
  int Playback(unsigned int iterations,
               uint64_t duration,
               uint64_t delay,
               uint64_t start = 0,
               uint64_t stop = 0);


  uint64_t GetRunTime() const {
    return m_run_time;
  }


  const std::map<unsigned int, uint64_t> &GetFrameCount() const {
    return m_frame_count;
  }


 private:
  ola::client::OlaClientWrapper m_client;
  ShowLoader m_loader;
  bool m_infinite_loop;
  unsigned int m_iteration_remaining;
  uint64_t m_loop_delay;
  uint64_t m_start;
  uint64_t m_stop;
  uint64_t m_playback_pos;
  uint64_t m_run_time;
  std::map<unsigned int, uint64_t> m_frame_count;
  bool m_simulate;

  /** Used for tracking simulation progress */
  typedef enum {
    TASK_COMPLETE,
    TASK_NEXT_FRAME,
    TASK_LOOP,
  } Task;
  Task m_next_task;
  int m_status;

  void Loop();
  ShowLoader::State SeekTo(uint64_t seek_time);
  void SendNextFrame();
  void SendEntry(const ShowEntry &entry);
  void RegisterNextTimeout(unsigned int timeout);
  void SendFrame(const ShowEntry &entry);
  void HandleEndOfShow();
  void HandleInvalidLine();
  void StopPlayback(int exit_status);
};
#endif  // EXAMPLES_SHOWPLAYER_H_
