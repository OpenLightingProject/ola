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

#include <deque>
#include <vector>
#include <cppunit/extensions/HelperMacros.h>
#include <lla/Logging.h>
#include <lla/StringUtils.h>

using namespace lla;
using std::deque;
using std::vector;

class LoggingTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(LoggingTest);
  CPPUNIT_TEST(testLogging);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testLogging();
};


class MockLogDestination: public lla::LogDestination {
  public:
    void AddExpected(log_level level, string log_line);
    void Write(log_level level, string &log_line);
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
void MockLogDestination::Write(log_level level, string &log_line) {
  vector<string> tokens;
  StringSplit(log_line, tokens, ":");
  vector<string>::iterator iter;
  CPPUNIT_ASSERT_EQUAL(tokens.size() , (size_t) 3);
  CPPUNIT_ASSERT(m_log_lines.size() > 0);
  std::pair<log_level, string> expected_result = m_log_lines.at(0);
  m_log_lines.pop_front();
  CPPUNIT_ASSERT_EQUAL(expected_result.first, level);
  CPPUNIT_ASSERT_EQUAL(expected_result.second, tokens.at(2));
}

/*
 * Check that logging works correctly.
 */
void LoggingTest::testLogging() {
  MockLogDestination *destination = new MockLogDestination();
  InitLogging(LLA_LOG_DEBUG, destination);
  destination->AddExpected(LLA_LOG_DEBUG, " debug\n");
  LLA_DEBUG << "debug";
  destination->AddExpected(LLA_LOG_INFO, " info\n");
  LLA_INFO << "info";
  destination->AddExpected(LLA_LOG_WARN, " warn\n");
  LLA_WARN << "warn";
  destination->AddExpected(LLA_LOG_FATAL, " fatal\n");
  LLA_FATAL << "fatal";

  // Now make sure nothing below WARN is logged
  SetLogLevel(LLA_LOG_WARN);
  LLA_DEBUG << "debug";
  LLA_INFO << "info";
  destination->AddExpected(LLA_LOG_WARN, " warn\n");
  LLA_WARN << "warn";
  destination->AddExpected(LLA_LOG_FATAL, " fatal\n");
  LLA_FATAL << "fatal";
  CPPUNIT_ASSERT_EQUAL(destination->LinesRemaining(), 0);

  // set the log level to INFO
  IncrementLogLevel();
  LLA_DEBUG << "debug";
  destination->AddExpected(LLA_LOG_INFO, " info\n");
  LLA_INFO << "info";
  destination->AddExpected(LLA_LOG_WARN, " warn\n");
  LLA_WARN << "warn";
  destination->AddExpected(LLA_LOG_FATAL, " fatal\n");
  LLA_FATAL << "fatal";
  CPPUNIT_ASSERT_EQUAL(destination->LinesRemaining(), 0);

  IncrementLogLevel();
  // this should wrap to NONE
  IncrementLogLevel();
  LLA_DEBUG << "debug";
  LLA_INFO << "info";
  LLA_WARN << "warn";
  LLA_FATAL << "fatal";
  CPPUNIT_ASSERT_EQUAL(destination->LinesRemaining(), 0);
}
