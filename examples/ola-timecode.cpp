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
 * ola-timecode.cpp
 * Send timecode data with OLA
 * Copyright (C) 2011 Simon Newton
 */

#include <errno.h>
#include <stdlib.h>
#include <ola/Logging.h>
#include <ola/StringUtils.h>
#include <ola/base/Flags.h>
#include <ola/base/Init.h>
#include <ola/base/SysExits.h>
#include <ola/client/ClientWrapper.h>
#include <ola/client/OlaClient.h>
#include <ola/timecode/TimeCode.h>
#include <ola/timecode/TimeCodeEnums.h>

#include <iostream>
#include <string>
#include <vector>

using ola::client::OlaClientWrapper;
using ola::StringToInt;
using ola::io::SelectServer;
using ola::timecode::TimeCode;
using std::cerr;
using std::endl;
using std::string;
using std::vector;

DEFINE_s_string(format, f, "SMPTE", "One of FILM, EBU, DF, SMPTE (default).");

/**
 * Called on when we return from sending timecode data.
 */
void TimeCodeDone(ola::io::SelectServer *ss,
                  const ola::client::Result &result) {
  if (!result.Success()) {
    OLA_WARN << result.Error();
  }
  ss->Terminate();
}

/*
 * Main
 */
int main(int argc, char *argv[]) {
  ola::AppInit(
      &argc,
      argv,
      "[options] <time_code>",
      "Send TimeCode data to OLA. time_code is in the form: \n"
          "Hours:Minutes:Seconds:Frames");
  ola::client::OlaClientWrapper ola_client;

  if (argc != 2)
    ola::DisplayUsageAndExit();

  ola::timecode::TimeCodeType time_code_type = ola::timecode::TIMECODE_SMPTE;
  if (!FLAGS_format.str().empty()) {
    string type = FLAGS_format;
    ola::ToLower(&type);
    if (type == "film") {
      time_code_type = ola::timecode::TIMECODE_FILM;
    } else if (type == "ebu") {
      time_code_type = ola::timecode::TIMECODE_EBU;
    } else if (type == "df") {
      time_code_type = ola::timecode::TIMECODE_DF;
    } else if (type == "smpte") {
      time_code_type = ola::timecode::TIMECODE_SMPTE;
    } else {
      cerr << "Invalid TimeCode format " << type << endl;
      exit(ola::EXIT_USAGE);
    }
  }

  vector<string> tokens;
  ola::StringSplit(argv[1], &tokens, ":");
  if (tokens.size() != 4) {
    cerr << "Invalid TimeCode value " << argv[1] << endl;
    exit(ola::EXIT_USAGE);
  }

  uint8_t hours, minutes, seconds, frames;
  if (!StringToInt(tokens[0], &hours, true)) {
    cerr << "Invalid TimeCode hours " << tokens[0] << endl;
    exit(ola::EXIT_USAGE);
  }
  if (!StringToInt(tokens[1], &minutes, true)) {
    cerr << "Invalid TimeCode minutes " << tokens[1] << endl;
    exit(ola::EXIT_USAGE);
  }
  if (!StringToInt(tokens[2], &seconds, true)) {
    cerr << "Invalid TimeCode seconds " << tokens[2] << endl;
    exit(ola::EXIT_USAGE);
  }
  if (!StringToInt(tokens[3], &frames, true)) {
    cerr << "Invalid TimeCode frames " << tokens[3] << endl;
    exit(ola::EXIT_USAGE);
  }

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
      timecode,
      ola::NewSingleCallback(&TimeCodeDone, ola_client.GetSelectServer()));

  ola_client.GetSelectServer()->Run();
  return ola::EXIT_OK;
}
