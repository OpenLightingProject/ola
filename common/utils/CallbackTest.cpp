/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * CallbackTest.cpp
 * Unittest for the autogen'ed Callback code
 * Copyright (C) 2005-2010 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <unistd.h>
#include <string>

#include "ola/Callback.h"
#include "ola/testing/TestUtils.h"


using std::string;

class CallbackTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(CallbackTest);
  CPPUNIT_TEST(testFunctionCallbacks);
  CPPUNIT_TEST(testMethodCallbacks);
  CPPUNIT_TEST(testFunctionCallbacks1);
  CPPUNIT_TEST(testMethodCallbacks1);
  CPPUNIT_TEST(testMethodCallbacks2);
  CPPUNIT_TEST_SUITE_END();

 public:
    void testFunctionCallbacks();
    void testMethodCallbacks();
    void testFunctionCallbacks1();
    void testMethodCallbacks1();
    void testMethodCallbacks2();
    void testMethodCallbacks4();

    void Method0() {}
    bool BoolMethod0() { return true; }

    void Method1(unsigned int i) {
      OLA_ASSERT_EQ(TEST_INT_VALUE, i);
    }

    bool BoolMethod1(unsigned int i) {
      OLA_ASSERT_EQ(TEST_INT_VALUE, i);
      return true;
    }

    void Method2(unsigned int i, int j) {
      OLA_ASSERT_EQ(TEST_INT_VALUE, i);
      OLA_ASSERT_EQ(TEST_INT_VALUE2, j);
    }

    bool BoolMethod2(unsigned int i, int j) {
      OLA_ASSERT_EQ(TEST_INT_VALUE, i);
      OLA_ASSERT_EQ(TEST_INT_VALUE2, j);
      return true;
    }

    void Method3(unsigned int i, int j, char c) {
      OLA_ASSERT_EQ(TEST_INT_VALUE, i);
      OLA_ASSERT_EQ(TEST_INT_VALUE2, j);
      OLA_ASSERT_EQ(TEST_CHAR_VALUE, c);
    }

    bool BoolMethod3(unsigned int i, int j, char c) {
      OLA_ASSERT_EQ(TEST_INT_VALUE, i);
      OLA_ASSERT_EQ(TEST_INT_VALUE2, j);
      OLA_ASSERT_EQ(TEST_CHAR_VALUE, c);
      return true;
    }

    void Method4(unsigned int i, int j, char c, const string &s) {
      OLA_ASSERT_EQ(TEST_INT_VALUE, i);
      OLA_ASSERT_EQ(TEST_INT_VALUE2, j);
      OLA_ASSERT_EQ(TEST_CHAR_VALUE, c);
      OLA_ASSERT_EQ(string(TEST_STRING_VALUE), s);
    }

    bool BoolMethod4(unsigned int i, int j, char c, const string &s) {
      OLA_ASSERT_EQ(TEST_INT_VALUE, i);
      OLA_ASSERT_EQ(TEST_INT_VALUE2, j);
      OLA_ASSERT_EQ(TEST_CHAR_VALUE, c);
      OLA_ASSERT_EQ(string(TEST_STRING_VALUE), s);
      return true;
    }

    static const unsigned int TEST_INT_VALUE;
    static const int TEST_INT_VALUE2;
    static const char TEST_CHAR_VALUE;
    static const char TEST_STRING_VALUE[];
};


const unsigned int CallbackTest::TEST_INT_VALUE = 42;
const int CallbackTest::TEST_INT_VALUE2 = 53;
const char CallbackTest::TEST_CHAR_VALUE = 'c';
const char CallbackTest::TEST_STRING_VALUE[] = "foo";
CPPUNIT_TEST_SUITE_REGISTRATION(CallbackTest);

using ola::BaseCallback1;
using ola::BaseCallback2;
using ola::BaseCallback4;
using ola::Callback0;
using ola::NewCallback;
using ola::NewCallback;
using ola::NewSingleCallback;
using ola::NewSingleCallback;
using ola::SingleUseCallback0;


// Functions used for testing
void Function0() {}
bool BoolFunction0() { return true; }

void Function1(unsigned int i) {
  OLA_ASSERT_EQ(CallbackTest::TEST_INT_VALUE, i);
}

bool BoolFunction1(unsigned int i) {
  OLA_ASSERT_EQ(CallbackTest::TEST_INT_VALUE, i);
  return true;
}

void Function2(unsigned int i, int j) {
  OLA_ASSERT_EQ(CallbackTest::TEST_INT_VALUE, i);
  OLA_ASSERT_EQ(CallbackTest::TEST_INT_VALUE2, j);
}

bool BoolFunction2(unsigned int i, int j) {
  OLA_ASSERT_EQ(CallbackTest::TEST_INT_VALUE, i);
  OLA_ASSERT_EQ(CallbackTest::TEST_INT_VALUE2, j);
  return true;
}


/*
 * Test the Function Callbacks class
 */
void CallbackTest::testFunctionCallbacks() {
  // no arg, void return closures
  SingleUseCallback0<void> *c1 = NewSingleCallback(&Function0);
  c1->Run();
  Callback0<void> *c2 = NewCallback(&Function0);
  c2->Run();
  c2->Run();
  delete c2;

  // no arg, bool closures
  SingleUseCallback0<bool> *c3 = NewSingleCallback(&BoolFunction0);
  OLA_ASSERT_TRUE(c3->Run());
  Callback0<bool> *c4 = NewCallback(&BoolFunction0);
  OLA_ASSERT_TRUE(c4->Run());
  OLA_ASSERT_TRUE(c4->Run());
  delete c4;

  // one arg, void return
  SingleUseCallback0<void> *c5 = NewSingleCallback(
      &Function1,
      TEST_INT_VALUE);
  c5->Run();
  Callback0<void> *c6 = NewCallback(&Function1, TEST_INT_VALUE);
  c6->Run();
  c6->Run();
  delete c6;

  // one arg, bool closures
  SingleUseCallback0<bool> *c7 = NewSingleCallback(
      &BoolFunction1,
      TEST_INT_VALUE);
  OLA_ASSERT_TRUE(c7->Run());
  Callback0<bool> *c8 = NewCallback(&BoolFunction1, TEST_INT_VALUE);
  OLA_ASSERT_TRUE(c8->Run());
  OLA_ASSERT_TRUE(c8->Run());
  delete c8;
}


/*
 * Test the Method Callbacks
 */
void CallbackTest::testMethodCallbacks() {
  // no arg, void return closures
  SingleUseCallback0<void> *c1 = NewSingleCallback(this,
                                                   &CallbackTest::Method0);
  c1->Run();
  Callback0<void> *c2 = NewCallback(this, &CallbackTest::Method0);
  c2->Run();
  c2->Run();
  delete c2;

  // no arg, bool closures
  SingleUseCallback0<bool> *c3 = NewSingleCallback(this,
                                                   &CallbackTest::BoolMethod0);
  OLA_ASSERT_TRUE(c3->Run());
  Callback0<bool> *c4 = NewCallback(this, &CallbackTest::BoolMethod0);
  OLA_ASSERT_TRUE(c4->Run());
  OLA_ASSERT_TRUE(c4->Run());
  delete c4;

  // one arg, void return
  SingleUseCallback0<void> *c5 = NewSingleCallback(
      this,
      &CallbackTest::Method1,
      TEST_INT_VALUE);
  c5->Run();
  Callback0<void> *c6 = NewCallback(this, &CallbackTest::Method1,
                                    TEST_INT_VALUE);
  c6->Run();
  c6->Run();
  delete c6;

  // one arg, bool closures
  SingleUseCallback0<bool> *c7 = NewSingleCallback(
      this,
      &CallbackTest::BoolMethod1,
      TEST_INT_VALUE);
  OLA_ASSERT_TRUE(c7->Run());
  Callback0<bool> *c8 = NewCallback(this,
                                    &CallbackTest::BoolMethod1,
                                    TEST_INT_VALUE);
  OLA_ASSERT_TRUE(c8->Run());
  OLA_ASSERT_TRUE(c8->Run());
  delete c8;

  // two arg, void return
  SingleUseCallback0<void> *c9 = NewSingleCallback(
      this,
      &CallbackTest::Method2,
      TEST_INT_VALUE,
      TEST_INT_VALUE2);
  c9->Run();
  Callback0<void> *c10 = NewCallback(this,
                                     &CallbackTest::Method2,
                                     TEST_INT_VALUE,
                                     TEST_INT_VALUE2);
  c10->Run();
  c10->Run();
  delete c10;

  // two arg, bool closures
  SingleUseCallback0<bool> *c11 = NewSingleCallback(
      this,
      &CallbackTest::BoolMethod2,
      TEST_INT_VALUE,
      TEST_INT_VALUE2);
  OLA_ASSERT_TRUE(c11->Run());
  Callback0<bool> *c12 = NewCallback(this,
                                     &CallbackTest::BoolMethod2,
                                     TEST_INT_VALUE,
                                     TEST_INT_VALUE2);
  OLA_ASSERT_TRUE(c12->Run());
  OLA_ASSERT_TRUE(c12->Run());
  delete c12;

  // three arg, void return
  SingleUseCallback0<void> *c13 = NewSingleCallback(
      this,
      &CallbackTest::Method3,
      TEST_INT_VALUE,
      TEST_INT_VALUE2,
      TEST_CHAR_VALUE);
  c13->Run();
  Callback0<void> *c14 = NewCallback(this,
                                     &CallbackTest::Method3,
                                     TEST_INT_VALUE,
                                     TEST_INT_VALUE2,
                                     TEST_CHAR_VALUE);
  c14->Run();
  c14->Run();
  delete c14;

  // three arg, bool closures
  SingleUseCallback0<bool> *c15 = NewSingleCallback(
      this,
      &CallbackTest::BoolMethod3,
      TEST_INT_VALUE,
      TEST_INT_VALUE2,
      TEST_CHAR_VALUE);
  OLA_ASSERT_TRUE(c15->Run());
  Callback0<bool> *c16 = NewCallback(this,
                                     &CallbackTest::BoolMethod3,
                                     TEST_INT_VALUE,
                                     TEST_INT_VALUE2,
                                     TEST_CHAR_VALUE);
  OLA_ASSERT_TRUE(c16->Run());
  OLA_ASSERT_TRUE(c16->Run());
  delete c16;
}



/*
 * Test the single argument function closures
 */
void CallbackTest::testFunctionCallbacks1() {
  // single arg, void return closures
  BaseCallback1<void, unsigned int> *c1 = NewSingleCallback(&Function1);
  c1->Run(TEST_INT_VALUE);
  BaseCallback1<void, unsigned int> *c2 = NewCallback(&Function1);
  c2->Run(TEST_INT_VALUE);
  c2->Run(TEST_INT_VALUE);
  delete c2;

  // test a function that returns bool
  BaseCallback1<bool, unsigned int> *c3 = NewSingleCallback(&BoolFunction1);
  OLA_ASSERT_TRUE(c3->Run(TEST_INT_VALUE));
  BaseCallback1<bool, unsigned int> *c4 = NewCallback(&BoolFunction1);
  OLA_ASSERT_TRUE(c4->Run(TEST_INT_VALUE));
  OLA_ASSERT_TRUE(c4->Run(TEST_INT_VALUE));
  delete c4;

  // single arg, void return closures
  BaseCallback1<void, int> *c6 = NewSingleCallback(
      &Function2,
      TEST_INT_VALUE);
  c6->Run(TEST_INT_VALUE2);
  BaseCallback1<void, int> *c7 = NewCallback(
    &Function2,
    TEST_INT_VALUE);
  c7->Run(TEST_INT_VALUE2);
  c7->Run(TEST_INT_VALUE2);
  delete c7;
}


/*
 * Test the Method Callbacks
 */
void CallbackTest::testMethodCallbacks1() {
  // test 1 arg callbacks that return unsigned ints
  BaseCallback1<void, unsigned int> *c1 = NewSingleCallback(
      this,
      &CallbackTest::Method1);
  c1->Run(TEST_INT_VALUE);
  BaseCallback1<void, unsigned int> *c2 = NewCallback(this,
                                                      &CallbackTest::Method1);
  c2->Run(TEST_INT_VALUE);
  c2->Run(TEST_INT_VALUE);
  delete c2;

  // test 1 arg callbacks that return bools
  BaseCallback1<bool, unsigned int> *c3 = NewSingleCallback(
      this,
      &CallbackTest::BoolMethod1);
  OLA_ASSERT_TRUE(c3->Run(TEST_INT_VALUE));
  BaseCallback1<bool, unsigned int> *c4 = NewCallback(
      this,
      &CallbackTest::BoolMethod1);
  OLA_ASSERT_TRUE(c4->Run(TEST_INT_VALUE));
  OLA_ASSERT_TRUE(c4->Run(TEST_INT_VALUE));
  delete c4;

  // test 1 arg initial, 1 arg deferred callbacks that return ints
  BaseCallback1<void, int> *c5 = NewSingleCallback(
      this,
      &CallbackTest::Method2,
      TEST_INT_VALUE);
  c5->Run(TEST_INT_VALUE2);
  BaseCallback1<void, int> *c6 = NewCallback(
      this,
      &CallbackTest::Method2,
      TEST_INT_VALUE);
  c6->Run(TEST_INT_VALUE2);
  c6->Run(TEST_INT_VALUE2);
  delete c6;

  // test 1 arg initial, 1 arg deferred callbacks that return bools
  BaseCallback1<bool, int> *c7 = NewSingleCallback(
      this,
      &CallbackTest::BoolMethod2,
      TEST_INT_VALUE);
  OLA_ASSERT_TRUE(c7->Run(TEST_INT_VALUE2));
  BaseCallback1<bool, int> *c8 = NewCallback(
      this,
      &CallbackTest::BoolMethod2,
      TEST_INT_VALUE);
  OLA_ASSERT_TRUE(c8->Run(TEST_INT_VALUE2));
  OLA_ASSERT_TRUE(c8->Run(TEST_INT_VALUE2));
  delete c8;

  // test 2 arg initial, 1 arg deferred callbacks that return ints
  BaseCallback1<void, char> *c9 = NewSingleCallback(
      this,
      &CallbackTest::Method3,
      TEST_INT_VALUE,
      TEST_INT_VALUE2);
  c9->Run(TEST_CHAR_VALUE);
  BaseCallback1<void, char> *c10 = NewCallback(
      this,
      &CallbackTest::Method3,
      TEST_INT_VALUE,
      TEST_INT_VALUE2);
  c10->Run(TEST_CHAR_VALUE);
  c10->Run(TEST_CHAR_VALUE);
  delete c10;

  // test 2 arg initial, 1 arg deferred callbacks that return bools
  BaseCallback1<bool, char> *c11 = NewSingleCallback(
      this,
      &CallbackTest::BoolMethod3,
      TEST_INT_VALUE,
      TEST_INT_VALUE2);
  OLA_ASSERT_TRUE(c11->Run(TEST_CHAR_VALUE));
  BaseCallback1<bool, char> *c12 = NewCallback(
      this,
      &CallbackTest::BoolMethod3,
      TEST_INT_VALUE,
      TEST_INT_VALUE2);
  OLA_ASSERT_TRUE(c12->Run(TEST_CHAR_VALUE));
  OLA_ASSERT_TRUE(c12->Run(TEST_CHAR_VALUE));
  delete c12;

  // test 3 arg initial, 1 arg deferred callbacks that return ints
  BaseCallback1<void, const string&> *c13 = NewSingleCallback(
      this,
      &CallbackTest::Method4,
      TEST_INT_VALUE,
      TEST_INT_VALUE2,
      TEST_CHAR_VALUE);
  c13->Run(TEST_STRING_VALUE);
  BaseCallback1<void, const string&> *c14 = NewCallback(
      this,
      &CallbackTest::Method4,
      TEST_INT_VALUE,
      TEST_INT_VALUE2,
      TEST_CHAR_VALUE);
  c14->Run(TEST_STRING_VALUE);
  c14->Run(TEST_STRING_VALUE);
  delete c14;

  // test 3 arg initial, 1 arg deferred callbacks that return bools
  BaseCallback1<bool, const string&> *c15 = NewSingleCallback(
      this,
      &CallbackTest::BoolMethod4,
      TEST_INT_VALUE,
      TEST_INT_VALUE2,
      TEST_CHAR_VALUE);
  OLA_ASSERT_TRUE(c15->Run(TEST_STRING_VALUE));
  BaseCallback1<bool, const string&> *c16 = NewCallback(
      this,
      &CallbackTest::BoolMethod4,
      TEST_INT_VALUE,
      TEST_INT_VALUE2,
      TEST_CHAR_VALUE);
  OLA_ASSERT_TRUE(c16->Run(TEST_STRING_VALUE));
  OLA_ASSERT_TRUE(c16->Run(TEST_STRING_VALUE));
  delete c16;
}


/*
 * Test the Method Callbacks
 */
void CallbackTest::testMethodCallbacks2() {
  // test 2 arg callbacks that return void
  BaseCallback2<void, unsigned int, int> *c1 = NewSingleCallback(
      this,
      &CallbackTest::Method2);
  c1->Run(TEST_INT_VALUE, TEST_INT_VALUE2);
  BaseCallback2<void, unsigned int, int> *c2 = NewCallback(
      this,
      &CallbackTest::Method2);
  c2->Run(TEST_INT_VALUE, TEST_INT_VALUE2);
  c2->Run(TEST_INT_VALUE, TEST_INT_VALUE2);
  delete c2;

  // test 2 arg callbacks that return bools
  BaseCallback2<bool, unsigned int, int> *c3 = NewSingleCallback(
      this,
      &CallbackTest::BoolMethod2);
  OLA_ASSERT_TRUE(c3->Run(TEST_INT_VALUE, TEST_INT_VALUE2));
  BaseCallback2<bool, unsigned int, int> *c4 = NewCallback(
      this,
      &CallbackTest::BoolMethod2);
  OLA_ASSERT_TRUE(c4->Run(TEST_INT_VALUE, TEST_INT_VALUE2));
  OLA_ASSERT_TRUE(c4->Run(TEST_INT_VALUE, TEST_INT_VALUE2));
  delete c4;

  // test 1 create time, 2 run time arg callbacks that return void
  BaseCallback2<void, int, char> *c5 = NewSingleCallback(
      this,
      &CallbackTest::Method3,
      TEST_INT_VALUE);
  c5->Run(TEST_INT_VALUE2, TEST_CHAR_VALUE);
  BaseCallback2<void, int, char> *c6 = NewCallback(
      this,
      &CallbackTest::Method3,
      TEST_INT_VALUE);
  c6->Run(TEST_INT_VALUE2, TEST_CHAR_VALUE);
  c6->Run(TEST_INT_VALUE2, TEST_CHAR_VALUE);
  delete c6;

  // test 1 create time, 2 run time arg callbacks that return bools
  BaseCallback2<bool, int, char> *c7 = NewSingleCallback(
      this,
      &CallbackTest::BoolMethod3,
      TEST_INT_VALUE);
  OLA_ASSERT_TRUE(c7->Run(TEST_INT_VALUE2, TEST_CHAR_VALUE));
  BaseCallback2<bool, int, char> *c8 = NewCallback(
      this,
      &CallbackTest::BoolMethod3,
      TEST_INT_VALUE);
  OLA_ASSERT_TRUE(c8->Run(TEST_INT_VALUE2, TEST_CHAR_VALUE));
  OLA_ASSERT_TRUE(c8->Run(TEST_INT_VALUE2, TEST_CHAR_VALUE));
  delete c8;
}


/*
 * Test the Method Callbacks
 */
void CallbackTest::testMethodCallbacks4() {
  // test 2 arg callbacks that return unsigned ints
  BaseCallback4<void, unsigned int, int, char, const string&> *c1 =
    NewSingleCallback(
      this,
      &CallbackTest::Method4);
  c1->Run(TEST_INT_VALUE, TEST_INT_VALUE2, TEST_CHAR_VALUE, TEST_STRING_VALUE);
  BaseCallback4<void, unsigned int, int, char, const string&> *c2 = NewCallback(
      this,
      &CallbackTest::Method4);
  c2->Run(TEST_INT_VALUE, TEST_INT_VALUE2, TEST_CHAR_VALUE, TEST_STRING_VALUE);
  c2->Run(TEST_INT_VALUE, TEST_INT_VALUE2, TEST_CHAR_VALUE, TEST_STRING_VALUE);
  delete c2;

  // test 2 arg callbacks that return bools
  BaseCallback4<bool, unsigned int, int, char, const string&> *c3 =
    NewSingleCallback(
      this,
      &CallbackTest::BoolMethod4);
  OLA_ASSERT_TRUE(c3->Run(TEST_INT_VALUE, TEST_INT_VALUE2, TEST_CHAR_VALUE,
                         TEST_STRING_VALUE));
  BaseCallback4<bool, unsigned int, int, char, const string&> *c4 =
    NewCallback(
      this,
      &CallbackTest::BoolMethod4);
  OLA_ASSERT_TRUE(c4->Run(TEST_INT_VALUE, TEST_INT_VALUE2, TEST_CHAR_VALUE,
                         TEST_STRING_VALUE));
  OLA_ASSERT_TRUE(c4->Run(TEST_INT_VALUE, TEST_INT_VALUE2, TEST_CHAR_VALUE,
                         TEST_STRING_VALUE));
  delete c4;
}
