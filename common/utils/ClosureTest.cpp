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
 * ClosureTest.cpp
 * Unittest for String functions.
 * Copyright (C) 2005-2010 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <unistd.h>
#include <string>
#include "ola/Closure.h"

using std::string;

class ClosureTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(ClosureTest);
  CPPUNIT_TEST(testFunctionClosures);
  CPPUNIT_TEST(testMethodClosures);
  CPPUNIT_TEST(testMethodCallbacks1);
  CPPUNIT_TEST(testMethodCallbacks2);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testFunctionClosures();
    void testMethodClosures();
    void testMethodCallbacks1();
    void testMethodCallbacks2();
    void testMethodCallbacks4();

    void Method0() {}
    bool BoolMethod0() { return true; }

    void Method1(unsigned int i) {
      CPPUNIT_ASSERT_EQUAL(TEST_INT_VALUE, i);
    }

    bool BoolMethod1(unsigned int i) {
      CPPUNIT_ASSERT_EQUAL(TEST_INT_VALUE, i);
      return true;
    }

    void Method2(unsigned int i, int j) {
      CPPUNIT_ASSERT_EQUAL(TEST_INT_VALUE, i);
      CPPUNIT_ASSERT_EQUAL(TEST_INT_VALUE2, j);
    }

    bool BoolMethod2(unsigned int i, int j) {
      CPPUNIT_ASSERT_EQUAL(TEST_INT_VALUE, i);
      CPPUNIT_ASSERT_EQUAL(TEST_INT_VALUE2, j);
      return true;
    }

    void Method4(unsigned int i, int j, char c, const string &s) {
      CPPUNIT_ASSERT_EQUAL(TEST_INT_VALUE, i);
      CPPUNIT_ASSERT_EQUAL(TEST_INT_VALUE2, j);
      CPPUNIT_ASSERT_EQUAL(TEST_CHAR_VALUE, c);
      CPPUNIT_ASSERT_EQUAL(string(TEST_STRING_VALUE), s);
    }

    bool BoolMethod4(unsigned int i, int j, char c, const string &s) {
      CPPUNIT_ASSERT_EQUAL(TEST_INT_VALUE, i);
      CPPUNIT_ASSERT_EQUAL(TEST_INT_VALUE2, j);
      CPPUNIT_ASSERT_EQUAL(TEST_CHAR_VALUE, c);
      CPPUNIT_ASSERT_EQUAL(string(TEST_STRING_VALUE), s);
      return true;
    }

    static const unsigned int TEST_INT_VALUE;
    static const int TEST_INT_VALUE2;
    static const char TEST_CHAR_VALUE;
    static const char TEST_STRING_VALUE[];
};


const unsigned int ClosureTest::TEST_INT_VALUE = 42;
const int ClosureTest::TEST_INT_VALUE2 = 53;
const char ClosureTest::TEST_CHAR_VALUE = 'c';
const char ClosureTest::TEST_STRING_VALUE[] = "foo";
CPPUNIT_TEST_SUITE_REGISTRATION(ClosureTest);

using ola::BaseCallback1;
using ola::BaseCallback2;
using ola::BaseCallback4;
using ola::Closure;
using ola::NewCallback;
using ola::NewClosure;
using ola::NewSingleCallback;
using ola::NewSingleClosure;
using ola::SingleUseClosure;


// Functions used for testing
void Function0() {}
bool BoolFunction0() { return true; }

void Function1(unsigned int i) {
  CPPUNIT_ASSERT_EQUAL(ClosureTest::TEST_INT_VALUE, i);
}

bool BoolFunction1(unsigned int i) {
  CPPUNIT_ASSERT_EQUAL(ClosureTest::TEST_INT_VALUE, i);
  return true;
}


/*
 * Test the Function Closures class
 */
void ClosureTest::testFunctionClosures() {
  // no arg, void return closures
  SingleUseClosure<void> *c1 = NewSingleClosure(&Function0);
  c1->Run();
  Closure<void> *c2 = NewClosure(&Function0);
  c2->Run();
  c2->Run();
  delete c2;

  // no arg, bool closures
  SingleUseClosure<bool> *c3 = NewSingleClosure(&BoolFunction0);
  CPPUNIT_ASSERT(c3->Run());
  Closure<bool> *c4 = NewClosure(&BoolFunction0);
  CPPUNIT_ASSERT(c4->Run());
  CPPUNIT_ASSERT(c4->Run());
  delete c4;

  // one arg, void return
  SingleUseClosure<void> *c5 = NewSingleClosure(
      &Function1,
      TEST_INT_VALUE);
  c5->Run();
  Closure<void> *c6 = NewClosure(&Function1, TEST_INT_VALUE);
  c6->Run();
  c6->Run();
  delete c6;

  // one arg, bool closures
  SingleUseClosure<bool> *c7 = NewSingleClosure(
      &BoolFunction1,
      TEST_INT_VALUE);
  CPPUNIT_ASSERT(c7->Run());
  Closure<bool> *c8 = NewClosure(&BoolFunction1, TEST_INT_VALUE);
  CPPUNIT_ASSERT(c8->Run());
  CPPUNIT_ASSERT(c8->Run());
  delete c8;
}


/*
 * Test the Method Closures
 */
void ClosureTest::testMethodClosures() {
  // no arg, void return closures
  SingleUseClosure<void> *c1 = NewSingleClosure(this, &ClosureTest::Method0);
  c1->Run();
  Closure<void> *c2 = NewClosure(this, &ClosureTest::Method0);
  c2->Run();
  c2->Run();
  delete c2;

  // no arg, bool closures
  SingleUseClosure<bool> *c3 = NewSingleClosure(this,
                                                &ClosureTest::BoolMethod0);
  CPPUNIT_ASSERT(c3->Run());
  Closure<bool> *c4 = NewClosure(this, &ClosureTest::BoolMethod0);
  CPPUNIT_ASSERT(c4->Run());
  CPPUNIT_ASSERT(c4->Run());
  delete c4;

  // one arg, void return
  SingleUseClosure<void> *c5 = NewSingleClosure(
      this,
      &ClosureTest::Method1,
      TEST_INT_VALUE);
  c5->Run();
  Closure<void> *c6 = NewClosure(this, &ClosureTest::Method1, TEST_INT_VALUE);
  c6->Run();
  c6->Run();
  delete c6;

  // one arg, bool closures
  SingleUseClosure<bool> *c7 = NewSingleClosure(
      this,
      &ClosureTest::BoolMethod1,
      TEST_INT_VALUE);
  CPPUNIT_ASSERT(c7->Run());
  Closure<bool> *c8 = NewClosure(this,
                                 &ClosureTest::BoolMethod1,
                                 TEST_INT_VALUE);
  CPPUNIT_ASSERT(c8->Run());
  CPPUNIT_ASSERT(c8->Run());
  delete c8;

  // two arg, void return
  SingleUseClosure<void> *c9 = NewSingleClosure(
      this,
      &ClosureTest::Method2,
      TEST_INT_VALUE,
      TEST_INT_VALUE2);
  c9->Run();
  Closure<void> *c10 = NewClosure(this,
                                  &ClosureTest::Method2,
                                  TEST_INT_VALUE,
                                  TEST_INT_VALUE2);
  c10->Run();
  c10->Run();
  delete c10;

  // two arg, bool closures
  SingleUseClosure<bool> *c11 = NewSingleClosure(
      this,
      &ClosureTest::BoolMethod2,
      TEST_INT_VALUE,
      TEST_INT_VALUE2);
  CPPUNIT_ASSERT(c11->Run());
  Closure<bool> *c12 = NewClosure(this,
                                  &ClosureTest::BoolMethod2,
                                  TEST_INT_VALUE,
                                  TEST_INT_VALUE2);
  CPPUNIT_ASSERT(c12->Run());
  CPPUNIT_ASSERT(c12->Run());
  delete c12;
}


/*
 * Test the Method Callbacks
 */
void ClosureTest::testMethodCallbacks1() {
  // test 1 arg callbacks that return unsigned ints
  BaseCallback1<void, unsigned int> *c1 = NewSingleCallback(
      this,
      &ClosureTest::Method1);
  c1->Run(TEST_INT_VALUE);
  BaseCallback1<void, unsigned int> *c2 = NewCallback(this,
                                                      &ClosureTest::Method1);
  c2->Run(TEST_INT_VALUE);
  c2->Run(TEST_INT_VALUE);
  delete c2;

  // test 1 arg callbacks that return bools
  BaseCallback1<bool, unsigned int> *c3 = NewSingleCallback(
      this,
      &ClosureTest::BoolMethod1);
  CPPUNIT_ASSERT(c3->Run(TEST_INT_VALUE));
  BaseCallback1<bool, unsigned int> *c4 = NewCallback(
      this,
      &ClosureTest::BoolMethod1);
  CPPUNIT_ASSERT(c4->Run(TEST_INT_VALUE));
  CPPUNIT_ASSERT(c4->Run(TEST_INT_VALUE));
  delete c4;

  // test 1 arg initial, 1 arg deferred callbacks that return ints
  BaseCallback1<void, int> *c5 = NewSingleCallback(
      this,
      &ClosureTest::Method2,
      TEST_INT_VALUE);
  c5->Run(TEST_INT_VALUE2);
  BaseCallback1<void, int> *c6 = NewCallback(
      this,
      &ClosureTest::Method2,
      TEST_INT_VALUE);
  c6->Run(TEST_INT_VALUE2);
  c6->Run(TEST_INT_VALUE2);
  delete c6;

  // test 1 arg initial, 1 arg deferred callbacks that return bools
  BaseCallback1<bool, int> *c7 = NewSingleCallback(
      this,
      &ClosureTest::BoolMethod2,
      TEST_INT_VALUE);
  CPPUNIT_ASSERT(c7->Run(TEST_INT_VALUE2));
  BaseCallback1<bool, int> *c8 = NewCallback(
      this,
      &ClosureTest::BoolMethod2,
      TEST_INT_VALUE);
  CPPUNIT_ASSERT(c8->Run(TEST_INT_VALUE2));
  CPPUNIT_ASSERT(c8->Run(TEST_INT_VALUE2));
  delete c8;
}


/*
 * Test the Method Callbacks
 */
void ClosureTest::testMethodCallbacks2() {
  // test 2 arg callbacks that return unsigned ints
  BaseCallback2<void, unsigned int, int> *c1 = NewSingleCallback(
      this,
      &ClosureTest::Method2);
  c1->Run(TEST_INT_VALUE, TEST_INT_VALUE2);
  BaseCallback2<void, unsigned int, int> *c2 = NewCallback(
      this,
      &ClosureTest::Method2);
  c2->Run(TEST_INT_VALUE, TEST_INT_VALUE2);
  c2->Run(TEST_INT_VALUE, TEST_INT_VALUE2);
  delete c2;

  // test 2 arg callbacks that return bools
  BaseCallback2<bool, unsigned int, int> *c3 = NewSingleCallback(
      this,
      &ClosureTest::BoolMethod2);
  CPPUNIT_ASSERT(c3->Run(TEST_INT_VALUE, TEST_INT_VALUE2));
  BaseCallback2<bool, unsigned int, int> *c4 = NewCallback(
      this,
      &ClosureTest::BoolMethod2);
  CPPUNIT_ASSERT(c4->Run(TEST_INT_VALUE, TEST_INT_VALUE2));
  CPPUNIT_ASSERT(c4->Run(TEST_INT_VALUE, TEST_INT_VALUE2));
  delete c4;
}


/*
 * Test the Method Callbacks
 */
void ClosureTest::testMethodCallbacks4() {
  // test 2 arg callbacks that return unsigned ints
  BaseCallback4<void, unsigned int, int, char, const string&> *c1 =
    NewSingleCallback(
      this,
      &ClosureTest::Method4);
  c1->Run(TEST_INT_VALUE, TEST_INT_VALUE2, TEST_CHAR_VALUE, TEST_STRING_VALUE);
  BaseCallback4<void, unsigned int, int, char, const string&> *c2 = NewCallback(
      this,
      &ClosureTest::Method4);
  c2->Run(TEST_INT_VALUE, TEST_INT_VALUE2, TEST_CHAR_VALUE, TEST_STRING_VALUE);
  c2->Run(TEST_INT_VALUE, TEST_INT_VALUE2, TEST_CHAR_VALUE, TEST_STRING_VALUE);
  delete c2;

  // test 2 arg callbacks that return bools
  BaseCallback4<bool, unsigned int, int, char, const string&> *c3 =
    NewSingleCallback(
      this,
      &ClosureTest::BoolMethod4);
  CPPUNIT_ASSERT(c3->Run(TEST_INT_VALUE, TEST_INT_VALUE2, TEST_CHAR_VALUE,
                         TEST_STRING_VALUE));
  BaseCallback4<bool, unsigned int, int, char, const string&> *c4 =
    NewCallback(
      this,
      &ClosureTest::BoolMethod4);
  CPPUNIT_ASSERT(c4->Run(TEST_INT_VALUE, TEST_INT_VALUE2, TEST_CHAR_VALUE,
                         TEST_STRING_VALUE));
  CPPUNIT_ASSERT(c4->Run(TEST_INT_VALUE, TEST_INT_VALUE2, TEST_CHAR_VALUE,
                         TEST_STRING_VALUE));
  delete c4;
}
