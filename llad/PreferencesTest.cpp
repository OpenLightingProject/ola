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
 * PreferencesTest.cpp
 * Test fixture for the Preferences classes
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <string>
#include <cppunit/extensions/HelperMacros.h>

#include "llad/Preferences.h"

using namespace lla;
using namespace std;


class PreferencesTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(PreferencesTest);
  CPPUNIT_TEST(testGetSetRemove);
  CPPUNIT_TEST(testFactory);
  CPPUNIT_TEST(testLoad);
  CPPUNIT_TEST(testSave);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testGetSetRemove();
    void testFactory();
    void testLoad();
    void testSave();
};


CPPUNIT_TEST_SUITE_REGISTRATION(PreferencesTest);


/*
 * Check that we can get/set the preferences
 */
void PreferencesTest::testGetSetRemove() {
  MemoryPreferencesFactory factory;
  Preferences *preferences = factory.NewPreference("dummy");

  string key1 = "foo";
  string value1 = "bar";
  string value2 = "baz";

  // test get/set single values
  CPPUNIT_ASSERT_EQUAL(string(""), preferences->GetValue(key1));
  preferences->SetValue(key1, value1);
  CPPUNIT_ASSERT_EQUAL(value1, preferences->GetValue(key1));
  preferences->SetValue(key1, value2);
  CPPUNIT_ASSERT_EQUAL(value2, preferences->GetValue(key1));

  preferences->RemoveValue(key1);
  CPPUNIT_ASSERT_EQUAL(string(""), preferences->GetValue(key1));

  // test get/set multiple value
  string key2 = "bat";
  vector<string> values = preferences->GetMultipleValue(key2);
  CPPUNIT_ASSERT_EQUAL((size_t) 0, values.size());
  preferences->SetMultipleValue(key2, value1);
  values = preferences->GetMultipleValue(key2);
  CPPUNIT_ASSERT_EQUAL((size_t) 1, values.size());
  CPPUNIT_ASSERT_EQUAL(value1, values.at(0));
  preferences->SetMultipleValue(key2, value2);
  values = preferences->GetMultipleValue(key2);
  CPPUNIT_ASSERT_EQUAL((size_t) 2, values.size());
  CPPUNIT_ASSERT_EQUAL(value1, values.at(0));
  CPPUNIT_ASSERT_EQUAL(value2, values.at(1));
}


/*
 * Check that the factory works as intended
 */
void PreferencesTest::testFactory() {
  MemoryPreferencesFactory factory;
  string preferences_name = "dummy";
  Preferences *preferences = factory.NewPreference(preferences_name);
  Preferences *preferences2 = factory.NewPreference(preferences_name);
  CPPUNIT_ASSERT_EQUAL(preferences, preferences2);
}


/*
 * Check that we can load from a file
 */
void PreferencesTest::testLoad() {
  FileBackedPreferences *preferences = new FileBackedPreferences("dummy");
  preferences->Clear();
  preferences->SetValue("foo", "bad");
  preferences->LoadFromFile("./testdata/test_preferences.conf");

  CPPUNIT_ASSERT_EQUAL(string("bar"), preferences->GetValue("foo"));
  CPPUNIT_ASSERT_EQUAL(string("bat"), preferences->GetValue("baz"));

  vector<string> values = preferences->GetMultipleValue("multi");
  CPPUNIT_ASSERT_EQUAL((size_t) 3, values.size());
  CPPUNIT_ASSERT_EQUAL(string("1"), values.at(0));
  CPPUNIT_ASSERT_EQUAL(string("2"), values.at(1));
  CPPUNIT_ASSERT_EQUAL(string("3"), values.at(2));
  delete preferences;
}


void PreferencesTest::testSave() {
  FileBackedPreferencesFactory factory;
  FileBackedPreferences *preferences = new FileBackedPreferences("dummy");
  preferences->Clear();

  string data_path = "./testdata/preferences.output";
  string key1 = "foo";
  string key2 = "bat";
  string value1 = "bar";
  string value2 = "baz";
  string multi_key = "multi";
  preferences->SetValue(key1, value1);
  preferences->SetValue(key2, value2);
  preferences->SetMultipleValue(multi_key, "1");
  preferences->SetMultipleValue(multi_key, "2");
  preferences->SetMultipleValue(multi_key, "3");
  preferences->SaveToFile(data_path);

  FileBackedPreferences *input_preferences = new
    FileBackedPreferences("input");
  input_preferences->LoadFromFile(data_path);
  CPPUNIT_ASSERT(*preferences == *input_preferences);
  delete preferences;
  delete input_preferences;
}
