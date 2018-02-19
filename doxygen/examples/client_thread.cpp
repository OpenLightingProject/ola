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
 * Copyright (C) 2015 Simon Newton
 */
//! [Client Thread] NOLINT(whitespace/comments)
#include <ola/io/SelectServer.h>
#include <ola/Logging.h>
#include <ola/client/ClientWrapper.h>
#include <ola/thread/Thread.h>
#include <ola/Callback.h>

#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <ola/win/CleanWindows.h>
#endif  // _WIN32

using ola::io::SelectServer;
using ola::NewSingleCallback;

using std::cout;
using std::endl;
using std::vector;

class OlaThread : public ola::thread::Thread {
 public:
  bool Start() {
    if (!m_wrapper.Setup()) {
      return false;
    }
    return ola::thread::Thread::Start();
  }

  void Stop() {
    m_wrapper.GetSelectServer()->Terminate();
  }

  void FetchPluginList(ola::client::PluginListCallback *callback) {
    m_wrapper.GetSelectServer()->Execute(
        NewSingleCallback(this, &OlaThread::InternalFetchPluginList, callback));
  }

  ola::io::SelectServer* GetSelectServer() {
    return m_wrapper.GetSelectServer();
  }

 protected:
  void *Run() {
    m_wrapper.GetSelectServer()->Run();
    return NULL;
  }

 private:
  ola::client::OlaClientWrapper m_wrapper;

  void InternalFetchPluginList(ola::client::PluginListCallback *callback) {
    m_wrapper.GetClient()->FetchPluginList(callback);
  }
};



// Called when plugin information is available.
// This function is run from the OLA Thread, if you use variables in the main
// program then you'll need to add locking.
void ShowPluginList(ola::io::SelectServer *ss,
                    const ola::client::Result &result,
                    const vector<ola::client::OlaPlugin> &plugins) {
  if (!result.Success()) {
    std::cerr << result.Error() << endl;
  } else {
    vector<ola::client::OlaPlugin>::const_iterator iter = plugins.begin();
    for (; iter != plugins.end(); ++iter) {
      cout << "Plugin: " << iter->Name() << endl;
    }
  }
  ss->Terminate();  // terminate the program.
}

int main(int, char *[]) {
  ola::InitLogging(ola::OLA_LOG_WARN, ola::OLA_LOG_STDERR);

  OlaThread ola_thread;
  if (!ola_thread.Start()) {
    std::cerr << "Failed to start OLA thread" << endl;
    exit(1);
  }

  // Control is returned to the main program here.

  // To fetch a list of plugins
  ola::io::SelectServer *ss = ola_thread.GetSelectServer();
  ola_thread.FetchPluginList(
      ola::NewSingleCallback(ShowPluginList, ss));

  // The main program continues...
#ifdef _WIN32
  Sleep(1000);
#else
  sleep(1);
#endif  // _WIN32

  // When it's time to exit, Stop the OLA thread.
  ola_thread.Stop();
  ola_thread.Join();
}
//! [Client Thread] NOLINT(whitespace/comments)
