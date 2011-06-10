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
 * slp-thread.cpp
 * Copyright (C) 2011 Simon Newton
 *
 */

#include <ola/Callback.h>
#include <ola/Logging.h>
#include <ola/network/SelectServer.h>
#include <pthread.h>

#include <string>
#include <vector>

#include "SlpThread.h"

using std::string;
using std::vector;

unsigned int counter = 0;

void DiscoveryDone(ola::network::SelectServer *ss,
                   bool ok,
                   std::vector<std::string> *urls) {
  OLA_INFO << "in discovery callback, thread " << pthread_self();
  OLA_INFO << "state is " << ok;
  OLA_INFO << "size is " << urls->size();

  vector<string>::const_iterator iter;
  for (iter = urls->begin(); iter != urls->end(); ++iter) {
    OLA_INFO << "  " << *iter;
  }

  counter++;
  if (counter == 2)
    ss->Terminate();
}


int main(int argc, char *argv[]) {
  ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);

  ola::network::SelectServer ss;

  SlpThread thread(&ss);

  if (!thread.Init()) {
    OLA_WARN << "Init failed";
    return 1;
  }


  OLA_INFO << "in main thread " << pthread_self();
  thread.Start();
  std::vector<std::string> urls;
  std::vector<std::string> urls2;
  thread.Discover(ola::NewSingleCallback(&DiscoveryDone, &ss),
                  &urls);
  thread.Discover(ola::NewSingleCallback(&DiscoveryDone, &ss),
                  &urls2);

  ss.Run();
  thread.Join(NULL);

  (void) argc;
  (void) argv;
}
