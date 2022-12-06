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
DEFINE_default_bool(verify_playback, true,
                    "Don't verify show file before playback");
DEFINE_s_string(universes, u, "",
                "A comma separated list of universes to record");
DEFINE_s_uint32(delay, d, 0, "The delay in ms between successive iterations.");
DEFINE_uint32(duration, 0, "Total playback time (seconds); the program will "
                           "close after this time has elapsed. This "
                           "option overrides the iteration option.");
// 0 means infinite looping
DEFINE_s_uint32(iterations, i, 1,
                "The number of times to repeat the show, 0 means unlimited. "
                "The duration option overrides this option.");
DEFINE_uint32(start, 0,
              "Time (milliseconds) in show file to start playback from.");
DEFINE_uint32(stop, 0,
              "Time (milliseconds) in show file to stop playback at. If "
              "the show file is shorter, the last look will be held until the "
              "stop point.");


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
 * @param[out] stream to receive a textual summary of the show
 */
int VerifyShow(const string &filename, std::ostream *summary) {
  ShowPlayer player(filename);
  int exit_status = player.Init(true);
  if (exit_status != ola::EXIT_OK) {
    // The init process will log any errors it encounters
    OLA_FATAL << "Error initializing the player. This is usually because of "
                 "incorrect command-line arguments or a system error, not "
                 "because of data. See any error messages above for details.";
    return exit_status;
  }
  // The playback simulation process will log any errors it encounters
  exit_status = player.Playback(FLAGS_iterations,
                                FLAGS_duration,
                                FLAGS_delay,
                                FLAGS_start,
                                FLAGS_stop);

  if (exit_status == ola::EXIT_OK) {
    if (summary != NULL) {
      map<unsigned int, uint64_t> frames_by_universe = player.GetFrameCount();
      const uint64_t total_time = player.GetRunTime();
      map<unsigned int, uint64_t>::const_iterator iter;
      uint64_t total = 0;
      *summary << "------------ Summary ----------" << endl;
      if (FLAGS_start > 0) {
        *summary << "Starting at " << FLAGS_start / 1000.0 << " second(s) from "
                 << "the start of the recording" << endl;
      }
      if (FLAGS_stop > 0) {
        *summary << "Stopping at " << FLAGS_stop / 1000.0 << " second(s) from "
                 << "the start of the recording" << endl;
      }
      if (FLAGS_delay > 0) {
        *summary << "Waiting " << FLAGS_delay / 1000.0 << " before looping"
                 << endl;
      }
      if (FLAGS_iterations == 0 && FLAGS_duration == 0) {
        // Infinite loop
        *summary << "For each iteration:" << endl;
      } else {
        if (FLAGS_iterations > 0) {
          // Defined iterations
          *summary << "For all (" << FLAGS_iterations << ") iterations:"
                   << endl;
        }
        if (FLAGS_duration > 0) {
          // Defined duration
          *summary << "After playing for " << FLAGS_duration << " second(s) "
                   << "total:" << endl;
        }
      }
      for (iter = frames_by_universe.begin(); iter != frames_by_universe.end();
           ++iter) {
        const unsigned int univ_frames = iter->second;
        *summary << "Universe " << iter->first << ": " << univ_frames
                 << " frames" << endl;
        total += univ_frames;
      }
      *summary << endl
               << "Total frames: " << total << endl
               << "Total playback time: " << total_time / 1000.0 << " seconds"
               << endl;
    }
    return exit_status;
  } else {
    // The ShowPlayer will have printed details about the problem(s), so direct
    // the user's attention there.
    OLA_FATAL << "Error loading show. See error message above for details.";
    return exit_status;
  }
}


/**
 * Playback a recorded show
 */
int PlaybackShow() {
  const string filename = FLAGS_playback.str();
  if (FLAGS_verify_playback) {
    // Verify the show and print a summary before running
    std::ostringstream summary;
    const int verified = VerifyShow(filename, &summary);
    OLA_INFO << "Verification of " << filename << ":" << endl << summary.str();
    if (verified != ola::EXIT_OK) {
      // Show did not pass verification
      // The Verify method has already informed the user, so fail with that
      // method's result.
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
    const int verified = VerifyShow(FLAGS_verify.str(), &cout);
    return verified;
  } else {
    OLA_FATAL << "One of --record or --playback or --verify must be provided";
    ola::DisplayUsage();
  }
  return ola::EXIT_OK;
}
