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
 * slp-sa-test.cpp
 * Copyright (C) 2013 Simon Newton
 */

#include <stdio.h>
#include <getopt.h>
#include <signal.h>

#include <ola/Logging.h>
#include <ola/StringUtils.h>
#include <ola/base/Init.h>
#include <ola/base/SysExits.h>
#include <ola/network/IPV4Address.h>

#include <iostream>
#include <string>
#include <vector>

#include "tools/e133/SLPSATestRunner.h"
#include "slp/ServerCommon.h"

using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;
using std::cout;
using std::endl;
using std::string;
using std::vector;


struct Options {
 public:
    bool help;
    bool list_tests;
    uint32_t timeout;
    ola::log_level log_level;
    string test_names;
    vector<string> extra_args;

    Options()
        : help(false),
          list_tests(false),
          timeout(1000),
          log_level(ola::OLA_LOG_INFO) {
    }
};


/*
 * Parse our command line options
 */
void ParseOptions(int argc, char *argv[],
                  Options *options) {
  int list_tests = 0;

  enum {
    TEST_NAMES_OPTION = 256,
  };

  static struct option long_options[] = {
      {"help", no_argument, 0, 'h'},
      {"log-level", required_argument, 0, 'l'},
      {"timeout", required_argument, 0, 't'},
      {"list-tests", no_argument, &list_tests, 1},
      {"tests", required_argument, 0, TEST_NAMES_OPTION},
      {0, 0, 0, 0}
  };

  int option_index = 0;

  while (1) {
    int c = getopt_long(argc, argv, "hl:t:", long_options, &option_index);

    if (c == -1)
      break;

    switch (c) {
      case 'h':
        options->help = true;
        break;
      case 'l':
        switch (atoi(optarg)) {
          case 0:
            // nothing is written at this level
            // so this turns logging off
            options->log_level = ola::OLA_LOG_NONE;
            break;
          case 1:
            options->log_level = ola::OLA_LOG_FATAL;
            break;
          case 2:
            options->log_level = ola::OLA_LOG_WARN;
            break;
          case 3:
            options->log_level = ola::OLA_LOG_INFO;
            break;
          case 4:
            options->log_level = ola::OLA_LOG_DEBUG;
            break;
          default :
            break;
        }
        break;
      case 't':
        options->timeout = atoi(optarg);
        break;
      case TEST_NAMES_OPTION:
        options->test_names = optarg;
        break;
      case '?':
        break;
      default:
       break;
    }
  }

  options->list_tests = list_tests;

  for (int index = optind; index < argc; index++)
    options->extra_args.push_back(argv[index]);
}


/*
 * Display the help message
 */
void DisplayHelpAndExit(char *argv[]) {
  cout << "Usage: " << argv[0] << " [options] <ip>[:port]\n"
  "\n"
  "Stress test an SLP SA.\n"
  "\n"
  "  -h, --help              Display this help message and exit.\n"
  "  -l, --log-level <level> Set the logging level 0 .. 4.\n"
  "  -t, --timeout <timeout> Number of ms to wait for responses.\n"
  "  --list-tests            List the test names\n"
  "  --tests <tests>         Limit to the given tests\n"
  " Example:\n"
  "   " << argv[0] << " 192.168.0.1\n"
  "   " << argv[0] << " 192.168.0.1:5568\n"
  << endl;
  exit(0);
}


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
  Options options;
  ParseOptions(argc, argv, &options);

  IPV4Address target_ip;

  if (options.list_tests)
    DisplayTestsAndExit();

  if (options.help || options.extra_args.size() != 1)
    DisplayHelpAndExit(argv);

  ola::InitLogging(options.log_level, ola::OLA_LOG_STDERR);
  ola::AppInit(argc, argv);

  vector<string> tests_to_run;
  if (!options.test_names.empty()) {
    ola::StringSplit(options.test_names, tests_to_run, ",");
  }

  IPV4SocketAddress target_endpoint;
  if (!IPV4SocketAddress::FromString(options.extra_args[0], &target_endpoint)) {
    // try just the IP
    IPV4Address target_ip;
    if (!IPV4Address::FromString(options.extra_args[0], &target_ip)) {
      OLA_WARN << "Invalid target : " << options.extra_args[0];
      return ola::EXIT_USAGE;
    }
    target_endpoint = IPV4SocketAddress(target_ip, ola::slp::DEFAULT_SLP_PORT);
  }

  TestRunner runner(options.timeout, tests_to_run, target_endpoint);
  runner.Run();
}
