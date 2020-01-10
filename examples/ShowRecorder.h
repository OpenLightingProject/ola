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
 * ShowRecorder.h
 * Create recordings for the simple show playback system.
 * Copyright (C) 2011 Simon Newton
 */

#include <ola/Clock.h>
#include <ola/DmxBuffer.h>
#include <ola/client/ClientWrapper.h>
#include <stdint.h>
#include <string>
#include <fstream>
#include <vector>

#include "examples/ShowSaver.h"

#ifndef EXAMPLES_SHOWRECORDER_H_
#define EXAMPLES_SHOWRECORDER_H_

/**
 * The show player class
 */
class ShowRecorder {
 public:
  ShowRecorder(const std::string &filename,
               const std::vector<unsigned int> &universes,
               const unsigned int duration);
  ~ShowRecorder();

  int Init();
  int Record();
  void Stop();

  uint64_t FrameCount() const { return m_frame_count; }

 private:
  ola::client::OlaClientWrapper m_client;
  ShowSaver m_saver;
  std::vector<unsigned int> m_universes;
  unsigned int m_duration;
  ola::Clock m_clock;
  uint64_t m_frame_count;

  void NewFrame(const ola::client::DMXMetadata &meta,
                const ola::DmxBuffer &data);
  void RegisterComplete(const ola::client::Result &result);
};
#endif  // EXAMPLES_SHOWRECORDER_H_
