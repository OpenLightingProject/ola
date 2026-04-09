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
 * ola-uni-stats.cpp
 * Display some simple universe stats.
 * Copyright (C) 2012 Simon Newton
 */

#include <errno.h>
#include <signal.h>
#include <stdlib.h>

#include <ola/Clock.h>
#include <ola/Constants.h>
#include <ola/DmxBuffer.h>
#include <ola/Logging.h>
#include <ola/OlaCallbackClient.h>
#include <ola/OlaClientWrapper.h>
#include <ola/StringUtils.h>
#include <ola/base/Flags.h>
#include <ola/base/Init.h>
#include <ola/base/SysExits.h>
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


class UniverseTracker {
 public:
    UniverseTracker(OlaCallbackClientWrapper *wrapper,
                    const vector<unsigned int> &universes);
    ~UniverseTracker() {}

    bool Run();
    void Stop() { m_wrapper->GetSelectServer()->Terminate(); }
    void PrintStats();
    void ResetStats();

 protected:
    void Input(int c);

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
        shortest_frame = ola::DMX_UNIVERSE_SIZE + 1,
        longest_frame = 0;
        frame_count = 0;
        frame_changes = 0;
      }
    };

    typedef std::map<unsigned int, UniverseStats> UniverseStatsMap;
    UniverseStatsMap m_stats;
    ola::TimeStamp m_start_time;
    OlaCallbackClientWrapper *m_wrapper;
    ola::io::StdinHandler m_stdin_handler;
    ola::Clock m_clock;

    void UniverseData(unsigned int universe, const DmxBuffer &dmx,
                      const string &error);
    void RegisterComplete(const string &error);
};


UniverseTracker::UniverseTracker(OlaCallbackClientWrapper *wrapper,
                                 const vector<unsigned int> &universes)
    : m_wrapper(wrapper),
      m_stdin_handler(wrapper->GetSelectServer(),
                      ola::NewCallback(this, &UniverseTracker::Input)) {
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
  m_clock.CurrentMonotonicTime(&m_start_time);
  m_wrapper->GetSelectServer()->Run();
  return true;
}


void UniverseTracker::PrintStats() {
  UniverseStatsMap::iterator iter = m_stats.begin();
  TimeStamp now;
  m_clock.CurrentMonotonicTime(&now);

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
    if (stats.shortest_frame == ola::DMX_UNIVERSE_SIZE + 1)
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
  m_clock.CurrentMonotonicTime(&m_start_time);

  UniverseStatsMap::iterator iter = m_stats.begin();
  for (; iter != m_stats.end(); ++iter) {
    iter->second.Reset();
  }
  cout << "Reset counters" << endl;
}

void UniverseTracker::Input(int c) {
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


SelectServer *ss = NULL;

static void InteruptSignal(OLA_UNUSED int signo) {
  int old_errno = errno;
  if (ss) {
    ss->Terminate();
  }
  errno = old_errno;
}


/*
 * Main
 */
int main(int argc, char *argv[]) {
  ola::AppInit(
      &argc,
      argv,
      "[options] <universe1> <universe2> ...",
      "Watch one or more universes and produce stats on DMX frame rates.");

  vector<unsigned int> universes;
  for (int i = 1; i < argc; i++) {
    unsigned int universe;
    if (!StringToInt(argv[i], &universe, true)) {
      cerr << "Invalid Universe " << argv[i] << endl;
      exit(ola::EXIT_USAGE);
    }
    universes.push_back(universe);
  }

  if (universes.size() <= 0) {
    ola::DisplayUsageAndExit();
  }

  ola::OlaCallbackClientWrapper ola_client;
  if (!ola_client.Setup()) {
    OLA_FATAL << "Setup failed";
    exit(ola::EXIT_UNAVAILABLE);
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
