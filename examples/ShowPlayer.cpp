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
      m_stop(0),
      m_playback_pos(0),
      m_run_time(0),
      m_simulate(false),
      m_next_task(TASK_LOOP),
      m_status(ola::EXIT_SOFTWARE) {
}

ShowPlayer::~ShowPlayer() {}

int ShowPlayer::Init(const bool simulate) {
  m_simulate = simulate;
  if (!simulate && !m_client.Setup()) {
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
  m_status = ola::EXIT_SOFTWARE;

  if (!m_simulate) {
    ola::io::SelectServer *ss = m_client.GetSelectServer();
    if (duration != 0) {
      ss->RegisterSingleTimeout(
          duration * 1000,
          ola::NewSingleCallback(ss, &ola::io::SelectServer::Terminate));
    }
    if ((SeekTo(m_start) != ShowLoader::OK)) {
      return ola::EXIT_DATAERR;
    }
    ss->Run();
  } else {
    // Never infinite loop when simulating
    if (iterations == 0 && duration == 0) {
      m_infinite_loop = false;
      m_iteration_remaining = 1;
    }
    // Simple event loop to simulate playback without involving olad
    // Start by seeking to start point
    m_next_task = TASK_LOOP;
    while (m_next_task != TASK_COMPLETE) {
      if (duration > 0 && m_run_time >= duration * 1000) {
        m_run_time = duration * 1000;
        break;
      }
      switch (m_next_task) {
        case TASK_COMPLETE:
          break;
        case TASK_NEXT_FRAME:
          SendNextFrame();
          break;
        case TASK_LOOP:
          Loop();
          break;
      }
    }
  }

  if (m_iteration_remaining > 0) {
    OLA_WARN << "More iterations requested than can fit in the duration "
                "(" << m_iteration_remaining << " iteration(s) remain)";
  }

  return m_status;
}


/**
 * Restart playback from start point
 */
void ShowPlayer::Loop() {
  ShowLoader::State state = SeekTo(m_start);

  switch (state) {
    // Success conditions
    case ShowLoader::END_OF_FILE:
    case ShowLoader::OK:
      m_status = ola::EXIT_OK;
      break;
    // All other states are considered errors of some sort
    case ShowLoader::INVALID_LINE:
      StopPlayback(ola::EXIT_DATAERR);
      break;
    default:
      // Handle future errors
      StopPlayback(ola::EXIT_SOFTWARE);
      break;
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
  // Seeking to the current position can result in the frame being skipped;
  // ensure the frame is loaded in this case as well.
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

    // Handle abnormal conditions
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
    } else if (state != ShowLoader::OK) {
      // Handle future errors
      OLA_FATAL << "An unknown error occurred near " << m_playback_pos << " ms";
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

  if (state == ShowLoader::OK) {
    m_status = ola::EXIT_OK;
    SendEntry(entry);
  } else if (state == ShowLoader::END_OF_FILE ||
             (m_stop > 0 && m_playback_pos >= m_stop)) {
    // At EOF or at user-requested stopping point
    if (m_stop == 0 || m_playback_pos == m_stop) {
      // Send the last frame before looping/exiting
      SendFrame(entry);
    }
    HandleEndOfShow();
    return;
  } else if (state == ShowLoader::INVALID_LINE) {
    HandleInvalidLine();
    return;
  } else {
    // Handle future errors
    OLA_FATAL << "An unknown error occurred near " << m_playback_pos << " ms";
    StopPlayback(ola::EXIT_SOFTWARE);
    return;
  }
}


/**
 * Send @p entry, update playhead position, and wait for next
 * @param entry the show file entry to send
 */
void ShowPlayer::SendEntry(const ShowEntry &entry) {
  // Send DMX data
  SendFrame(entry);
  m_playback_pos += entry.next_wait;

  // Set when next to send data
  RegisterNextTimeout(entry.next_wait);
}


/**
 * Send the next frame in @p timeout milliseconds
 */
void ShowPlayer::RegisterNextTimeout(const unsigned int timeout) {
  m_run_time += timeout;
  m_next_task = TASK_NEXT_FRAME;
  if (!m_simulate) {
    OLA_DEBUG << "Registering timeout for " << timeout << "ms";
    m_client.GetSelectServer()->RegisterSingleTimeout(
        timeout,
        ola::NewSingleCallback(this, &ShowPlayer::SendNextFrame));
  }
}


/**
 * Send data contained in @p entry
 */
void ShowPlayer::SendFrame(const ShowEntry &entry) {
  if (entry.buffer.Size() == 0) {
    return;
  }
  if (!m_simulate) {
    OLA_DEBUG << "Universe: " << entry.universe << ": "
              << entry.buffer.ToString();
    ola::client::SendDMXArgs args;
    m_client.GetClient()->SendDMX(entry.universe, entry.buffer, args);
  }
  m_frame_count[entry.universe]++;
}


/**
 * Handle the case where we reach the end of file
 */
void ShowPlayer::HandleEndOfShow() {
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
    // Move to start point and send the frame
    m_run_time += loop_delay;
    m_next_task = TASK_LOOP;
    if (!m_simulate) {
      OLA_INFO << "----- "
               << m_iteration_remaining << " iteration(s) remain "
               << "-----";
      OLA_INFO << "----- Waiting " << loop_delay << " ms before looping -----";
      m_client.GetSelectServer()->RegisterSingleTimeout(
          loop_delay,
          ola::NewSingleCallback(this, &ShowPlayer::Loop));
    }
    return;
  } else {
    StopPlayback(ola::EXIT_OK);
  }
}


/**
 * Handle reading an invalid line from the show file
 */
void ShowPlayer::HandleInvalidLine() {
  OLA_FATAL << "Invalid data at line " << m_loader.GetCurrentLineNumber();
  StopPlayback(ola::EXIT_DATAERR);
}


/**
 * Stop playback
 * @param exit_status program exit status; ola::EXIT_OK iff no errors
 */
void ShowPlayer::StopPlayback(const int exit_status) {
  m_next_task = TASK_COMPLETE;
  m_status = exit_status;
  if (!m_simulate) {
    m_client.GetSelectServer()->Terminate();
  }
}
