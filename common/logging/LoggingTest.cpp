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
 * LoggingTest.cpp
 * Test fixture for the Logging framework
 * Copyright (C) 2005-2009 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <deque>
#include <string>
#include <utility>
#include <vector>

#include "ola/testing/TestUtils.h"

#include "ola/Logging.h"
#include "ola/StringUtils.h"

using std::deque;
using std::vector;
using std::string;
using ola::IncrementLogLevel;
using ola::log_level;


class LoggingTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(LoggingTest);
  CPPUNIT_TEST(testLogging);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testLogging();
};


class MockLogDestination: public ola::LogDestination {
  public:
    void AddExpected(log_level level, string log_line);
    void Write(log_level level, const string &log_line);
    int LinesRemaining() const { return m_log_lines.size(); }
  private:
    deque<std::pair<log_level, string> > m_log_lines;
};


CPPUNIT_TEST_SUITE_REGISTRATION(LoggingTest);


void MockLogDestination::AddExpected(log_level level, string log_line) {
  std::pair<log_level, string> expected_result(level, log_line);
  m_log_lines.push_back(expected_result);
}


/*
 * Check that what gets written is what we expected
 */
void MockLogDestination::Write(log_level level, const string &log_line) {
  vector<string> tokens;
  ola::StringSplit(log_line, tokens, ":");
  vector<string>::iterator iter;
  OLA_ASSERT_EQ(tokens.size() , (size_t) 3);
  OLA_ASSERT_TRUE(m_log_lines.size() > 0);
  std::pair<log_level, string> expected_result = m_log_lines.at(0);
  m_log_lines.pop_front();
  OLA_ASSERT_EQ(expected_result.first, level);
  OLA_ASSERT_EQ(expected_result.second, tokens.at(2));
}

/*
 * Check that logging works correctly.
 */
void LoggingTest::testLogging() {
  MockLogDestination *destination = new MockLogDestination();
  InitLogging(ola::OLA_LOG_DEBUG, destination);
  destination->AddExpected(ola::OLA_LOG_DEBUG, " debug\n");
  OLA_DEBUG << "debug";
  destination->AddExpected(ola::OLA_LOG_INFO, " info\n");
  OLA_INFO << "info";
  destination->AddExpected(ola::OLA_LOG_WARN, " warn\n");
  OLA_WARN << "warn";
  destination->AddExpected(ola::OLA_LOG_FATAL, " fatal\n");
  OLA_FATAL << "fatal";

  // Now make sure nothing below WARN is logged
  ola::SetLogLevel(ola::OLA_LOG_WARN);
  OLA_DEBUG << "debug";
  OLA_INFO << "info";
  destination->AddExpected(ola::OLA_LOG_WARN, " warn\n");
  OLA_WARN << "warn";
  destination->AddExpected(ola::OLA_LOG_FATAL, " fatal\n");
  OLA_FATAL << "fatal";
  OLA_ASSERT_EQ(destination->LinesRemaining(), 0);

  // set the log level to INFO
  IncrementLogLevel();
  OLA_DEBUG << "debug";
  destination->AddExpected(ola::OLA_LOG_INFO, " info\n");
  OLA_INFO << "info";
  destination->AddExpected(ola::OLA_LOG_WARN, " warn\n");
  OLA_WARN << "warn";
  destination->AddExpected(ola::OLA_LOG_FATAL, " fatal\n");
  OLA_FATAL << "fatal";
  OLA_ASSERT_EQ(destination->LinesRemaining(), 0);

  IncrementLogLevel();
  // this should wrap to NONE
  IncrementLogLevel();
  OLA_DEBUG << "debug";
  OLA_INFO << "info";
  OLA_WARN << "warn";
  OLA_FATAL << "fatal";
  OLA_ASSERT_EQ(destination->LinesRemaining(), 0);
}
