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
 * slp-sa-test.cpp
 * Copyright (C) 2013 Simon Newton
 */

#include <stdio.h>
#include <getopt.h>
#include <signal.h>

#include <ola/Logging.h>
#include <ola/StringUtils.h>
#include <ola/base/Flags.h>
#include <ola/base/Init.h>
#include <ola/base/SysExits.h>
#include <ola/file/Util.h>
#include <ola/network/IPV4Address.h>

#include <iostream>
#include <string>
#include <vector>

#include "tools/e133/SLPSATestRunner.h"
#include "slp/ServerCommon.h"

using ola::file::FilenameFromPathOrPath;
using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;
using std::cout;
using std::endl;
using std::string;
using std::vector;

DEFINE_bool(list_tests, false, "List the test names.");
DEFINE_s_uint32(timeout, t, 1000, "Number of ms to wait for responses");
DEFINE_string(tests, "", "Restrict the tests that will be run");

/*
 * List the tests
 */
void DisplayTestsAndExit() {
  vector<string> test_names;
  GetTestnames(&test_names);
  vector<string>::const_iterator iter = test_names.begin();
  for (; iter != test_names.end(); ++iter) {
    cout << *iter << endl;
  }
  exit(0);
}


/*
 * Parse options, and run the tests.
 */
int main(int argc, char *argv[]) {
  std::ostringstream help_msg;
  help_msg <<
      "Stress test an SLP SA.\n"
      "\n"
      "Examples:\n"
      "   " << FilenameFromPathOrPath(argv[0]) << " 192.168.0.1\n"
      "   " << FilenameFromPathOrPath(argv[0]) << " 192.168.0.1:5568";

  ola::AppInit(&argc, argv, "[options] <ip>[:port]", help_msg.str());

  if (FLAGS_list_tests)
    DisplayTestsAndExit();

  if (argc != 2) {
    ola::DisplayUsage();
    exit(ola::EXIT_OK);
  }

  vector<string> tests_to_run;
  if (!FLAGS_tests.str().empty()) {
    ola::StringSplit(FLAGS_tests.str(), tests_to_run, ",");
  }

  IPV4SocketAddress target_endpoint;
  if (!IPV4SocketAddress::FromString(argv[1], &target_endpoint)) {
    // try just the IP
    IPV4Address target_ip;
    if (!IPV4Address::FromString(argv[1], &target_ip)) {
      OLA_WARN << "Invalid target : " << argv[1];
      return ola::EXIT_USAGE;
    }
    target_endpoint = IPV4SocketAddress(target_ip, ola::slp::DEFAULT_SLP_PORT);
  }

  TestRunner runner(FLAGS_timeout, tests_to_run, target_endpoint);
  runner.Run();
}
