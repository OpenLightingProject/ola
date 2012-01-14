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
 * ola-trigger.cpp
 * Copyright (C) 2011 Simon Newton
 */

#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sysexits.h>

#include <ola/BaseTypes.h>
#include <ola/Callback.h>
#include <ola/DmxBuffer.h>
#include <ola/Logging.h>
#include <ola/OlaCallbackClient.h>
#include <ola/OlaClientWrapper.h>
#include <ola/network/SelectServer.h>

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
using std::vector;

// prototype of bison-generated parser function
int yyparse();

// globals modified by the config parser
Context *global_context;
SlotActionMap global_slots;

// The SelectServer to kill when we catch SIGINT
ola::network::SelectServer *ss = NULL;

// The options struct
typedef struct {
  bool help;
  ola::log_level log_level;
  unsigned int universe;
  unsigned int offset;
  vector<string> args;  // extra args
} options;


typedef vector<Slot*> SlotList;


/*
 * Parse our command line options
 */
void ParseOptions(int argc, char *argv[], options *opts) {
  static struct option long_options[] = {
      {"help", no_argument, 0, 'h'},
      {"log-level", required_argument, 0, 'l'},
      {"offset", required_argument, 0, 'o'},
      {"universe", required_argument, 0, 'u'},
      {0, 0, 0, 0}
    };

  int option_index = 0;

  while (1) {
    int c = getopt_long(argc, argv, "hl:o:u:", long_options, &option_index);

    if (c == -1)
      break;

    switch (c) {
      case 'h':
        opts->help = true;
        break;
      case 'l':
        switch (atoi(optarg)) {
          case 0:
            // nothing is written at this level
            // so this turns logging off
            opts->log_level = ola::OLA_LOG_NONE;
            break;
          case 1:
            opts->log_level = ola::OLA_LOG_FATAL;
            break;
          case 2:
            opts->log_level = ola::OLA_LOG_WARN;
            break;
          case 3:
            opts->log_level = ola::OLA_LOG_INFO;
            break;
          case 4:
            opts->log_level = ola::OLA_LOG_DEBUG;
            break;
          default :
            break;
        }
        break;
      case 'o':
        opts->offset = atoi(optarg);
        break;
      case 'u':
        opts->universe = atoi(optarg);
        break;
      case '?':
        break;
      default:
       break;
    }
  }

  int index = optind;
  for (; index < argc; index++)
    opts->args.push_back(argv[index]);
  return;
}


/*
 * Display the help message
 */
void DisplayHelpAndExit(char *argv[]) {
  std::cout << "Usage: " << argv[0] << " [options] <config_file>\n"
  "\n"
  "Run programs based on the values in a DMX stream.\n"
  "\n"
  "  -h, --help                 Display this help message and exit.\n"
  "  -l, --log-level <level>    Set the logging level 0 .. 4.\n"
  "  -o, --offset <slot_offset> Apply an offset to the slot numbers. Valid\n"
  "                             offsets are 0 to 512, default is 0.\n"
  "  -u, --universe <universe>  The universe to use, defaults to 1"
  << std::endl;
  exit(0);
}


/*
 * Catch SIGCHLD.
 */
static void CatchSIGCHLD(int signo) {
  pid_t pid;
  do {
    pid = waitpid(-1, NULL, WNOHANG);
  } while (pid > 0);
  (void) signo;
}


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
  return true;
}


/**
 * The DMX Handler, this calls the trigger if the universes match.
 */
void NewDmx(unsigned int our_universe,
            DMXTrigger *trigger,
            unsigned int universe,
            const DmxBuffer &data,
            const std::string &error) {
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
    if (slots->SlotOffset() + offset >= DMX_UNIVERSE_SIZE) {
      OLA_FATAL << "Slot " << slots->SlotOffset() << " + offset " <<
        offset << " is greater than " << DMX_UNIVERSE_SIZE - 1;
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
  options opts;
  opts.log_level = ola::OLA_LOG_INFO;
  opts.universe = 1;
  opts.offset = 0;
  opts.help = false;
  ParseOptions(argc, argv, &opts);

  if (opts.help)
    DisplayHelpAndExit(argv);

  if (opts.offset >= DMX_UNIVERSE_SIZE) {
    std::cerr << "Invalid slot offset: " << opts.offset << std::endl;
    DisplayHelpAndExit(argv);
  }

  if (opts.args.size() != 1)
    DisplayHelpAndExit(argv);

  ola::InitLogging(opts.log_level, ola::OLA_LOG_STDERR);

  // setup the defaut context
  global_context = new Context();
  OLA_INFO << "Loading config from " << opts.args[0];

  // open the config file
  if (freopen(opts.args[0].c_str(), "r", stdin) == NULL) {
    OLA_FATAL << "File " << opts.args[0] << " cannot be opened.\n";
    exit(EX_DATAERR);
  }

  yyparse();

  // if we got to this stage the config is ok, setup the client
  ola::OlaCallbackClientWrapper wrapper;

  if (!wrapper.Setup())
    exit(EX_UNAVAILABLE);

  ss = wrapper.GetSelectServer();

  if (!InstallSignals())
    exit(EX_OSERR);

  // create the vector of Slot
  SlotList slots;
  if (ApplyOffset(opts.offset, &slots)) {
    // setup the trigger
    DMXTrigger trigger(global_context, slots);

    // register for DMX
    ola::OlaCallbackClient *client = wrapper.GetClient();
    client->SetDmxCallback(
        ola::NewCallback(&NewDmx, opts.universe, &trigger));
    client->RegisterUniverse(opts.universe, ola::REGISTER, NULL);

    // start the client
    wrapper.GetSelectServer()->Run();
  }

  // cleanup
  FreeSlot(&slots);
}
