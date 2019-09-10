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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * TestUtils.h
 * Useful functions that improve upon CPPUNIT's test functions
 * Copyright (C) 2012 Simon Newton
 */

#ifndef INCLUDE_OLA_TESTING_TESTUTILS_H_
#define INCLUDE_OLA_TESTING_TESTUTILS_H_

#include <stdint.h>
#include <cppunit/Asserter.h>
#include <cppunit/SourceLine.h>
#include <cppunit/extensions/HelperMacros.h>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace ola {
namespace testing {

typedef CPPUNIT_NS::SourceLine SourceLine;

// Assert that two data blocks are the same.
// Private, use OLA_ASSERT_DATA_EQUALS below.
void ASSERT_DATA_EQUALS(const SourceLine &source_line,
                        const uint8_t *expected,
                        unsigned int expected_length,
                        const uint8_t *actual,
                        unsigned int actual_length);

void ASSERT_DATA_EQUALS(const SourceLine &source_line,
                        const char *expected,
                        unsigned int expected_length,
                        const char *actual,
                        unsigned int actual_length);

// Private, use OLA_ASSERT_VECTOR_EQ below
template <typename T>
void _AssertVectorEq(const SourceLine &source_line,
                     const std::vector<T> &t1,
                     const std::vector<T> &t2) {
  _AssertEquals(t1.size(), t2.size(), source_line,
                "Vector sizes not equal");

  typename std::vector<T>::const_iterator iter1 = t1.begin();
  typename std::vector<T>::const_iterator iter2 = t2.begin();
  while (iter1 != t1.end()) {
    _AssertEquals(*iter1++, *iter2++, source_line,
                  "Vector elements not equal");
  }
}

// Private, use OLA_ASSERT_SET_EQ below
template <typename T>
void _AssertSetEq(const SourceLine &source_line,
                  const std::set<T> &t1,
                  const std::set<T> &t2) {
  _AssertEquals(t1.size(), t2.size(), source_line,
                "Set sizes not equal");

  typename std::set<T>::const_iterator iter1 = t1.begin();
  typename std::set<T>::const_iterator iter2 = t2.begin();
  while (iter1 != t1.end()) {
    _AssertEquals(*iter1++, *iter2++, source_line,
                  "Set elements not equal");
  }
}

// Useful methods. This allows us to switch between unit testing frameworks in
// the future. You should generally use the macros below instead
template <typename T>
inline void _AssertEquals(const SourceLine &source_line,
                          const T &expected,
                          const T &actual,
                          const std::string &message = "") {
  CPPUNIT_NS::assertEquals(expected,
                           actual,
                           source_line,
                           message);
}

inline void _Fail(const SourceLine &source_line,
                  const std::string &message) {
  CPPUNIT_NS::Asserter::fail(message,
                             source_line);
}

inline void _FailIf(const SourceLine &source_line,
                    bool shouldFail,
                    const std::string &message) {
  CPPUNIT_NS::Asserter::failIf(shouldFail,
                               message,
                               source_line);
}

// Useful macros. This allows us to switch between unit testing frameworks in
// the future.
#define OLA_SOURCELINE() \
  CPPUNIT_SOURCELINE()

#define OLA_ASSERT(condition)  \
  CPPUNIT_ASSERT(condition)

#define OLA_ASSERT_TRUE(condition)  \
  CPPUNIT_ASSERT(condition)

#define OLA_ASSERT_TRUE_MSG(condition, msg)  \
  CPPUNIT_ASSERT_MESSAGE(msg, condition)

#define OLA_ASSERT_FALSE(condition)  \
  CPPUNIT_ASSERT(!(condition))

#define OLA_ASSERT_FALSE_MSG(condition, msg)  \
  CPPUNIT_ASSERT_MESSAGE(msg, !(condition))

#define OLA_ASSERT_EQ(expected, output)  \
  CPPUNIT_ASSERT_EQUAL(expected, output)

#define OLA_ASSERT_EQ_MSG(expected, output, message)  \
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message, expected, output)

#define OLA_ASSERT_NE(expected, output)  \
  CPPUNIT_ASSERT((expected) != (output))

#define OLA_ASSERT_DOUBLE_EQ(expected, output, delta)  \
  CPPUNIT_ASSERT_DOUBLES_EQUAL(expected, output, delta)

#define OLA_ASSERT_LT(expected, output)  \
  CPPUNIT_ASSERT((expected) < (output))

#define OLA_ASSERT_LTE(expected, output)  \
  CPPUNIT_ASSERT((expected) <= (output))

#define OLA_ASSERT_GT(expected, output)  \
  CPPUNIT_ASSERT((expected) > (output))

#define OLA_ASSERT_GTE(expected, output)  \
  CPPUNIT_ASSERT((expected) >= (output))

#define OLA_ASSERT_VECTOR_EQ(expected, output)  \
  ola::testing::_AssertVectorEq(CPPUNIT_SOURCELINE(), (expected), (output))

#define OLA_ASSERT_SET_EQ(expected, output)  \
  ola::testing::_AssertSetEq(CPPUNIT_SOURCELINE(), (expected), (output))

#define OLA_ASSERT_DATA_EQUALS(expected, expected_length, actual, \
                               actual_length)  \
  ola::testing::ASSERT_DATA_EQUALS(OLA_SOURCELINE(), (expected), \
                                   (expected_length), (actual), \
                                   (actual_length))

#define OLA_ASSERT_DMX_EQUALS(expected, actual)  \
  ola::testing::ASSERT_DATA_EQUALS(OLA_SOURCELINE(), \
                                   (expected.GetRaw()), (expected.Size()), \
                                   (actual.GetRaw()), (actual.Size())); \
  OLA_ASSERT_EQ(expected, actual)

#define OLA_ASSERT_NULL(value) \
  CPPUNIT_NS::Asserter::failIf( \
    NULL != value, \
    CPPUNIT_NS::Message("Expression: " #value " != NULL"), \
    CPPUNIT_SOURCELINE())

#define OLA_ASSERT_NOT_NULL(value) \
  CPPUNIT_NS::Asserter::failIf( \
    NULL == value, \
    CPPUNIT_NS::Message("Expression: " #value " == NULL"), \
    CPPUNIT_SOURCELINE())

#define OLA_ASSERT_EMPTY(container) \
  CPPUNIT_NS::Asserter::failIf( \
    !container.empty(), \
    CPPUNIT_NS::Message("Expression: " #container " is not empty"), \
    CPPUNIT_SOURCELINE())

#define OLA_ASSERT_NOT_EMPTY(container) \
  CPPUNIT_NS::Asserter::failIf( \
    container.empty(), \
    CPPUNIT_NS::Message("Expression: " #container " is empty"), \
    CPPUNIT_SOURCELINE())

#define OLA_FAIL(reason)  \
  CPPUNIT_FAIL(reason)
}  // namespace testing
}  // namespace ola
#endif  // INCLUDE_OLA_TESTING_TESTUTILS_H_
