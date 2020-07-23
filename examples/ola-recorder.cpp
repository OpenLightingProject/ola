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
 * ola-recorder.cpp
 * A simple tool to record & playback shows.
 * Copyright (C) 2011 Simon Newton
 */

#include <ola/Callback.h>
#include <ola/DmxBuffer.h>
#include <ola/Logging.h>
#include <ola/StringUtils.h>
#include <ola/base/Flags.h>
#include <ola/base/Init.h>
#include <ola/base/SysExits.h>
#include <signal.h>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "examples/ShowPlayer.h"
#include "examples/ShowLoader.h"
#include "examples/ShowRecorder.h"

// On MinGW, SignalThread.h pulls in pthread.h which pulls in Windows.h, which
// needs to be after WinSock2.h, hence this order
#include <ola/thread/SignalThread.h>  // NOLINT(build/include_order)

using std::auto_ptr;
using std::cout;
using std::endl;
using std::map;
using std::vector;
using std::string;

DEFINE_s_string(playback, p, "", "The show file to playback.");
DEFINE_s_string(record, r, "", "The show file to record data to.");
DEFINE_string(verify, "", "The show file to verify.");
DEFINE_default_bool(no_verify, false, "Don't verify show file before playback");
DEFINE_s_string(universes, u, "",
                "A comma separated list of universes to record");
DEFINE_s_uint32(delay, d, 0, "The delay in ms between successive iterations.");
DEFINE_uint32(duration, 0, "The length of time (seconds) to run for.");
// 0 means infinite looping
DEFINE_s_uint32(iterations, i, 1,
                "The number of times to repeat the show, 0 means unlimited.");
DEFINE_uint32(start, 0,
              "Time (milliseconds) in show file to start playback from.");
DEFINE_uint32(stop, 0,
              "Time (milliseconds) in show file to stop playback at. If "
              "the show file is shorter, this option is ignored.");


void TerminateRecorder(ShowRecorder *recorder) {
  recorder->Stop();
}


/**
 * Record a show
 */
int RecordShow() {
  if (FLAGS_universes.str().empty()) {
    OLA_FATAL << "No universes specified, use -u";
    exit(ola::EXIT_USAGE);
  }

  vector<string> universe_strs;
  vector<unsigned int> universes;
  ola::StringSplit(FLAGS_universes.str(), &universe_strs, ",");
  vector<string>::const_iterator iter = universe_strs.begin();
  for (; iter != universe_strs.end(); ++iter) {
    unsigned int universe;
    if (!ola::StringToInt(*iter, &universe)) {
      OLA_FATAL << *iter << " isn't a valid universe number";
      exit(ola::EXIT_USAGE);
    }
    universes.push_back(universe);
  }

  ShowRecorder show_recorder(FLAGS_record.str(), universes);
  int status = show_recorder.Init();
  if (status)
    return status;

  {
    ola::thread::SignalThread signal_thread;
    cout << "Recording, hit Control-C to end" << endl;
    signal_thread.InstallSignalHandler(
        SIGINT, ola::NewCallback(TerminateRecorder, &show_recorder));
    signal_thread.InstallSignalHandler(
        SIGTERM, ola::NewCallback(TerminateRecorder, &show_recorder));
    if (!signal_thread.Start()) {
      show_recorder.Stop();
    }
    show_recorder.Record();
  }
  cout << "Saved " << show_recorder.FrameCount() << " frames" << endl;
  return ola::EXIT_OK;
}


/**
 * Verify a show file is valid
 * @param[in] filename file to check
 * @param[out] string to fill with a summary of the show
 */
int VerifyShow(const string &filename, string *summary) {
//  ShowLoader loader(filename);
//  if (!loader.Load())
//    return ola::EXIT_NOINPUT;
//
//  map<unsigned int, unsigned int> frames_by_universe;
//
//  ShowEntry entry;
//  ShowLoader::State state;
//  uint64_t playback_pos = 0;
//  bool playing = false;
//  while (true) {
//    state = loader.NextEntry(&entry);
//    if (state != ShowLoader::OK) {
//      // If this is a problem, an explanation will be printed later following
//      // the summary.
//      break;
//    }
//    playback_pos += entry.next_wait;
//    if (playing) {
//      frames_by_universe[entry.universe]++;
//    } else {
//      // Clamp the frame count to 1 as we haven't actually started playing yet
//      frames_by_universe[entry.universe] = 1;
//    }
//    if (FLAGS_stop > 0 && playback_pos >= FLAGS_stop) {
//      // Compensate for overshooting the stop time
//      playback_pos = FLAGS_stop;
//      break;
//    }
//    if (!playing && playback_pos > FLAGS_start) {
//      // Found the start point
//      playing = true;
//    }
//  }
//  if (FLAGS_start > playback_pos) {
//    OLA_WARN << "Show file ends before the start time (actual length "
//             << playback_pos << " ms)";
//  }
//  if (FLAGS_stop > playback_pos) {
//    OLA_WARN << "Show file ends before the stop time (actual length "
//             << playback_pos << " ms)";
//  }
  ShowPlayer player(filename);
  int state = player.Init(true);
  if (state != ola::EXIT_OK) {
    return state;
  }
  state = player.Playback(FLAGS_iterations,
                          FLAGS_duration,
                          FLAGS_delay,
                          FLAGS_start,
                          FLAGS_stop);

  if (state == ola::EXIT_OK && summary != NULL) {
//    uint64_t total_time;
//    if (playback_pos >= FLAGS_start) {
//      // Frames will have been sent
//      total_time = playback_pos - FLAGS_start;
//      if (FLAGS_iterations > 0) {
//        total_time *= FLAGS_iterations;
//      }
//      // Duration will cause playback to stop before it reaches the end if
//      // duration is shorter than the playback time.  Likewise, playback will
//      // stop if it reaches the end before the desired duration because a
//      // number of iterations has been specified.
//      if (FLAGS_duration > 0 && FLAGS_duration < total_time) {
//        total_time = FLAGS_duration * 1000;
//      }
//    } else {
//      // Will do precisely nothing for FLAGS_duration seconds
//      total_time = FLAGS_duration * 1000;
//    }

    map<unsigned int, uint64_t> frames_by_universe = player.GetFrameCount();
    const uint64_t total_time = player.GetRunTime();
    std::stringstream out;
    map<unsigned int, uint64_t>::const_iterator iter;
    uint64_t total = 0;
    out << "------------ Summary ----------" << endl;
    if (FLAGS_start > 0) {
      out << "Starting at: " << FLAGS_start / 1000.0 << " second(s)" << endl;
    }
    if (FLAGS_stop > 0) {
      out << "Stopping at: " << FLAGS_stop / 1000.0 << " second(s)" << endl;
    }
    for (iter = frames_by_universe.begin(); iter != frames_by_universe.end();
         ++iter) {
      const unsigned int univ_frames = iter->second;
//      if (FLAGS_iterations > 0) {
//        univ_frames *= FLAGS_iterations;
//      }
      out << "Universe " << iter->first << ": " << univ_frames << " frames"
          << endl;
      total += univ_frames;
    }
    out << endl;
    out << "Total frames: " << total << endl;
    out << "Playback time: " << total_time / 1000.0 << " second(s)" << endl;

    summary->assign(out.str());
  }
  if ((state == ShowLoader::OK) || (state == ShowLoader::END_OF_FILE)) {
    return ola::EXIT_OK;
  } else {
    OLA_FATAL << "Error loading show, got state " << state;
    return ola::EXIT_DATAERR;
  }
}


/**
 * Playback a recorded show
 */
int PlaybackShow() {
  const string filename = FLAGS_playback.str();
  if (!FLAGS_no_verify) {
    // Verify the show and print a summary before running
    string summary;
    const int verified = VerifyShow(filename, &summary);
    // Printing a newline first makes this output look better in interactive
    // terminal logs.
    OLA_INFO << endl << summary;
    if (verified != ola::EXIT_OK) {
      // Show did not pass verification
      return verified;
    }
  }

  // Begin playback
  ShowPlayer player(filename);
  int status = player.Init();
  if (status == ola::EXIT_OK) {
    status = player.Playback(FLAGS_iterations,
                             FLAGS_duration,
                             FLAGS_delay,
                             FLAGS_start,
                             FLAGS_stop);
  }
  return status;
}


/*
 * Main
 */
int main(int argc, char *argv[]) {
  ola::AppInit(&argc, argv,
               "[--record <file> --universes <universe_list>] [--playback "
               "<file>] [--verify <file>]",
               "Record a series of universes, or playback a previously "
               "recorded show.");

  if (FLAGS_stop > 0 && FLAGS_stop < FLAGS_start) {
    OLA_FATAL << "Stop time must be later than start time.";
    return ola::EXIT_USAGE;
  }

  if (!FLAGS_playback.str().empty()) {
    return PlaybackShow();
  } else if (!FLAGS_record.str().empty()) {
    return RecordShow();
  } else if (!FLAGS_verify.str().empty()) {
    string summary;
    const int verified = VerifyShow(FLAGS_verify.str(), &summary);
    cout << summary << endl;
    return verified;
  } else {
    OLA_FATAL << "One of --record or --playback or --verify must be provided";
    ola::DisplayUsage();
  }
  return ola::EXIT_OK;
}
