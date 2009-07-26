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
 * ExportMap.cpp
 * Test fixture for the ExportMap and Variable classes
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <string>
#include <cppunit/extensions/HelperMacros.h>

#include <ola/ExportMap.h>

using namespace ola;
using namespace std;

class ExportMapTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(ExportMapTest);
  CPPUNIT_TEST(testIntegerVariable);
  CPPUNIT_TEST(testStringVariable);
  CPPUNIT_TEST(testStringMapVariable);
  CPPUNIT_TEST(testIntMapVariable);
  CPPUNIT_TEST(testExportMap);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testIntegerVariable();
    void testStringVariable();
    void testStringMapVariable();
    void testIntMapVariable();
    void testExportMap();
};


CPPUNIT_TEST_SUITE_REGISTRATION(ExportMapTest);


/*
 * Check that the IntegerVariable works correctly.
 */
void ExportMapTest::testIntegerVariable() {
  string name = "foo";
  IntegerVariable var(name);

  CPPUNIT_ASSERT_EQUAL(var.Name(), name);
  CPPUNIT_ASSERT_EQUAL(var.Value(), string("0"));
  CPPUNIT_ASSERT_EQUAL(var.Get(), 0);
  var.Increment();
  CPPUNIT_ASSERT_EQUAL(var.Get(), 1);
  CPPUNIT_ASSERT_EQUAL(var.Value(), string("1"));
  var.Decrement();
  CPPUNIT_ASSERT_EQUAL(var.Get(), 0);
  CPPUNIT_ASSERT_EQUAL(var.Value(), string("0"));
  var.Set(100);
  CPPUNIT_ASSERT_EQUAL(var.Get(), 100);
  CPPUNIT_ASSERT_EQUAL(var.Value(), string("100"));
}


/*
 * Check that the StringVariable works correctly.
 */
void ExportMapTest::testStringVariable() {
  string name = "foo";
  StringVariable var(name);

  CPPUNIT_ASSERT_EQUAL(var.Name(), name);
  CPPUNIT_ASSERT_EQUAL(var.Value(), string(""));
  CPPUNIT_ASSERT_EQUAL(var.Get(), string(""));
  var.Set("bar");
  CPPUNIT_ASSERT_EQUAL(var.Value(), string("bar"));
  CPPUNIT_ASSERT_EQUAL(var.Get(), string("bar"));
}


/*
 * Check that the StringMap works correctly.
 */
void ExportMapTest::testStringMapVariable() {
  string name = "foo";
  string label = "count";
  StringMap var(name, label);

  CPPUNIT_ASSERT_EQUAL(var.Name(), name);
  CPPUNIT_ASSERT_EQUAL(var.Label(), label);
  CPPUNIT_ASSERT_EQUAL(var.Value(), string("map:count"));

  string key1 = "key1";
  string value1 = "value1";
  var.Set(key1, value1);
  CPPUNIT_ASSERT_EQUAL(var.Get(key1), value1);
  CPPUNIT_ASSERT_EQUAL(var.Value(), string("map:count key1:value1"));

  string key2 = "key2";
  string value2 = "value2";
  var.Set(key2, value2);
  CPPUNIT_ASSERT_EQUAL(var.Get(key2), value2);
  CPPUNIT_ASSERT_EQUAL(var.Value(), string("map:count key1:value1 key2:value2"));

  var.Remove(key1);
  CPPUNIT_ASSERT_EQUAL(var.Get(key1), string(""));
  CPPUNIT_ASSERT_EQUAL(var.Value(), string("map:count key2:value2"));
}


/*
 * Check that the IntMap works correctly.
 */
void ExportMapTest::testIntMapVariable() {
  string name = "foo";
  string label = "count";
  IntMap var(name, label);

  CPPUNIT_ASSERT_EQUAL(var.Name(), name);
  CPPUNIT_ASSERT_EQUAL(var.Label(), label);
  CPPUNIT_ASSERT_EQUAL(var.Value(), string("map:count"));

  string key1 = "key1";
  var.Set(key1, 100);
  CPPUNIT_ASSERT_EQUAL(var.Get(key1), 100);
  CPPUNIT_ASSERT_EQUAL(var.Value(), string("map:count key1:100"));

  string key2 = "key2";
  var.Set(key2, 99);
  CPPUNIT_ASSERT_EQUAL(var.Get(key2), 99);
  CPPUNIT_ASSERT_EQUAL(var.Value(), string("map:count key1:100 key2:99"));

  var.Remove(key1);
  CPPUNIT_ASSERT_EQUAL(var.Get(key1), 0);
  CPPUNIT_ASSERT_EQUAL(var.Value(), string("map:count key2:99"));
}

/*
 * Check the export map works correctly.
 */
void ExportMapTest::testExportMap() {
  ExportMap map;
  string int_var_name = "int_var";
  string str_var_name = "str_var";
  string map_var_name = "map_var";
  string map_var_label = "label";
  IntegerVariable *int_var = map.GetIntegerVar(int_var_name);
  StringVariable *str_var = map.GetStringVar(str_var_name);
  StringMap *map_var = map.GetStringMapVar(map_var_name, map_var_label);

  CPPUNIT_ASSERT_EQUAL(int_var->Name(), int_var_name);
  CPPUNIT_ASSERT_EQUAL(str_var->Name(), str_var_name);
  CPPUNIT_ASSERT_EQUAL(map_var->Name(), map_var_name);
  CPPUNIT_ASSERT_EQUAL(map_var->Label(), map_var_label);

  map_var = map.GetStringMapVar(map_var_name);
  CPPUNIT_ASSERT_EQUAL(map_var->Name(), map_var_name);
  CPPUNIT_ASSERT_EQUAL(map_var->Label(), map_var_label);

  vector<BaseVariable*> variables = map.AllVariables();
  CPPUNIT_ASSERT_EQUAL(variables.size(), (size_t) 3);
}
