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
 * PreferencesTest.cpp
 * Test fixture for the Preferences classes
 * Copyright (C) 2005 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <set>
#include <string>
#include <vector>

#include "ola/Logging.h"
#include "ola/StringUtils.h"
#include "olad/Preferences.h"
#include "ola/testing/TestUtils.h"


using ola::BoolValidator;
using ola::FileBackedPreferences;
using ola::FileBackedPreferencesFactory;
using ola::IntToString;
using ola::IntValidator;
using ola::UIntValidator;
using ola::MemoryPreferencesFactory;
using ola::Preferences;
using ola::SetValidator;
using ola::StringValidator;
using ola::IPv4Validator;
using ola::IPv6Validator;
using std::string;
using std::vector;


class PreferencesTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(PreferencesTest);
  CPPUNIT_TEST(testValidators);
  CPPUNIT_TEST(testGetSetRemove);
  CPPUNIT_TEST(testBool);
  CPPUNIT_TEST(testFactory);
  CPPUNIT_TEST(testLoad);
  CPPUNIT_TEST(testSave);
  CPPUNIT_TEST_SUITE_END();

 public:
    void setUp() {
      ola::InitLogging(ola::OLA_LOG_DEBUG, ola::OLA_LOG_STDERR);
    }
    void testValidators();
    void testGetSetRemove();
    void testBool();
    void testFactory();
    void testLoad();
    void testSave();
};


CPPUNIT_TEST_SUITE_REGISTRATION(PreferencesTest);

/*
 * Check the validators work
 */
void PreferencesTest::testValidators() {
  StringValidator string_validator;
  OLA_ASSERT(string_validator.IsValid("foo"));
  OLA_ASSERT_FALSE(string_validator.IsValid(""));

  std::set<string> values;
  values.insert("one");
  values.insert("two");
  SetValidator<string> set_validator(values);
  OLA_ASSERT(set_validator.IsValid("one"));
  OLA_ASSERT(set_validator.IsValid("two"));
  OLA_ASSERT_FALSE(set_validator.IsValid("zero"));
  OLA_ASSERT_FALSE(set_validator.IsValid("three"));

  std::set<unsigned int> values2;
  values2.insert(1);
  values2.insert(3);
  SetValidator<unsigned int> set_validator2(values2);
  OLA_ASSERT(set_validator2.IsValid("1"));
  OLA_ASSERT(set_validator2.IsValid("3"));
  OLA_ASSERT_FALSE(set_validator2.IsValid("0"));
  OLA_ASSERT_FALSE(set_validator2.IsValid("2"));
  OLA_ASSERT_FALSE(set_validator2.IsValid("4"));

  // a string validator that allows empty strings
  StringValidator string_validator2(true);
  OLA_ASSERT(string_validator2.IsValid("foo"));
  OLA_ASSERT(string_validator2.IsValid(""));

  BoolValidator bool_validator;
  OLA_ASSERT(bool_validator.IsValid("true"));
  OLA_ASSERT(bool_validator.IsValid("false"));
  OLA_ASSERT_FALSE(bool_validator.IsValid(""));

  IntValidator int_validator(-3, 4);
  OLA_ASSERT(int_validator.IsValid("-3"));
  OLA_ASSERT(int_validator.IsValid("0"));
  OLA_ASSERT(int_validator.IsValid("4"));
  OLA_ASSERT_FALSE(int_validator.IsValid("-4"));
  OLA_ASSERT_FALSE(int_validator.IsValid("5"));

  UIntValidator uint_validator(10, 14);
  OLA_ASSERT(uint_validator.IsValid("10"));
  OLA_ASSERT(uint_validator.IsValid("14"));
  OLA_ASSERT_FALSE(uint_validator.IsValid("0"));
  OLA_ASSERT_FALSE(uint_validator.IsValid("9"));
  OLA_ASSERT_FALSE(uint_validator.IsValid("15"));

  IPv4Validator ipv4_validator;  // empty ok
  OLA_ASSERT(ipv4_validator.IsValid(""));
  OLA_ASSERT(ipv4_validator.IsValid("1.2.3.4"));
  OLA_ASSERT(ipv4_validator.IsValid("10.0.255.1"));
  OLA_ASSERT_FALSE(ipv4_validator.IsValid("foo"));
  OLA_ASSERT_FALSE(ipv4_validator.IsValid("1.2.3"));
  OLA_ASSERT_FALSE(ipv4_validator.IsValid("1.2.3.4.5"));
  OLA_ASSERT_FALSE(ipv4_validator.IsValid("1.f00.3.4"));

  IPv4Validator ipv4_validator2(false);  // empty not ok
  OLA_ASSERT_FALSE(ipv4_validator2.IsValid(""));
  OLA_ASSERT(ipv4_validator2.IsValid("1.2.3.4"));
  OLA_ASSERT(ipv4_validator2.IsValid("10.0.255.1"));
  OLA_ASSERT_FALSE(ipv4_validator2.IsValid("foo"));
  OLA_ASSERT_FALSE(ipv4_validator2.IsValid("1.2.3"));
  OLA_ASSERT_FALSE(ipv4_validator2.IsValid("1.2.3.4.5"));
  OLA_ASSERT_FALSE(ipv4_validator2.IsValid("1.f00.3.4"));

  IPv6Validator ipv6_validator;  // empty ok
  OLA_ASSERT(ipv6_validator.IsValid(""));
  OLA_ASSERT(ipv6_validator.IsValid("2001:db8:1234:5678:90ab:cdef:feed:face"));
  OLA_ASSERT(ipv6_validator.IsValid("::ffff:1.2.3.4"));
  OLA_ASSERT(ipv6_validator.IsValid("::ffff:10.0.255.1"));
  OLA_ASSERT_FALSE(ipv6_validator.IsValid("foo"));
  OLA_ASSERT_FALSE(ipv6_validator.IsValid(
      "2001:db8:1234:5678:90ab:cdef:feed:face:0000"));
  OLA_ASSERT_FALSE(ipv6_validator.IsValid(
      "2001:db8:1234:5678:90ab:cdef:feed:gggg"));
  OLA_ASSERT_FALSE(ipv6_validator.IsValid("::ffff:1.2.3"));
  OLA_ASSERT_FALSE(ipv6_validator.IsValid("::ffff:1.2.3.4.5"));
  OLA_ASSERT_FALSE(ipv6_validator.IsValid("::ffff:1.f00.3.4"));

  IPv6Validator ipv6_validator2(false);  // empty not ok
  OLA_ASSERT_FALSE(ipv6_validator2.IsValid(""));
  OLA_ASSERT(ipv6_validator2.IsValid(
      "2001:db8:1234:5678:90ab:cdef:feed:face"));
  OLA_ASSERT(ipv6_validator2.IsValid("::ffff:1.2.3.4"));
  OLA_ASSERT(ipv6_validator2.IsValid("::ffff:10.0.255.1"));
  OLA_ASSERT_FALSE(ipv6_validator2.IsValid("foo"));
  OLA_ASSERT_FALSE(ipv6_validator2.IsValid(
      "2001:db8:1234:5678:90ab:cdef:feed:face:0000"));
  OLA_ASSERT_FALSE(ipv6_validator2.IsValid(
      "2001:db8:1234:5678:90ab:cdef:feed:gggg"));
  OLA_ASSERT_FALSE(ipv6_validator2.IsValid("::ffff:1.2.3"));
  OLA_ASSERT_FALSE(ipv6_validator2.IsValid("::ffff:1.2.3.4.5"));
  OLA_ASSERT_FALSE(ipv6_validator2.IsValid("::ffff:1.f00.3.4"));
}


/*
 * Check that we can get/set the preferences
 */
void PreferencesTest::testGetSetRemove() {
  MemoryPreferencesFactory factory;
  Preferences *preferences = factory.NewPreference("dummy");

  string key1 = "foo";
  string key2 = "bat";
  string value1 = "bar";
  string value2 = "baz";
  unsigned int value3 = 1;
  unsigned int value4 = 2;
  int value5 = 3;
  int value6 = 4;
  const char value7[] = "bar";
  const char value8[] = "baz";

  // test get/set/has single values string
  OLA_ASSERT_EQ(string(""), preferences->GetValue(key1));
  preferences->SetValue(key1, value1);
  OLA_ASSERT_EQ(value1, preferences->GetValue(key1));
  OLA_ASSERT(preferences->HasKey(key1));
  preferences->SetValue(key1, value2);
  OLA_ASSERT_EQ(value2, preferences->GetValue(key1));

  preferences->RemoveValue(key1);
  OLA_ASSERT_EQ(string(""), preferences->GetValue(key1));
  OLA_ASSERT_FALSE(preferences->HasKey(key1));

  // test get/set/has single values uint
  OLA_ASSERT_EQ(string(""), preferences->GetValue(key1));
  preferences->SetValue(key1, value3);
  OLA_ASSERT_EQ(IntToString(value3), preferences->GetValue(key1));
  OLA_ASSERT(preferences->HasKey(key1));
  preferences->SetValue(key1, value4);
  OLA_ASSERT_EQ(IntToString(value4), preferences->GetValue(key1));

  preferences->RemoveValue(key1);
  OLA_ASSERT_EQ(string(""), preferences->GetValue(key1));
  OLA_ASSERT_FALSE(preferences->HasKey(key1));

  // test get/set/has single values int
  OLA_ASSERT_EQ(string(""), preferences->GetValue(key1));
  preferences->SetValue(key1, value5);
  OLA_ASSERT_EQ(IntToString(value5), preferences->GetValue(key1));
  OLA_ASSERT(preferences->HasKey(key1));
  preferences->SetValue(key1, value6);
  OLA_ASSERT_EQ(IntToString(value6), preferences->GetValue(key1));

  preferences->RemoveValue(key1);
  OLA_ASSERT_EQ(string(""), preferences->GetValue(key1));
  OLA_ASSERT_FALSE(preferences->HasKey(key1));

  vector<string> values;
  // test get/set multiple value string
  values = preferences->GetMultipleValue(key2);
  OLA_ASSERT_EQ((size_t) 0, values.size());
  preferences->SetMultipleValue(key2, value1);
  values = preferences->GetMultipleValue(key2);
  OLA_ASSERT(preferences->HasKey(key2));
  OLA_ASSERT_EQ((size_t) 1, values.size());
  OLA_ASSERT_EQ(value1, values.at(0));
  preferences->SetMultipleValue(key2, value2);
  values = preferences->GetMultipleValue(key2);
  OLA_ASSERT_EQ((size_t) 2, values.size());
  OLA_ASSERT_EQ(value1, values.at(0));
  OLA_ASSERT_EQ(value2, values.at(1));
  preferences->RemoveValue(key2);

  // test get/set multiple value uint
  values = preferences->GetMultipleValue(key2);
  OLA_ASSERT_EQ((size_t) 0, values.size());
  preferences->SetMultipleValue(key2, value3);
  values = preferences->GetMultipleValue(key2);
  OLA_ASSERT(preferences->HasKey(key2));
  OLA_ASSERT_EQ((size_t) 1, values.size());
  OLA_ASSERT_EQ(IntToString(value3), values.at(0));
  preferences->SetMultipleValue(key2, value4);
  values = preferences->GetMultipleValue(key2);
  OLA_ASSERT_EQ((size_t) 2, values.size());
  OLA_ASSERT_EQ(IntToString(value3), values.at(0));
  OLA_ASSERT_EQ(IntToString(value4), values.at(1));
  preferences->RemoveValue(key2);

  // test get/set multiple value int
  values = preferences->GetMultipleValue(key2);
  OLA_ASSERT_EQ((size_t) 0, values.size());
  preferences->SetMultipleValue(key2, value5);
  values = preferences->GetMultipleValue(key2);
  OLA_ASSERT(preferences->HasKey(key2));
  OLA_ASSERT_EQ((size_t) 1, values.size());
  OLA_ASSERT_EQ(IntToString(value5), values.at(0));
  preferences->SetMultipleValue(key2, value6);
  values = preferences->GetMultipleValue(key2);
  OLA_ASSERT_EQ((size_t) 2, values.size());
  OLA_ASSERT_EQ(IntToString(value5), values.at(0));
  OLA_ASSERT_EQ(IntToString(value6), values.at(1));
  preferences->RemoveValue(key2);

  // test SetDefaultValue String
  OLA_ASSERT(preferences->SetDefaultValue(key1, StringValidator(), value1));
  OLA_ASSERT_EQ(value1, preferences->GetValue(key1));
  OLA_ASSERT_FALSE(preferences->SetDefaultValue(key1, StringValidator(),
                                                value2));
  OLA_ASSERT_EQ(value1, preferences->GetValue(key1));
  OLA_ASSERT(preferences->HasKey(key1));
  preferences->RemoveValue(key1);

  // test SetDefaultValue uint
  UIntValidator uint_validator(0, 3);
  OLA_ASSERT(preferences->SetDefaultValue(key1, uint_validator, value3));
  OLA_ASSERT_EQ(IntToString(value3), preferences->GetValue(key1));
  OLA_ASSERT_FALSE(preferences->SetDefaultValue(key1, uint_validator,
                                                value4));
  OLA_ASSERT_EQ(IntToString(value3), preferences->GetValue(key1));
  OLA_ASSERT(preferences->HasKey(key1));
  preferences->RemoveValue(key1);

  // test SetDefaultValue int
  IntValidator int_validator(0, 5);
  OLA_ASSERT(preferences->SetDefaultValue(key1, int_validator, value5));
  OLA_ASSERT_EQ(IntToString(value5), preferences->GetValue(key1));
  OLA_ASSERT_FALSE(preferences->SetDefaultValue(key1, int_validator,
                                                value6));
  OLA_ASSERT_EQ(IntToString(value5), preferences->GetValue(key1));
  OLA_ASSERT(preferences->HasKey(key1));
  preferences->RemoveValue(key1);

  // test SetDefaultValue char[]
  OLA_ASSERT(preferences->SetDefaultValue(key1, StringValidator(), value7));
  OLA_ASSERT_EQ(string(value7), preferences->GetValue(key1));
  OLA_ASSERT_FALSE(preferences->SetDefaultValue(key1, StringValidator(),
                                                value8));
  OLA_ASSERT_EQ(string(value7), preferences->GetValue(key1));
  OLA_ASSERT(preferences->HasKey(key1));
  preferences->RemoveValue(key1);

  // test SetDefaultValue bool
  OLA_ASSERT(preferences->SetDefaultValue(key1, BoolValidator(), true));
  OLA_ASSERT_EQ(string(BoolValidator::ENABLED), preferences->GetValue(key1));
  OLA_ASSERT_FALSE(preferences->SetDefaultValue(key1, BoolValidator(),
                                                false));
  OLA_ASSERT_EQ(string(BoolValidator::ENABLED), preferences->GetValue(key1));
  OLA_ASSERT(preferences->HasKey(key1));
  preferences->RemoveValue(key1);
}


/*
 * Check that we can get/set the preferences
 */
void PreferencesTest::testBool() {
  MemoryPreferencesFactory factory;
  Preferences *preferences = factory.NewPreference("dummy");

  string key1 = "foo";
  string value1 = "bar";

  // test get/set single values
  OLA_ASSERT_EQ(false, preferences->GetValueAsBool(key1));
  preferences->SetValueAsBool(key1, true);
  OLA_ASSERT_EQ(true, preferences->GetValueAsBool(key1));
  preferences->SetValueAsBool(key1, false);
  OLA_ASSERT_EQ(false, preferences->GetValueAsBool(key1));
  preferences->SetValue(key1, value1);
  OLA_ASSERT_EQ(false, preferences->GetValueAsBool(key1));
}


/*
 * Check that the factory works as intended
 */
void PreferencesTest::testFactory() {
  MemoryPreferencesFactory factory;
  string preferences_name = "dummy";
  Preferences *preferences = factory.NewPreference(preferences_name);
  Preferences *preferences2 = factory.NewPreference(preferences_name);
  OLA_ASSERT_EQ(preferences, preferences2);
}


/*
 * Check that we can load from a file
 */
void PreferencesTest::testLoad() {
  FileBackedPreferences *preferences = new FileBackedPreferences(
      "", "dummy", NULL);
  preferences->Clear();
  preferences->SetValue("foo", "bad");
  preferences->LoadFromFile(
      TEST_SRC_DIR "/olad/testdata/test_preferences.conf");

  OLA_ASSERT_EQ(string("bar"), preferences->GetValue("foo"));
  OLA_ASSERT(preferences->HasKey("foo"));
  OLA_ASSERT_EQ(string("bat"), preferences->GetValue("baz"));
  OLA_ASSERT(preferences->HasKey("baz"));

  vector<string> values = preferences->GetMultipleValue("multi");
  OLA_ASSERT_EQ((size_t) 3, values.size());
  OLA_ASSERT_EQ(string("1"), values.at(0));
  OLA_ASSERT_EQ(string("2"), values.at(1));
  OLA_ASSERT_EQ(string("3"), values.at(2));
  delete preferences;
}


void PreferencesTest::testSave() {
  const string data_path = TEST_BUILD_DIR "/olad/ola-output.conf";

  ola::FilePreferenceSaverThread saver_thread;
  saver_thread.Start();
  FileBackedPreferences *preferences = new FileBackedPreferences(
      TEST_BUILD_DIR "/olad", "output", &saver_thread);
  preferences->Clear();

  unlink(data_path.c_str());
  string key1 = "foo";
  string key2 = "bat";
  string key3 = "/dev/ttyUSB0";
  string value1 = "bar";
  string value2 = "baz";
  string value3 = "boo";
  string multi_key = "multi";
  preferences->SetValue(key1, value1);
  preferences->SetValue(key2, value2);
  preferences->SetValue(key3, value3);
  preferences->SetMultipleValue(multi_key, "1");
  preferences->SetMultipleValue(multi_key, "2");
  preferences->SetMultipleValue(multi_key, "3");
  preferences->Save();

  saver_thread.Synchronize();

  FileBackedPreferences *input_preferences = new
    FileBackedPreferences("", "input", NULL);
  input_preferences->LoadFromFile(data_path);
  OLA_ASSERT(*preferences == *input_preferences);
  delete preferences;
  delete input_preferences;

  saver_thread.Join();
}
