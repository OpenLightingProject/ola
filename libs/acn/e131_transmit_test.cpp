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
 * e131_transmit_test.cpp
 * This sends custom E1.31 packets in order to test the implementation of a
 * remote node.
 * Copyright (C) 2010 Simon Newton
 *
 * The remote node needs to be listening for Universe 1.
 */

#include <getopt.h>
#include <string>
#include <vector>
#include "ola/DmxBuffer.h"
#include "ola/Logging.h"
#include "libs/acn/E131TestFramework.h"

using ola::DmxBuffer;
using std::string;
using std::vector;

DmxBuffer BufferFromString(const string &data) {
  DmxBuffer buffer;
  buffer.SetFromString(data);
  return buffer;
}


DmxBuffer BufferFromValue(uint8_t value) {
  DmxBuffer buffer;
  buffer.SetRangeToValue(0, value, ola::DMX_UNIVERSE_SIZE);
  return buffer;
}


TestState s1("Single Source Send",
             new NodeSimpleSend(20),
             new NodeInactive(),
             "512 x 20",
             BufferFromValue(20));
TestState s2("Single Source Timeout",
             new NodeInactive(),
             new NodeInactive(),
             "Loss of data after 2.5s",
             DmxBuffer());
TestState s3("Single Source Send",
             new NodeSimpleSend(10),
             new NodeInactive(),
             "512 x 10",
             BufferFromValue(10));
TestState s4("Single Source Terminate",
             new NodeTerminate(),
             new NodeInactive(),
             "Immediate loss of data",
             DmxBuffer());
TestState s5("Single Source Send",
             new NodeSimpleSend(30),
             new NodeInactive(),
             "512 x 30",
             BufferFromValue(30));
TestState s6("Single Source Terminate with data",
             new NodeTerminateWithData(10),
             new NodeInactive(),
             "Immediate loss of data, no values of 10",
             DmxBuffer());
TestState s7("Single Source priority = 201",
             new NodeSimpleSend(201),
             new NodeInactive(),
             "No data, priority > 200 should be ignored",
             DmxBuffer());
TestState s8("Single Source priority = 100",
             new NodeSimpleSend(100),
             new NodeInactive(),
             "512 x 100",
             BufferFromValue(100));
TestState s9("Single Source priority = 99",
             new NodeSimpleSend(99),
             new NodeInactive(),
             "512 x 99, missing data indicates a problem when a source reduces"
             " it's priority",
             BufferFromValue(99));
// stay in this state for 3s
TestState s10("Single Source Timeout",
              new NodeInactive(),
              new NodeInactive(),
              "Loss of data after 2.5s",
             DmxBuffer());
TestState s11("Single Source Terminate with data",
              new NodeTerminateWithData(10),
              new NodeInactive(),
              "No effect, source should have already timed out",
             DmxBuffer());

// now test sequence handling
TestState s12("Single Source Sequence Test",
              // 1 in 4 chance of sending a packet with 0s rather than 255s
              new NodeVarySequenceNumber(255, 0, 4),
              new NodeInactive(),
              "512x255, any 0s indicate a problem with seq #",
              BufferFromValue(255));
TestState s13("Single Source Terminate",
              new NodeTerminate(),
              new NodeInactive(),
              "Immediate loss of data",
              DmxBuffer());

// now we do the merge tests, this tests a second source appearing with a
// priority less than the active one
TestState s14("Single Source Send",
              new NodeSimpleSend(20),
              new NodeInactive(),
              "512 x 20",
              BufferFromValue(20));
TestState s15("Dual Sources with pri 20 & 10",
              new NodeSimpleSend(20),
              new NodeSimpleSend(10),
              "512 x 20, no values of 10 otherwise this indicates a priority "
              "problem",
              BufferFromValue(20));

RelaxedTestState s16("Dual Sources with pri 20 & 30",
                     new NodeSimpleSend(20),
                     new NodeSimpleSend(30),
                     "One packet of 512x20, the 512 x 30",
                     BufferFromValue(20),
                     BufferFromValue(30));
RelaxedTestState s17("Dual Sources with pri 20 & 10",
                     new NodeSimpleSend(20),
                     new NodeSimpleSend(10, "100,100,100,100"),
                     "512 x 20, may see single packet with 4 x 100",
                     BufferFromString("100,100,100,100"),
                     BufferFromValue(20));
RelaxedTestState s18("Dual Sources with pri 20 & 20, HTP merge",
                     new NodeSimpleSend(20, "1,1,100,100"),
                     new NodeSimpleSend(20, "100,100,1,1"),
                     "4 x 100 if we HTP merge for arbitration",
                     BufferFromString("1,1,100,100"),
                     BufferFromString("100,100,100,100"));
RelaxedTestState s19("Dual Sources with pri 20 & 20, HTP merge",
                     new NodeSimpleSend(20, "1,1,100,0"),
                     new NodeSimpleSend(20, "100,0,1,1"),
                     "[100,1,100,1] if we HTP merge for arbitration",
                     BufferFromString("100,100,100,1"),
                     BufferFromString("100,1,100,1"));
// timing is important here
OrderedTestState s20("Dual Sources with one timing out",
                     new NodeInactive(),
                     new NodeSimpleSend(20, "100,0,1,1"),
                     "[100,0,1,1] after 2.5s",
                     BufferFromString("100,1,100,1"),
                     BufferFromString("100,0,1,1"));
TestState s21("Timeout",
              new NodeInactive(),
              new NodeInactive(),
              "Loss of all data after 2.5s",
              BufferFromString("100,0,1,1"));

// now we test the case where a data arrives from a new source more than the
// active priority
TestState s22("Single Source Send",
              new NodeSimpleSend(20),
              new NodeInactive(),
              "512 x 20",
              BufferFromValue(20));
RelaxedTestState s23("Dual Sources with pri 20 & 30",
                     new NodeSimpleSend(20),
                     new NodeSimpleSend(30),
                     "512 x 20, followed by 512 x 30",
                     BufferFromValue(20),
                     BufferFromValue(30));
TestState s24("Both Sources Terminate",
              new NodeTerminate(),
              new NodeTerminate(),
              "Loss of data, may see 512 x 20",
              DmxBuffer());

// now we test the case where a data arrives from a new source equal to the
// active priority
TestState s25("Single Source Send",
              new NodeSimpleSend(20, "20,20,20,20"),
              new NodeInactive(),
              "20,20,20,20",
              BufferFromString("20,20,20,20"));
RelaxedTestState s26("Dual Sources with pri 20 & 20",
                     new NodeSimpleSend(20, "20,20,20,20"),
                     new NodeSimpleSend(20, "100,100,100,100"),
                     "[20,20,20,20], then  [100,100,100,100]",
                     BufferFromString("20,20,20,20"),
                     BufferFromString("100,100,100,100"));
RelaxedTestState s27("Terminate second source",
                     new NodeSimpleSend(20, "20,20,20,20"),
                     new NodeTerminate(),
                     "512 x 20",
                     BufferFromString("100,100,100,100"),
                     BufferFromString("20,20,20,20"));

TestState *states[] = {&s1, &s2, &s3, &s4, &s5, &s6, &s7, &s8, &s9, &s10,
                       &s11, &s11, &s12, &s13, &s14, &s15, &s16, &s17, &s18,
                       &s19, &s20, &s21, &s22, &s23, &s24, &s25, &s26, &s27,
                       NULL};


/*
 * Display the help message
 */
void DisplayHelp(const char *binary_name) {
  std::cout << "Usage: " << binary_name << " [--interactive]\n"
  "\n"
  "Run the E1.31 Transmit test. This test can run in one of two modes:\n"
  "  * interactive mode. This sends data to the multicast addresses\n"
  "    and a human gets to verify it.\n"
  "  * local mode (default). This starts a local E131Node and sends it data,\n"
  "    verifying against the expected output.\n"
  "\n"
  "  -h, --help                  Display this help message and exit.\n"
  "  -i, --interactive           Run in interactive mode.\n"
  << std::endl;
}


int main(int argc, char* argv[]) {
  bool interactive_mode = false;

  ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
  vector<TestState*> test_states;
  TestState **ptr = states;
  while (*ptr)
    test_states.push_back(*ptr++);

  static struct option long_options[] = {
      {"interactive", no_argument, 0, 'i'},
      {"help", no_argument, 0, 'h'},
      {0, 0, 0, 0}
    };

  int option_index = 0;

  while (1) {
    int c = getopt_long(argc, argv, "ih", long_options, &option_index);

    if (c == -1)
      break;

    switch (c) {
      case 0:
        break;
      case 'h':
        DisplayHelp(argv[0]);
        return 0;
      case 'i':
        interactive_mode = true;
        break;
      case '?':
        break;
      default:
        break;
    }
  }
  StateManager manager(test_states, interactive_mode);
  manager.Init();
  manager.Run();
  return manager.Passed() ? 0 : 1;
}
