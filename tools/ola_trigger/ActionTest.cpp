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
 * ActionTest.cpp
 * Test fixture for the Action classes
 * Copyright (C) 2011 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <ola/Logging.h>
#include <sstream>
#include <string>
#include <vector>

#include "tools/ola_trigger/Action.h"
#include "ola/testing/TestUtils.h"


using std::vector;
using std::string;


class ActionTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(ActionTest);
  CPPUNIT_TEST(testVariableAssignment);
  CPPUNIT_TEST(testVariableAssignmentInterpolation);
  CPPUNIT_TEST(testCommandAction);
  CPPUNIT_TEST_SUITE_END();

 public:
    void testVariableAssignment();
    void testVariableAssignmentInterpolation();
    void testCommandAction();

    void setUp() {
      ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
    }
};


CPPUNIT_TEST_SUITE_REGISTRATION(ActionTest);


/**
 * A Mock CommandAction which doesn't fork and exec
 */
class MockCommandAction: CommandAction {
 public:
    MockCommandAction(const string &command,
                      const vector<string> &args)
      : CommandAction(command, args) {
    }

    void Execute(Context *context, uint8_t slot_value);
    void CheckArgs(int32_t line, const char* args[]);

 private:
    vector<string> m_interpolated_args;
};


/**
 * Build the list of args and save it.
 */
void MockCommandAction::Execute(Context *context, uint8_t) {
  m_interpolated_args.clear();

  char **args = BuildArgList(context);
  char **ptr = args;
  while (*ptr)
    m_interpolated_args.push_back(string(*ptr++));
  FreeArgList(args);
}


/**
 * Check what we got matches what we expected
 */
void MockCommandAction::CheckArgs(int32_t line, const char* args[]) {
  std::stringstream str;
  str << "From ActionTest.cpp:" << line;
  const char **ptr = args;
  vector<string>::const_iterator iter = m_interpolated_args.begin();
  while (*ptr && iter != m_interpolated_args.end())
    OLA_ASSERT_EQ_MSG(string(*ptr++), *iter++, str.str());

  if (iter != m_interpolated_args.end()) {
    str << ", got extra args: ";
    while (iter != m_interpolated_args.end()) {
      str << *iter;
      iter++;
      if (iter != m_interpolated_args.end())
        str << ", ";
    }
    OLA_FAIL(str.str());
  } else if (*ptr) {
    str << ", missing args: ";
    while (*ptr) {
      str << *ptr++;
      if (*ptr)
        str << ", ";
    }
    OLA_FAIL(str.str());
  }
  m_interpolated_args.clear();
}


/*
 * Check that the VariableAssignmentActions work.
 */
void ActionTest::testVariableAssignment() {
  const string VARIABLE_ONE = "one";
  const string FOO_VALUE = "foo";
  const string BAR_VALUE = "bar";

  Context context;
  string value;
  OLA_ASSERT_FALSE(context.Lookup(VARIABLE_ONE, &value));

  // trigger the action
  VariableAssignmentAction action(VARIABLE_ONE, FOO_VALUE);
  action.Execute(&context, 0);

  // check the context has updated
  OLA_ASSERT(context.Lookup(VARIABLE_ONE, &value));
  OLA_ASSERT_EQ(FOO_VALUE, value);

  // another action
  VariableAssignmentAction action2(VARIABLE_ONE, BAR_VALUE);
  action2.Execute(&context, 0);

  OLA_ASSERT(context.Lookup(VARIABLE_ONE, &value));
  OLA_ASSERT_EQ(BAR_VALUE, value);
}


/**
 * Check that variable interpolation works with the VariableAssignmentAction
 */
void ActionTest::testVariableAssignmentInterpolation() {
  const string VARIABLE_NAME = "var1";
  const string ASSIGNMENT_STRING = "${slot_offset} = ${slot_value}";

  Context context;
  context.Update(Context::SLOT_OFFSET_VARIABLE, "1");
  context.Update(Context::SLOT_VALUE_VARIABLE, "100");
  string value;

  VariableAssignmentAction action(VARIABLE_NAME, ASSIGNMENT_STRING);
  action.Execute(&context, 1);

  OLA_ASSERT(context.Lookup(VARIABLE_NAME, &value));
  OLA_ASSERT_EQ(string("1 = 100"), value);
}


/**
 * Test the command action
 */
void ActionTest::testCommandAction() {
  Context context;
  vector<string> args;
  args.push_back("one");
  args.push_back("two");
  MockCommandAction action("echo", args);
  action.Execute(&context, 0);

  const char *expected_args[] = {"echo", "one", "two", NULL};
  action.CheckArgs(__LINE__, expected_args);

  // now check interpolated variables
  args.push_back("_${slot_offset}_");
  args.push_back("_${slot_value}_");
  MockCommandAction action2("echo", args);

  context.SetSlotOffset(1);
  context.SetSlotValue(100);
  action2.Execute(&context, 0);

  const char *expected_args2[] = {"echo", "one", "two", "_1_", "_100_", NULL};
  action2.CheckArgs(__LINE__, expected_args2);
}
