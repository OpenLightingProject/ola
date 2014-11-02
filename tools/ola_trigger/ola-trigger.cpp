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
 * ola-trigger.cpp
 * Copyright (C) 2011 Simon Newton
 */

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#ifndef _WIN32
#include <sys/wait.h>
#endif

#include <ola/base/Flags.h>
#include <ola/base/Init.h>
#include <ola/base/SysExits.h>
#include <ola/Callback.h>
#include <ola/Constants.h>
#include <ola/DmxBuffer.h>
#include <ola/io/SelectServer.h>
#include <ola/Logging.h>
#include <ola/OlaCallbackClient.h>
#include <ola/OlaClientWrapper.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "tools/ola_trigger/Action.h"
#include "tools/ola_trigger/Context.h"
#include "tools/ola_trigger/DMXTrigger.h"
#include "tools/ola_trigger/ParserGlobals.h"

using ola::DmxBuffer;
using std::map;
using std::string;
using std::vector;

DEFINE_s_uint16(offset, o, 0,
                "Apply an offset to the slot numbers. Valid offsets are 0 to "
                "512, default is 0.");
DEFINE_s_uint32(universe, u, 0, "The universe to use, defaults to 0.");
DEFINE_default_bool(validate, false,
                    "Validate the config file, rather than running it.");

// prototype of bison-generated parser function
int yyparse();

// globals modified by the config parser
Context *global_context;
SlotActionMap global_slots;

// The SelectServer to kill when we catch SIGINT
ola::io::SelectServer *ss = NULL;

typedef vector<Slot*> SlotList;

/*
 * Catch SIGCHLD.
 */
#ifndef _WIN32
static void CatchSIGCHLD(int signo) {
  pid_t pid;
  do {
    pid = waitpid(-1, NULL, WNOHANG);
  } while (pid > 0);
  (void) signo;
}
#endif


/*
 * Terminate cleanly on interrupt
 */
static void CatchSIGINT(int signo) {
  // there is a race condition here if you send the signal before we call Run()
  // it's not a huge deal though.
  if (ss)
    ss->Terminate();
  (void) signo;
}


/*
 * Install the SIGCHLD handler.
 */
bool InstallSignals() {
#ifdef WIN32
  // There's no SIGCHILD on Windows
  if (signal(SIGINT, CatchSIGINT) == reinterpret_cast<void(*)(int)>(EINVAL)) {
    OLA_WARN << "Failed to install signal SIGINT";
    return false;
  }
  if (signal(SIGTERM, CatchSIGINT) == reinterpret_cast<void(*)(int)>(EINVAL)) {
    OLA_WARN << "Failed to install signal SIGTERM";
    return false;
  }
#else
  struct sigaction act, oact;

  act.sa_handler = CatchSIGCHLD;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;

  if (sigaction(SIGCHLD, &act, &oact) < 0) {
    OLA_WARN << "Failed to install signal SIGCHLD";
    return false;
  }

  act.sa_handler = CatchSIGINT;
  if (sigaction(SIGINT, &act, &oact) < 0) {
    OLA_WARN << "Failed to install signal SIGINT";
    return false;
  }
  if (sigaction(SIGTERM, &act, &oact) < 0) {
    OLA_WARN << "Failed to install signal SIGTERM";
    return false;
  }
#endif
  return true;
}


/**
 * The DMX Handler, this calls the trigger if the universes match.
 */
void NewDmx(unsigned int our_universe,
            DMXTrigger *trigger,
            unsigned int universe,
            const DmxBuffer &data,
            const string &error) {
  if (universe == our_universe) {
    if (error.empty())
      trigger->NewDMX(data);
  }
  (void) error;
}


/**
 * Delete all the slot actions in the vector.
 */
void FreeSlot(SlotList *slots) {
  SlotList::iterator action_iter = slots->begin();
  for (; action_iter != slots->end(); ++action_iter)
    delete *action_iter;
  slots->clear();
}


/**
 * Build a vector of Slot from the global_slots map with the
 * offset applied.
 *
 * The clears the global_slots map.
 * @returns true if the offset was applied correctly, false otherwise.
 */
bool ApplyOffset(uint16_t offset, SlotList *all_slots) {
  bool ok = true;
  all_slots->reserve(global_slots.size());
  SlotActionMap::const_iterator iter = global_slots.begin();
  for (; iter != global_slots.end(); ++iter) {
    Slot *slots = iter->second;
    if (slots->SlotOffset() + offset >= ola::DMX_UNIVERSE_SIZE) {
      OLA_FATAL << "Slot " << slots->SlotOffset() << " + offset " <<
        offset << " is greater than " << ola::DMX_UNIVERSE_SIZE - 1;
      ok = false;
      break;
    }
    slots->SetSlotOffset(slots->SlotOffset() + offset);
    all_slots->push_back(slots);
  }

  if (!ok) {
    all_slots->clear();
    for (iter = global_slots.begin(); iter != global_slots.end();
         ++iter)
      delete iter->second;
  }

  global_slots.clear();
  return ok;
}


/*
 * Main
 */
int main(int argc, char *argv[]) {
  ola::AppInit(&argc,
               argv,
               "[options] <config_file>",
               "Run programs based on the values in a DMX stream.");

  if (FLAGS_offset >= ola::DMX_UNIVERSE_SIZE) {
    std::cerr << "Invalid slot offset: " << FLAGS_offset << std::endl;
    exit(ola::EXIT_USAGE);
  }

  if (argc != 2)
    ola::DisplayUsageAndExit();

  // setup the default context
  global_context = new Context();
  OLA_INFO << "Loading config from " << argv[1];

  // open the config file
  if (freopen(argv[1], "r", stdin) == NULL) {
    OLA_FATAL << "File " << argv[1] << " cannot be opened.\n";
    exit(ola::EXIT_DATAERR);
  }

  yyparse();

  if (FLAGS_validate) {
    std::cout << "File " << argv[1] << " is valid." << std::endl;
    // TODO(Peter): Print some stats here, validate the offset if supplied
    exit(ola::EXIT_OK);
  }

  // if we got to this stage the config is ok and we want to run it, setup the
  // client
  ola::OlaCallbackClientWrapper wrapper;

  if (!wrapper.Setup())
    exit(ola::EXIT_UNAVAILABLE);

  ss = wrapper.GetSelectServer();

  if (!InstallSignals())
    exit(ola::EXIT_OSERR);

  // create the vector of Slot
  SlotList slots;
  if (ApplyOffset(FLAGS_offset, &slots)) {
    // setup the trigger
    DMXTrigger trigger(global_context, slots);

    // register for DMX
    ola::OlaCallbackClient *client = wrapper.GetClient();
    client->SetDmxCallback(
        ola::NewCallback(&NewDmx,
                         static_cast<unsigned int>(FLAGS_universe),
                         &trigger));
    client->RegisterUniverse(FLAGS_universe, ola::REGISTER, NULL);

    // start the client
    wrapper.GetSelectServer()->Run();
  }

  // cleanup
  FreeSlot(&slots);
}
