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
//! [Client Fetch Plugins] NOLINT(whitespace/comments)
#include <ola/io/SelectServer.h>
#include <ola/Logging.h>
#include <ola/client/ClientWrapper.h>
#include <ola/Callback.h>

#include <vector>

using std::cout;
using std::endl;
using std::vector;

// Called when plugin information is available.
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
  ola::client::OlaClientWrapper wrapper;

  if (!wrapper.Setup()) {
    std::cerr << "Setup failed" << endl;
    exit(1);
  }

  // Fetch the list of plugins, a pointer to the SelectServer is passed to the
  // callback so we can use it to terminate the program.
  ola::io::SelectServer *ss = wrapper.GetSelectServer();
  wrapper.GetClient()->FetchPluginList(
      ola::NewSingleCallback(ShowPluginList, ss));

  // Start the main loop
  ss->Run();
}
//! [Client Fetch Plugins] NOLINT(whitespace/comments)
