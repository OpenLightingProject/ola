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
 * slp-register.cpp
 * Copyright (C) 2011 Simon Newton
 *
 */

#include <ola/Callback.h>
#include <ola/Logging.h>
#include <ola/network/SelectServer.h>
#include <pthread.h>
#include <signal.h>

#include <string>

#include "SlpThread.h"

using std::string;

static const char URL[] = "127.0.0.1:707a:00000002";

// stupid globals for now
SlpThread *thread;
ola::network::SelectServer ss;

void RegisterCallback(bool ok) {
  OLA_INFO << "in register callback, thread " << pthread_self() <<
    ", state is " << ok;
}


void DeRegisterCallback(ola::network::SelectServer *ss,
                        bool ok) {
  OLA_INFO << "in deregister callback, thread " << pthread_self() <<
    ", state is " << ok;

  ss->Terminate();
}


/*
 * Deregister on interrupt.
 */
static void sig_interupt(int signo) {
  signo = 0;
  ss.Terminate();
}


int main(int argc, char *argv[]) {
  ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);

  // signal handler
  struct sigaction act, oact;

  act.sa_handler = sig_interupt;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;

  if (sigaction(SIGINT, &act, &oact) < 0) {
    OLA_WARN << "Failed to install signal SIGINT";
    return false;
  }

  thread = new SlpThread(&ss);
  if (!thread->Init()) {
    OLA_WARN << "Init failed";
    delete thread;
    return 1;
  }

  OLA_INFO << "in main thread " << pthread_self();
  thread->Start();
  thread->Register(ola::NewSingleCallback(&RegisterCallback), URL, 1000);

  ss.Run();

  // start the de-registration process
  thread->DeRegister(ola::NewSingleCallback(&DeRegisterCallback, &ss),
                     URL);
  ss.Restart();
  ss.Run();

  thread->Join(NULL);
  delete thread;
  (void) argc;
  (void) argv;
}
