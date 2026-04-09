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
 * ola-latency.cpp
 * Call FetchDmx and track the latency for each call.
 * Copyright (C) 2005 Simon Newton
 */

#include <stdlib.h>
#include <ola/Callback.h>
#include <ola/Clock.h>
#include <ola/DmxBuffer.h>
#include <ola/Logging.h>
#include <ola/OlaClientWrapper.h>
#include <ola/base/Flags.h>
#include <ola/base/Init.h>
#include <ola/thread/SignalThread.h>

#include <iostream>
#include <string>

using ola::DmxBuffer;
using ola::NewSingleCallback;
using ola::OlaCallbackClientWrapper;
using ola::TimeStamp;
using ola::TimeInterval;
using std::cout;
using std::endl;
using std::string;

DEFINE_s_uint32(universe, u, 1, "The universe to receive data for");
DEFINE_default_bool(send_dmx, false, "Use SendDmx messages, default is GetDmx");
DEFINE_s_uint32(count, c, 0,
    "Exit after this many RPCs, default: infinite (0)");

class Tracker {
 public:
    Tracker()
        : m_count(0),
          m_sum(0) {
      m_buffer.Blackout();
    }

    bool Setup();
    void Start();

    void GotDmx(const DmxBuffer &data, const string &error);
    void SendComplete(const string &error);

 private:
    uint32_t m_count;
    uint64_t m_sum;
    TimeInterval m_max;
    ola::DmxBuffer m_buffer;
    OlaCallbackClientWrapper m_wrapper;
    ola::Clock m_clock;
    ola::thread::SignalThread m_signal_thread;
    TimeStamp m_send_time;

    void SendRequest();
    void LogTime();
    void StartSignalThread();
};

bool Tracker::Setup() {
  return m_wrapper.Setup();
}

void Tracker::Start() {
  ola::io::SelectServer *ss = m_wrapper.GetSelectServer();
  m_signal_thread.InstallSignalHandler(
      SIGINT,
      ola::NewCallback(ss, &ola::io::SelectServer::Terminate));
  m_signal_thread.InstallSignalHandler(
      SIGTERM,
      ola::NewCallback(ss, &ola::io::SelectServer::Terminate));
  SendRequest();

  ss->Execute(ola::NewSingleCallback(this, &Tracker::StartSignalThread));
  ss->Run();

  // Print this via cout to ensure we actually get some output by default
  // It also means you can just see the stats and not each individual request
  // if you want.
  cout << "--------------" << endl;
  cout << "Sent " << m_count << " RPCs" << endl;
  cout << "Max was " << m_max.MicroSeconds() << " microseconds" << endl;
  cout << "Mean " << m_sum / m_count << " microseconds" << endl;
}

void Tracker::GotDmx(const DmxBuffer &, const string &) {
  LogTime();
}

void Tracker::SendComplete(const string &) {
  LogTime();
}

void Tracker::SendRequest() {
  m_clock.CurrentMonotonicTime(&m_send_time);
  if (FLAGS_send_dmx) {
    m_wrapper.GetClient()->SendDmx(
        FLAGS_universe,
        m_buffer,
        NewSingleCallback(this, &Tracker::SendComplete));

  } else {
    m_wrapper.GetClient()->FetchDmx(
        FLAGS_universe,
        NewSingleCallback(this, &Tracker::GotDmx));
  }
}

void Tracker::LogTime() {
  TimeStamp now;
  m_clock.CurrentMonotonicTime(&now);
  TimeInterval delta = now - m_send_time;
  if (delta > m_max) {
    m_max = delta;
  }
  m_sum += delta.MicroSeconds();

  OLA_INFO << "RPC took " << delta;
  if (FLAGS_count == ++m_count) {
    m_wrapper.GetSelectServer()->Terminate();
  } else {
    SendRequest();
  }
}

void Tracker::StartSignalThread() {
  if (!m_signal_thread.Start()) {
    m_wrapper.GetSelectServer()->Terminate();
  }
}

int main(int argc, char *argv[]) {
  ola::AppInit(&argc, argv, "[options]",
               "Measure the latency of RPCs to olad.");

  Tracker tracker;
  if (!tracker.Setup()) {
    OLA_FATAL << "Setup failed";
    exit(1);
  }

  tracker.Start();
  return 0;
}
