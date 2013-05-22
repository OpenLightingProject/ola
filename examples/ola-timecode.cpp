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
 *  ola-timecode.cpp
 *  Send timecode data with OLA
 *  Copyright (C) 2011 Simon Newton
 */

#include <errno.h>
#include <getopt.h>
#include <stdlib.h>
#include <ola/Logging.h>
#include <ola/OlaCallbackClient.h>
#include <ola/OlaClientWrapper.h>
#include <ola/StringUtils.h>
#include <ola/base/SysExits.h>
#include <ola/timecode/TimeCode.h>
#include <ola/timecode/TimeCodeEnums.h>

#include <iostream>
#include <string>
#include <vector>

using ola::OlaCallbackClientWrapper;
using ola::StringToInt;
using ola::io::SelectServer;
using ola::timecode::TimeCode;
using std::string;
using std::vector;


typedef struct {
  bool help;
  std::vector<string> args;  // extra args
  string format;  // timecode format
} options;


/*
 * parse our cmd line options
 */
void ParseOptions(int argc, char *argv[], options *opts) {
  static struct option long_options[] = {
      {"help", no_argument, 0, 'h'},
      {"format", required_argument, 0, 'f'},
      {0, 0, 0, 0}
    };

  opts->help = false;

  int c;
  int option_index = 0;

  while (1) {
    c = getopt_long(argc, argv, "f:h", long_options, &option_index);

    if (c == -1)
      break;

    switch (c) {
      case 0:
        break;
      case 'h':
        opts->help = true;
        break;
      case 'f':
        opts->format = optarg;
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
}


/*
 * Display the help message
 */
void DisplayHelpAndExit(char arg[]) {
  std::cout << "Usage: " << arg << " <time_code>\n"
  "\n"
  "Send TimeCode data to OLA. time_code is in the form: \n"
  "Hours:Minutes:Seconds:Frames\n"
  "\n"
  "  -h, --help     Display this help message and exit.\n"
  "  -f, --format   One of FILM, EBU, DF, SMPTE (default).\n"
  << std::endl;
  exit(ola::EXIT_USAGE);
}


/**
 * Called on when we return from sending timecode data.
 */
void TimeCodeDone(ola::io::SelectServer *ss,
                  const string &status) {
  OLA_WARN << status;
  ss->Terminate();
}

/*
 * Main
 */
int main(int argc, char *argv[]) {
  ola::InitLogging(ola::OLA_LOG_WARN, ola::OLA_LOG_STDERR);
  ola::OlaCallbackClientWrapper ola_client;
  options opts;

  ParseOptions(argc, argv, &opts);

  if (opts.help)
    DisplayHelpAndExit(argv[0]);

  if (opts.args.size() != 1)
    DisplayHelpAndExit(argv[0]);

  ola::timecode::TimeCodeType time_code_type = ola::timecode::TIMECODE_SMPTE;
  if (!opts.format.empty()) {
    string type = opts.format;
    ola::ToLower(&type);
    if (type == "film")
      time_code_type = ola::timecode::TIMECODE_FILM;
    else if (type == "ebu")
      time_code_type = ola::timecode::TIMECODE_EBU;
    else if (type == "df")
      time_code_type = ola::timecode::TIMECODE_DF;
    else if (type == "smpte")
      time_code_type = ola::timecode::TIMECODE_SMPTE;
    else
      DisplayHelpAndExit(argv[0]);
  }

  vector<string> tokens;
  ola::StringSplit(opts.args[0], tokens, ":");
  if (tokens.size() != 4)
    DisplayHelpAndExit(argv[0]);

  uint8_t hours, minutes, seconds, frames;
  if (!StringToInt(tokens[0], &hours, true))
    DisplayHelpAndExit(argv[0]);
  if (!StringToInt(tokens[1], &minutes, true))
    DisplayHelpAndExit(argv[0]);
  if (!StringToInt(tokens[2], &seconds, true))
    DisplayHelpAndExit(argv[0]);
  if (!StringToInt(tokens[3], &frames, true))
    DisplayHelpAndExit(argv[0]);

  TimeCode timecode(time_code_type, hours, minutes, seconds, frames);
  if (!timecode.IsValid()) {
    OLA_FATAL << "Invalid TimeCode value";
    exit(ola::EXIT_USAGE);
  }

  if (!ola_client.Setup()) {
    OLA_FATAL << "Setup failed";
    exit(ola::EXIT_UNAVAILABLE);
  }

  ola_client.GetClient()->SendTimeCode(
      ola::NewSingleCallback(&TimeCodeDone, ola_client.GetSelectServer()),
      timecode);

  ola_client.GetSelectServer()->Run();
  return ola::EXIT_OK;
}
