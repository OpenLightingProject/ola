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
 *  ola-uni-stats.cpp
 *  Display some simple universe stats.
 *  Copyright (C) 2012 Simon Newton
 */

#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdlib.h>
#include <sysexits.h>

#include <ola/BaseTypes.h>
#include <ola/Clock.h>
#include <ola/DmxBuffer.h>
#include <ola/Logging.h>
#include <ola/OlaCallbackClient.h>
#include <ola/OlaClientWrapper.h>
#include <ola/StringUtils.h>
#include <ola/base/Init.h>
#include <ola/io/StdinHandler.h>

#include <iostream>
#include <iomanip>
#include <map>
#include <string>
#include <vector>

using ola::DmxBuffer;
using ola::OlaCallbackClientWrapper;
using ola::StringToInt;
using ola::TimeInterval;
using ola::TimeStamp;
using ola::io::SelectServer;
using std::cerr;
using std::cout;
using std::endl;
using std::setw;
using std::string;
using std::vector;


class UniverseTracker: private ola::io::StdinHandler {
  public:
    UniverseTracker(OlaCallbackClientWrapper *wrapper,
                    const vector<unsigned int> &universes);
    ~UniverseTracker() {}

    bool Run();
    void Stop() { m_wrapper->GetSelectServer()->Terminate(); }
    void PrintStats();
    void ResetStats();

  protected:
    void HandleCharacter(char c);

  private:
    struct UniverseStats {
      public:
        uint16_t shortest_frame;
        uint16_t longest_frame;
        uint64_t frame_count;
        uint64_t frame_changes;
        DmxBuffer frame_data;

        UniverseStats() { Reset(); }

        void Reset() {
          shortest_frame = DMX_UNIVERSE_SIZE + 1,
          longest_frame = 0;
          frame_count = 0;
          frame_changes = 0;
        }
    };

    typedef std::map<unsigned int, UniverseStats> UniverseStatsMap;
    UniverseStatsMap m_stats;
    ola::TimeStamp m_start_time;
    OlaCallbackClientWrapper *m_wrapper;
    ola::Clock m_clock;

    void UniverseData(unsigned int universe, const DmxBuffer &dmx,
                      const string &error);
    void RegisterComplete(const string &error);
};


UniverseTracker::UniverseTracker(OlaCallbackClientWrapper *wrapper,
                                 const vector<unsigned int> &universes)
    : StdinHandler(wrapper->GetSelectServer()),
      m_wrapper(wrapper) {
  // set callback
  m_wrapper->GetClient()->SetDmxCallback(
      ola::NewCallback(this, &UniverseTracker::UniverseData));

  vector<unsigned int>::const_iterator iter = universes.begin();
  for (; iter != universes.end(); ++iter) {
    struct UniverseStats stats;
    m_stats[*iter] = stats;

    // register here
    m_wrapper->GetClient()->RegisterUniverse(
      *iter,
      ola::REGISTER,
      ola::NewSingleCallback(this, &UniverseTracker::RegisterComplete));
  }
}


bool UniverseTracker::Run() {
  m_clock.CurrentTime(&m_start_time);
  m_wrapper->GetSelectServer()->Run();
  return true;
}


void UniverseTracker::PrintStats() {
  UniverseStatsMap::iterator iter = m_stats.begin();
  TimeStamp now;
  m_clock.CurrentTime(&now);

  TimeInterval interval = now - m_start_time;
  OLA_INFO << "Time delta was " << interval;

  for (; iter != m_stats.end(); ++iter) {
    const UniverseStats stats = iter->second;

    float fps = 0.0;
    if (interval.Seconds() > 0)
      fps = static_cast<float>(stats.frame_count) / interval.Seconds();

    cout << "Universe " << iter->first << endl;
    cout << "  Frames Received: " << stats.frame_count <<
      ", Frames/sec: " << fps << endl;
    cout << "  Frame changes: " << stats.frame_changes << endl;
    cout << "  Smallest Frame: ";
    if (stats.shortest_frame == DMX_UNIVERSE_SIZE + 1)
      cout << "N/A";
    else
      cout << stats.shortest_frame;
    cout << ", Largest Frame: ";
    if (stats.longest_frame == 0)
      cout << "N/A";
    else
      cout << stats.longest_frame;
    cout << endl;
    cout << "------------------------------" << endl;
  }
}


void UniverseTracker::ResetStats() {
  m_clock.CurrentTime(&m_start_time);

  UniverseStatsMap::iterator iter = m_stats.begin();
  for (; iter != m_stats.end(); ++iter) {
    iter->second.Reset();
  }
  cout << "Reset counters" << endl;
}

void UniverseTracker::HandleCharacter(char c) {
  switch (c) {
    case 'q':
      m_wrapper->GetSelectServer()->Terminate();
      break;
    case 'p':
      PrintStats();
      break;
    case 'r':
      ResetStats();
      break;
    default:
      break;
  }
}


void UniverseTracker::UniverseData(unsigned int universe, const DmxBuffer &dmx,
                                   const string &error) {
  if (!error.empty()) {
    OLA_WARN << error;
    return;
  }

  UniverseStatsMap::iterator iter = m_stats.find(universe);
  if (iter == m_stats.end()) {
    OLA_WARN << "Received data for unknown universe " << universe;
    return;
  }

  if (dmx.Size() < iter->second.shortest_frame)
    iter->second.shortest_frame = dmx.Size();

  if (dmx.Size() > iter->second.longest_frame)
    iter->second.longest_frame = dmx.Size();

  iter->second.frame_count++;

  DmxBuffer &last_dmx = iter->second.frame_data;
  if (last_dmx.Size() == 0) {
    // this is the first frame
    last_dmx.Set(dmx);
  } else {
    if (last_dmx.Size() != dmx.Size() || last_dmx != dmx) {
      // the frame changed
      iter->second.frame_changes++;
      last_dmx.Set(dmx);
    }
  }
}

void UniverseTracker::RegisterComplete(const string &error) {
  if (!error.empty())
    OLA_WARN << "Register command failed with " << errno;
}


typedef struct {
  bool help;
  ola::log_level log_level;
  vector<string> args;
} TrackerOptions;


/*
 * parse our cmd line options
 */
void ParseOptions(int argc, char *argv[], TrackerOptions *options) {
  static struct option long_options[] = {
      {"help", no_argument, 0, 'h'},
      {"log-level", required_argument, 0, 'l'},
      {0, 0, 0, 0}
    };

  int c;
  int option_index = 0;

  while (1) {
    c = getopt_long(argc, argv, "hl:", long_options, &option_index);

    if (c == -1)
      break;

    switch (c) {
      case 0:
        break;
      case 'h':
        options->help = true;
        break;
      case 'l':
        switch (atoi(optarg)) {
          case 0:
            // nothing is written at this level
            // so this turns logging off
            options->log_level = ola::OLA_LOG_NONE;
            break;
          case 1:
            options->log_level = ola::OLA_LOG_FATAL;
            break;
          case 2:
            options->log_level = ola::OLA_LOG_WARN;
            break;
          case 3:
            options->log_level = ola::OLA_LOG_INFO;
            break;
          case 4:
            options->log_level = ola::OLA_LOG_DEBUG;
            break;
          default :
            break;
        }
        break;
      case '?':
        break;
      default:
        break;
    }
  }

  int index = optind;
  for (; index < argc; index++)
    options->args.push_back(argv[index]);
}


/*
 * Display the help message
 */
void DisplayHelpAndExit(char arg[]) {
  std::cout << "Usage: " << arg << " [options] <universe1> <universe2> ...\n"
  "\n"
  "Watch one or more universes and produce stats on DMX frame rates.\n"
  "\n"
  "  -h, --help               Display this help message and exit.\n"
  "  -l, --log-level <level>  Set the logging level 0 .. 4.\n"
  << std::endl;
  exit(EX_USAGE);
}

SelectServer *ss = NULL;

static void InteruptSignal(int unused) {
  if (ss)
    ss->Terminate();
  (void) unused;
}


/*
 * Main
 */
int main(int argc, char *argv[]) {
  TrackerOptions options;
  options.help = false;
  options.log_level = ola::OLA_LOG_WARN;
  ParseOptions(argc, argv, &options);

  if (options.help)
    DisplayHelpAndExit(argv[0]);

  if (options.args.empty())
    DisplayHelpAndExit(argv[0]);

  vector<unsigned int> universes;
  vector<string>::const_iterator iter = options.args.begin();
  for (; iter != options.args.end(); ++iter) {
    unsigned int universe;
    if (!StringToInt(*iter, &universe, true)) {
      cerr << "Invalid Universe " << *iter << endl;
      exit(EX_USAGE);
    }
    universes.push_back(universe);
  }

  ola::InitLogging(options.log_level, ola::OLA_LOG_STDERR);

  ola::OlaCallbackClientWrapper ola_client;
  if (!ola_client.Setup()) {
    OLA_FATAL << "Setup failed";
    exit(EX_UNAVAILABLE);
  }
  ss = ola_client.GetSelectServer();

  UniverseTracker tracker(&ola_client, universes);
  ola::InstallSignal(SIGINT, InteruptSignal);
  cout << "Actions:" << endl;
  cout << "  p - Print stats" << endl;
  cout << "  q - Quit" << endl;
  cout << "  r - Reset stats" << endl;
  bool r = tracker.Run();
  if (r)
    tracker.PrintStats();
  return r;
}
