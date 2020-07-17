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
 * ShowPlayer.cpp
 * A simple show playback system.
 * Copyright (C) 2011 Simon Newton
 *
 * The data file is in the form:
 * universe-number channel1,channel2,channel3
 * delay-in-ms
 * universe-number channel1,channel2,channel3
 */

#include <errno.h>
#include <string.h>
#include <ola/Callback.h>
#include <ola/Logging.h>
#include <ola/StringUtils.h>
#include <ola/base/SysExits.h>
#include <ola/client/ClientWrapper.h>
#include <ola/client/OlaClient.h>
#include <fstream>
#include <iostream>
#include <string>
#include <map>
#include <vector>

#include "examples/ShowPlayer.h"

using std::vector;
using std::string;
using std::map;
using ola::DmxBuffer;


ShowPlayer::ShowPlayer(const string &filename)
    : m_loader(filename),
      m_infinite_loop(false),
      m_iteration_remaining(0),
      m_loop_delay(0),
      m_start(0),
      m_stop(0) {
}

ShowPlayer::~ShowPlayer() {}

int ShowPlayer::Init() {
  if (!m_client.Setup()) {
    OLA_FATAL << "Client Setup failed";
    return ola::EXIT_UNAVAILABLE;
  }

  if (!m_loader.Load()) {
    return ola::EXIT_NOINPUT;
  }

  return ola::EXIT_OK;
}


int ShowPlayer::Playback(unsigned int iterations,
                         uint64_t duration,
                         uint64_t delay,
                         uint64_t start,
                         uint64_t stop) {
  m_infinite_loop = iterations == 0 || duration != 0;
  m_iteration_remaining = iterations;
  m_loop_delay = delay;
  m_start = start;
  m_stop = stop;

  ola::io::SelectServer *ss = m_client.GetSelectServer();
  if (duration != 0) {
    ss->RegisterSingleTimeout(
        duration * 1000,
        ola::NewSingleCallback(ss, &ola::io::SelectServer::Terminate));
  }

  if ((SeekTo(m_start) != ShowLoader::State::OK)) {
    return ola::EXIT_DATAERR;
  }
  ss->Run();
  return ola::EXIT_OK;
}


/**
 * Restart playback from start point
 */
void ShowPlayer::Loop() {
  ShowLoader::State state = SeekTo(m_start);
  if (state != ShowLoader::State::OK) {
    m_client.GetSelectServer()->Terminate();
  }
}


/**
 * Seek to @p seek_time in the show file
 * @param seek_time the time (in milliseconds) to seek to
 */
ShowLoader::State ShowPlayer::SeekTo(uint64_t seek_time) {
  // Seeking to a time before the playhead's position requires moving from the
  // beginning of the file.  This could be optimized more if this happens
  // frequently.
  if (seek_time <= m_playback_pos) {
    m_loader.Reset();
    m_playback_pos = 0;
  }

  // Keep reading through the show file until desired time is reached.
  map<unsigned int, ShowEntry> entries;
  uint64_t playhead_time = m_playback_pos;
  ShowLoader::State state;
  bool found = false;
  while (true) {
    ShowEntry entry;
    state = m_loader.NextEntry(&entry);
    if (state == ShowLoader::END_OF_FILE) {
      if (playhead_time == seek_time) {
        // Send the only frame(s) we have and loop
        OLA_WARN << "Starting at the end of the file; verify start time if "
                    "output looks wrong.";
        break;
      }
      OLA_FATAL << "Show file ends before the start time (actual length "
                << m_playback_pos << " ms)";
      return state;
    } else if (state == ShowLoader::INVALID_LINE) {
      HandleInvalidLine();
      return state;
    }
    playhead_time += entry.next_wait;
    if (entry.buffer.Size() > 0) {
      // TODO(Dan): Merge entry buffers to handle buffers with different lengths
      entries[entry.universe] = entry;
    }
    if (!found && playhead_time == seek_time) {
      // Gather frames from other universes before sending if landing on the
      // trailing edge of a frame's timeout.
      found = true;
    } else if ((found && entry.next_wait > 0) || playhead_time > seek_time) {
      break;
    }
  }
  m_playback_pos = playhead_time;

  // Send data in the state it would be in at the given time
  map<unsigned int, ShowEntry>::iterator entry_it;
  for (entry_it = entries.begin(); entry_it != entries.end(); ++entry_it) {
    SendFrame(entry_it->second);
  }
  // Adjust the timeout to handle landing in the middle of the entry's timeout
  RegisterNextTimeout(playhead_time-seek_time);

  return ShowLoader::OK;
}


/**
 * Send the next frame in the show file
 */
void ShowPlayer::SendNextFrame() {
  ShowEntry entry;
  ShowLoader::State state = m_loader.NextEntry(&entry);
  const bool stop_point_hit = m_stop > 0 && m_playback_pos >= m_stop;
  if (state == ShowLoader::END_OF_FILE || stop_point_hit) {
    if (m_playback_pos == m_stop) {
      // Send the last frame before looping/exiting
      SendFrame(entry);
    }
    HandleEndOfFile();
    return;
  } else if (state == ShowLoader::INVALID_LINE) {
    HandleInvalidLine();
    return;
  }
  SendEntry(entry);
}


/**
 * Send @p entry, update playhead position, and wait for next
 * @param entry the show file entry to send
 */
void ShowPlayer::SendEntry(const ShowEntry &entry) {
  // Send DMX data
  SendFrame(entry);

  // Set when next to send data
  m_playback_pos += entry.next_wait;
  RegisterNextTimeout(entry.next_wait);
}


/**
 * Send the next frame in @p timeout milliseconds
 */
void ShowPlayer::RegisterNextTimeout(const unsigned int timeout) {
  OLA_INFO << "Registering timeout for " << timeout << "ms";
  m_client.GetSelectServer()->RegisterSingleTimeout(
     timeout,
     ola::NewSingleCallback(this, &ShowPlayer::SendNextFrame));
}


/**
 * Send data contained in @p entry
 */
void ShowPlayer::SendFrame(const ShowEntry &entry) const {
  if (entry.buffer.Size() == 0) {
    return;
  }
  OLA_INFO << "Universe: " << entry.universe << ": " << entry.buffer.ToString();
  ola::client::SendDMXArgs args;
  m_client.GetClient()->SendDMX(entry.universe, entry.buffer, args);
}


/**
 * Handle the case where we reach the end of file
 */
void ShowPlayer::HandleEndOfFile() {
  if (m_iteration_remaining > 0) {
    m_iteration_remaining--;
  }

  const bool loop = m_infinite_loop || m_iteration_remaining > 0;
  uint64_t loop_delay = m_loop_delay;
  if (m_stop > m_playback_pos) {
    OLA_WARN << "Show file ends before the stop time (actual length "
             << m_playback_pos << " ms)";
    if (loop) {
      const uint64_t remaining_time = m_stop - m_playback_pos;
      OLA_WARN << "Waiting additional " << remaining_time << " ms before "
               << "looping.";
      loop_delay += remaining_time;
    }
  }

  if (loop) {
    OLA_INFO << "----- Waiting " << loop_delay << " ms before looping -----";
    // Move to start point and send the frame
    m_client.GetSelectServer()->RegisterSingleTimeout(
        loop_delay,
        ola::NewSingleCallback(this, &ShowPlayer::Loop));
    return;
  } else {
    // stop the show
    m_client.GetSelectServer()->Terminate();
  }
}


/**
 * Handle reading an invalid line from the show file
 */
void ShowPlayer::HandleInvalidLine() {
  OLA_FATAL << "Invalid data at line " << m_loader.GetCurrentLineNumber();
  m_client.GetSelectServer()->Terminate();
}
