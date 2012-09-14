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
 *  ola-recorder.cpp
 *  A simple tool to record & playback shows.
 *  Copyright (C) 2011 Simon Newton
 */

#include <errno.h>
#include <getopt.h>
#include <string.h>
#include <sysexits.h>
#include <ola/DmxBuffer.h>
#include <ola/Logging.h>
#include <ola/StringUtils.h>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "examples/ShowPlayer.h"
#include "examples/ShowLoader.h"
#include "examples/ShowRecorder.h"

using std::cout;
using std::endl;
using std::map;
using std::vector;
using std::string;
using ola::OlaUniverse;
using ola::SimpleClient;

typedef struct {
  ola::log_level level;
  bool help;       // show the help
  string cmd;      // argv[0]
  bool playback;
  bool record;
  bool verify;
  // 0 means infinite looping
  unsigned int loop_iterations;
  unsigned int loop_delay;
  string file;
  string universes;
} options;


/*
 * Parse our cmd line options.
 */
int ParseOptions(int argc, char *argv[], options *opts) {
  static struct option long_options[] = {
      {"help", no_argument, 0, 'h'},
      {"delay", required_argument, 0, 'd'},
      {"iterations", required_argument, 0, 'i'},
      {"log-level", required_argument, 0, 'l'},
      {"playback", required_argument, 0, 'p'},
      {"record", required_argument, 0, 'r'},
      {"universes", required_argument, 0, 'u'},
      {"verify", required_argument, 0, 'v'},
      {0, 0, 0, 0}
  };

  opts->help = false;
  opts->level = ola::OLA_LOG_INFO;
  opts->playback = false;
  opts->loop_iterations = 1;
  opts->loop_delay = 0;
  opts->record = false;
  opts->verify = false;

  int c, ll;
  int option_index = 0;

  while (1) {
    c = getopt_long(argc,
                    argv,
                    "d:hi:l:p:r:u:v:",
                    long_options,
                    &option_index);

    if (c == -1)
      break;

    switch (c) {
      case 0:
        break;
      case 'd':
        opts->loop_delay = atoi(optarg);
        break;
      case 'h':
        opts->help = true;
        break;
      case 'i':
        opts->loop_iterations = atoi(optarg);
        break;
      case 'l':
        ll = atoi(optarg);
        switch (ll) {
          case 0:
            // nothing is written at this level
            // so this turns logging off
            opts->level = ola::OLA_LOG_NONE;
            break;
          case 1:
            opts->level = ola::OLA_LOG_FATAL;
            break;
          case 2:
            opts->level = ola::OLA_LOG_WARN;
            break;
          case 3:
            opts->level = ola::OLA_LOG_INFO;
            break;
          case 4:
            opts->level = ola::OLA_LOG_DEBUG;
            break;
          default :
            break;
        }
        break;
      case 'p':
        opts->file = optarg;
        opts->playback = true;
        break;
      case 'r':
        opts->file = optarg;
        opts->record = true;
        break;
      case 'u':
        opts->universes = optarg;
        break;
      case 'v':
        opts->file = optarg;
        opts->verify = true;
        break;
      case '?':
        break;
      default:
        break;
    }
  }
  return 0;
}


/*
 * Display the help message
 */
void DisplayHelpAndExit(const options &opts) {
  cout << "Usage: " << opts.cmd <<
  " --universes <universe_list> [--record <file>] [--playback <file>]\n"
  "\n"
  "Record a series of universes, or playback a previously recorded show.\n"
  "\n"
  "  -h, --help              Display this help message and exit.\n"
  "  -l, --log-level <level> Set the logging level 0 .. 4 .\n"
  "  -r, --record <file>     Path to a file to record data\n"
  "  -p, --playback <file>   Path to a file from which to playback data\n"
  "  -u, --universes <list>  Comma separated list of universes to record.\n"
  "  -v, --verify <file>     Path to a file to verify\n"
  "\n"
  "Additional Playback Options:\n"
  "  -d, --delay             Delay in ms between successive iterations.\n"
  "  -i, --iterations        Number of times to repeat the show, 0: unlimited."
  "\n"
  << endl;
  exit(0);
}


/**
 * Record a show
 */
int RecordShow(const options &opts) {
  if (opts.universes.empty()) {
    OLA_FATAL << "No universes specified, use -u";
    exit(EX_USAGE);
  }

  vector<string> universe_strs;
  vector<unsigned int> universes;
  ola::StringSplit(opts.universes, universe_strs, ",");
  vector<string>::const_iterator iter = universe_strs.begin();
  for (; iter != universe_strs.end(); ++iter) {
    unsigned int universe;
    if (!ola::StringToInt(*iter, &universe)) {
      OLA_FATAL << *iter << " isn't a valid universe number";
      exit(EX_USAGE);
    }
    universes.push_back(universe);
  }

  ShowRecorder recorder(opts.file, universes);
  int status = recorder.Init();
  if (status)
    return status;

  recorder.Record();
  return EX_OK;
}


/**
 * Verify a show file is valid
 */
int VerifyShow(const string &filename) {
  ShowLoader loader(filename);
  if (!loader.Load())
    return EX_NOINPUT;

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

  return EX_OK;
}


/*
 * Main
 */
int main(int argc, char *argv[]) {
  options opts;

  ParseOptions(argc, argv, &opts);
  ola::InitLogging(opts.level, ola::OLA_LOG_STDERR);

  if (opts.help)
    DisplayHelpAndExit(opts);

  int check = (opts.playback ? 1 : 0) +
              (opts.record ? 1 : 0) +
              (opts.verify ? 1 : 0);

  if (check == 0) {
    DisplayHelpAndExit(opts);
  }

  if (check != 1) {
    OLA_FATAL << "Only one of --record or --playback must be provided";
    exit(EX_USAGE);
  } else if (opts.record) {
    return RecordShow(opts);
  } else if (opts.playback) {
    ShowPlayer player(opts.file);
    int status = player.Init();
    if (!status)
      status = player.Playback(opts.loop_iterations,
                               opts.loop_delay);
    return status;
  } else if (opts.verify) {
    return VerifyShow(opts.file);
  }
  return EX_OK;
}
