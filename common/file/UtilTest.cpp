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
 * UtilTest.cpp
 * Unittest for file util functions.
 * Copyright (C) 2013 Peter Newman
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string>
#include <iterator>
#include <vector>

#include "ola/StringUtils.h"
#include "ola/file/Util.h"
#include "ola/testing/TestUtils.h"

using ola::file::FilenameFromPath;
using ola::file::FilenameFromPathOrDefault;
using ola::file::FilenameFromPathOrPath;
using ola::file::JoinPaths;
using ola::file::FindMatchingFiles;
using std::string;

class UtilTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(UtilTest);
  CPPUNIT_TEST(testJoinPaths);
  CPPUNIT_TEST(testFilenameFromPath);
  CPPUNIT_TEST(testFindMatchingFiles);
  CPPUNIT_TEST_SUITE_END();

 public:
  void testFilenameFromPath();
  void testJoinPaths();
  void testFindMatchingFiles();
};


CPPUNIT_TEST_SUITE_REGISTRATION(UtilTest);

void UtilTest::testJoinPaths() {
  // Same behaviour as os.path.join()
  OLA_ASSERT_EQ(string("/tmp/1"), JoinPaths("/tmp", "1"));
  OLA_ASSERT_EQ(string("/tmp/1"), JoinPaths("/tmp/", "1"));
  OLA_ASSERT_EQ(string("1"), JoinPaths("", "1"));
  OLA_ASSERT_EQ(string("/tmp/"), JoinPaths("/tmp/", ""));
  OLA_ASSERT_EQ(string("/tmp"), JoinPaths("/tmp", ""));
  OLA_ASSERT_EQ(string("/foo"), JoinPaths("/tmp", "/foo"));
  OLA_ASSERT_EQ(string(""), JoinPaths("", ""));
}

/*
 * Test the FilenameFromPath function
 */
void UtilTest::testFilenameFromPath() {
  // TODO(Peter): Make these tests work on Windows too
  OLA_ASSERT_EQ(string(""), FilenameFromPath(""));
  OLA_ASSERT_EQ(string(""), FilenameFromPath("foo"));
  OLA_ASSERT_EQ(string(""), FilenameFromPath("/"));
  OLA_ASSERT_EQ(string("foo"), FilenameFromPath("/foo"));
  OLA_ASSERT_EQ(string(""), FilenameFromPath("/foo/"));
  OLA_ASSERT_EQ(string("bar"), FilenameFromPath("/foo/bar"));
  OLA_ASSERT_EQ(string(""), FilenameFromPath("/foo/bar/"));
  OLA_ASSERT_EQ(string("baz"), FilenameFromPath("/foo/bar/baz"));

  OLA_ASSERT_EQ(string("bak"), FilenameFromPathOrDefault("", "bak"));
  OLA_ASSERT_EQ(string("bak"), FilenameFromPathOrDefault("foo", "bak"));
  OLA_ASSERT_EQ(string(""), FilenameFromPathOrDefault("/", "bak"));
  OLA_ASSERT_EQ(string("foo"), FilenameFromPathOrDefault("/foo", "bak"));
  OLA_ASSERT_EQ(string(""), FilenameFromPathOrDefault("/foo/", "bak"));
  OLA_ASSERT_EQ(string("bar"), FilenameFromPathOrDefault("/foo/bar", "bak"));
  OLA_ASSERT_EQ(string(""), FilenameFromPathOrDefault("/foo/bar/", "bak"));
  OLA_ASSERT_EQ(string("baz"),
                FilenameFromPathOrDefault("/foo/bar/baz", "bak"));

  OLA_ASSERT_EQ(string(""), FilenameFromPathOrPath(""));
  OLA_ASSERT_EQ(string("foo"), FilenameFromPathOrPath("foo"));
  OLA_ASSERT_EQ(string(""), FilenameFromPathOrPath("/"));
  OLA_ASSERT_EQ(string("foo"), FilenameFromPathOrPath("/foo"));
  OLA_ASSERT_EQ(string(""), FilenameFromPathOrPath("/foo/"));
  OLA_ASSERT_EQ(string("bar"), FilenameFromPathOrPath("/foo/bar"));
  OLA_ASSERT_EQ(string(""), FilenameFromPathOrPath("/foo/bar/"));
  OLA_ASSERT_EQ(string("baz"), FilenameFromPathOrPath("/foo/bar/baz"));
}

/*
 * Test the FindMatchingFiles function
 */
void UtilTest::testFindMatchingFiles() {
  bool okay = false;

  std::vector<std::string> files;
  std::vector<std::string>::iterator file;

  OLA_ASSERT_TRUE_MSG(ola::file::PATH_SEPARATOR == '/' ||
                      ola::file::PATH_SEPARATOR == '\\',
                      "ola::file::PATH_SEPARATOR is neither / nor \\");

  okay = FindMatchingFiles(std::string(TEST_SRC_DIR) +
                           ola::file::PATH_SEPARATOR + std::string("man"),
                           std::string("rdm_"), &files);

  OLA_ASSERT_TRUE_MSG(okay, "FindMatchingFiles returned false");

  // At the time this test was written, there were 3 files in folder "man"
  // starting with "rdm_". If this changed, please adapt the number below
  // Or find something better to match against
  OLA_ASSERT_EQ_MSG(3, (int)files.size(),
                    "Not exactly 3 files man/rdm_* returned");

  bool rdm_model_collector_found = false;
  bool rdm_responder_test_found = false;
  bool rdm_test_server_found = false;

  // Iterate over all files that have been returned and check if the ones
  // we expected are in that list
  for (file = files.begin(); file < files.end(); file++) {
    if (ola::StringEndsWith(*file, "rdm_model_collector.py.1")) {
      // make sure it has not been reported as found before
      OLA_ASSERT_FALSE(rdm_model_collector_found);
      rdm_model_collector_found = true;
    } else if (ola::StringEndsWith(*file, "rdm_responder_test.py.1")) {
      // make sure it has not been reported as found before
      OLA_ASSERT_FALSE(rdm_responder_test_found);
      rdm_responder_test_found = true;
    } else if (ola::StringEndsWith(*file, "rdm_test_server.py.1")) {
      // make sure it has not been reported as found before
      OLA_ASSERT_FALSE(rdm_test_server_found);
      rdm_test_server_found = true;
    }
  }

  OLA_ASSERT_TRUE_MSG(rdm_model_collector_found,
                      "Result lacks rdm_model_collector.py.1");
  OLA_ASSERT_TRUE_MSG(rdm_responder_test_found,
                      "Result lacks rdm_responder_test.py.1");
  OLA_ASSERT_TRUE_MSG(rdm_test_server_found,
                      "Result lacks rdm_test_server.py.1");
}
