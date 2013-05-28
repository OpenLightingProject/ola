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
 * slp-thread.cpp
 * Copyright (C) 2011 Simon Newton
 *
 */
#include <ola/Callback.h>
#include <ola/Logging.h>
#include <ola/base/Flags.h>
#include <ola/base/Init.h>
#include <ola/base/SysExits.h>
#include <ola/e133/SLPThread.h>
#include <ola/io/SelectServer.h>
#include <ola/slp/URLEntry.h>
#include <signal.h>

#include <iostream>
#include <memory>
#include <string>

using ola::slp::URLEntries;
using std::auto_ptr;
using std::string;

DEFINE_s_uint16(refresh, r, 60, "How often to check for new/expired services.");

// The SelectServer is global so we can access it from the signal handler.
ola::io::SelectServer ss;

/*
 * Called when we receive a new url list.
 */
void DiscoveryDone(bool ok, const URLEntries &urls) {
  if (!ok) {
    OLA_WARN << "SLP discovery failed";
  } else if (urls.empty()) {
    OLA_INFO << "No services found";
  } else {
    URLEntries::const_iterator iter;
    for (iter = urls.begin(); iter != urls.end(); ++iter) {
      std::cout << "  " << iter->url() << std::endl;
    }
  }
}

/*
 * Terminate on interrupt.
 */
static void InteruptSignal(int signo) {
  ss.Terminate();
  (void) signo;
}


/**
 * Main
 */
int main(int argc, char *argv[]) {
  ola::AppInit(argc, argv);
  ola::SetHelpString("[options]", "Locate E1.33 SLP services.");
  ola::ParseFlags(&argc, argv);
  ola::InitLoggingFromFlags();

  ola::InstallSignal(SIGINT, InteruptSignal);

  auto_ptr<ola::e133::BaseSLPThread> slp_thread(
    ola::e133::SLPThreadFactory::NewSLPThread(&ss));
  slp_thread->SetNewDeviceCallback(ola::NewCallback(&DiscoveryDone));

  if (!slp_thread->Init()) {
    OLA_WARN << "Init failed";
    exit(ola::EXIT_UNAVAILABLE);
  }

  if (!slp_thread->Start()) {
    OLA_WARN << "SLPThread Start() failed";
    exit(ola::EXIT_UNAVAILABLE);
  }

  ss.Run();
  slp_thread->Join(NULL);
}
