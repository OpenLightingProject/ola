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
DEFINE_s_string(universes, u, "",
                "A comma separated list of universes to record");
DEFINE_s_uint32(delay, d, 0, "The delay in ms between successive iterations.");
DEFINE_uint32(duration, 0, "The length of time (seconds) to run for.");
// 0 means infinite looping
DEFINE_s_uint32(iterations, i, 1,
                "The number of times to repeat the show, 0 means unlimited.");

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
 */
int VerifyShow(const string &filename) {
  ShowLoader loader(filename);
  if (!loader.Load())
    return ola::EXIT_NOINPUT;

  map<unsigned int, unsigned int> frames_by_universe;
  uint64_t total_time = 0;

  unsigned int universe;
  ola::DmxBuffer buffer;
  unsigned int timeout;
  ShowLoader::State state;
  while (true) {
    state = loader.NextFrame(&universe, &buffer);
    if (state != ShowLoader::OK)
      break;
    frames_by_universe[universe]++;

    state = loader.NextTimeout(&timeout);
    if (state != ShowLoader::OK)
      break;
    total_time += timeout;
  }

  map<unsigned int, unsigned int>::const_iterator iter;
  unsigned int total = 0;
  cout << "------------ Summary ----------" << endl;
  for (iter = frames_by_universe.begin(); iter != frames_by_universe.end();
       ++iter) {
    cout << "Universe " << iter->first << ": " << iter->second << " frames" <<
      endl;
    total += iter->second;
  }
  cout << "Total frames: " << total << endl;
  cout << "Playback time: " << total_time / 1000 << "." << total_time % 10 <<
    " seconds" << endl;

  if ((state == ShowLoader::OK) || (state == ShowLoader::END_OF_FILE)) {
    return ola::EXIT_OK;
  } else {
    OLA_FATAL << "Error loading show, got state " << state;
    return ola::EXIT_DATAERR;
  }
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

  if (!FLAGS_playback.str().empty()) {
    ShowPlayer player(FLAGS_playback.str());
    int status = player.Init();
    if (!status)
      status = player.Playback(FLAGS_iterations, FLAGS_duration, FLAGS_delay);
    return status;
  } else if (!FLAGS_record.str().empty()) {
    return RecordShow();
  } else if (!FLAGS_verify.str().empty()) {
    return VerifyShow(FLAGS_verify.str());
  } else {
    OLA_FATAL << "One of --record or --playback or --verify must be provided";
    ola::DisplayUsage();
  }
  return ola::EXIT_OK;
}
