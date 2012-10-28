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

#include <cppunit/extensions/HelperMacros.h>
#include <string>
#include <vector>

#include "ola/ExportMap.h"
#include "ola/testing/TestUtils.h"

using ola::BaseVariable;
using ola::BoolVariable;
using ola::CounterVariable;
using ola::ExportMap;
using ola::IntMap;
using ola::IntegerVariable;
using ola::StringMap;
using ola::StringVariable;
using std::string;
using std::vector;


class ExportMapTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(ExportMapTest);
  CPPUNIT_TEST(testIntegerVariable);
  CPPUNIT_TEST(testCounterVariable);
  CPPUNIT_TEST(testStringVariable);
  CPPUNIT_TEST(testBoolVariable);
  CPPUNIT_TEST(testStringMapVariable);
  CPPUNIT_TEST(testIntMapVariable);
  CPPUNIT_TEST(testExportMap);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testIntegerVariable();
    void testCounterVariable();
    void testStringVariable();
    void testBoolVariable();
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

  OLA_ASSERT_EQ(var.Name(), name);
  OLA_ASSERT_EQ(var.Value(), string("0"));
  OLA_ASSERT_EQ(var.Get(), 0);
  var++;
  OLA_ASSERT_EQ(var.Get(), 1);
  OLA_ASSERT_EQ(var.Value(), string("1"));
  var--;
  OLA_ASSERT_EQ(var.Get(), 0);
  OLA_ASSERT_EQ(var.Value(), string("0"));
  var.Set(100);
  OLA_ASSERT_EQ(var.Get(), 100);
  OLA_ASSERT_EQ(var.Value(), string("100"));
}


/*
 * Check that the CounterVariable works correctly.
 */
void ExportMapTest::testCounterVariable() {
  string name = "foo";
  CounterVariable var(name);

  OLA_ASSERT_EQ(var.Name(), name);
  OLA_ASSERT_EQ(var.Value(), string("0"));
  OLA_ASSERT_EQ((unsigned int) 0, var.Get());
  var++;
  OLA_ASSERT_EQ((unsigned int) 1, var.Get());
  OLA_ASSERT_EQ(var.Value(), string("1"));
  var += 10;
  OLA_ASSERT_EQ((unsigned int) 11, var.Get());
  OLA_ASSERT_EQ(var.Value(), string("11"));
  var += 100;
  OLA_ASSERT_EQ((unsigned int) 111, var.Get());
  OLA_ASSERT_EQ(var.Value(), string("111"));
}


/*
 * Check that the StringVariable works correctly.
 */
void ExportMapTest::testStringVariable() {
  string name = "foo";
  StringVariable var(name);

  OLA_ASSERT_EQ(var.Name(), name);
  OLA_ASSERT_EQ(var.Value(), string(""));
  OLA_ASSERT_EQ(var.Get(), string(""));
  var.Set("bar");
  OLA_ASSERT_EQ(var.Value(), string("bar"));
  OLA_ASSERT_EQ(var.Get(), string("bar"));
}


/*
 * Check that the BoolVariable works correctly.
 */
void ExportMapTest::testBoolVariable() {
  string name = "foo";
  BoolVariable var(name);

  OLA_ASSERT_EQ(name, var.Name());
  OLA_ASSERT_EQ(false, var.Get());
  OLA_ASSERT_EQ(string("0"), var.Value());
  var.Set(true);
  OLA_ASSERT_EQ(string("1"), var.Value());
  OLA_ASSERT_EQ(true, var.Get());
}

/*
 * Check that the StringMap works correctly.
 */
void ExportMapTest::testStringMapVariable() {
  string name = "foo";
  string label = "count";
  StringMap var(name, label);

  OLA_ASSERT_EQ(var.Name(), name);
  OLA_ASSERT_EQ(var.Label(), label);
  OLA_ASSERT_EQ(var.Value(), string("map:count"));

  string key1 = "key1";
  string value1 = "value1";
  var[key1] = value1;
  OLA_ASSERT_EQ(value1, var[key1]);
  OLA_ASSERT_EQ(var.Value(), string("map:count key1:\"value1\""));

  string key2 = "key2";
  string value2 = "value 2";
  var[key2] = value2;
  OLA_ASSERT_EQ(value2, var[key2]);
  OLA_ASSERT_EQ(var.Value(),
                       string("map:count key1:\"value1\" key2:\"value 2\""));

  var.Remove(key1);
  OLA_ASSERT_EQ(string(""), var[key1]);
  var.Remove(key1);
  OLA_ASSERT_EQ(var.Value(), string("map:count key2:\"value 2\""));
  var[key2] = "foo\"";
  OLA_ASSERT_EQ(var.Value(), string("map:count key2:\"foo\\\"\""));
}


/*
 * Check that the IntMap works correctly.
 */
void ExportMapTest::testIntMapVariable() {
  string name = "foo";
  string label = "count";
  IntMap var(name, label);

  OLA_ASSERT_EQ(var.Name(), name);
  OLA_ASSERT_EQ(var.Label(), label);
  OLA_ASSERT_EQ(var.Value(), string("map:count"));

  string key1 = "key1";
  var[key1] = 100;
  OLA_ASSERT_EQ(100, var[key1]);
  OLA_ASSERT_EQ(var.Value(), string("map:count key1:100"));

  string key2 = "key2";
  var[key2] = 99;
  OLA_ASSERT_EQ(99, var[key2]);
  OLA_ASSERT_EQ(var.Value(), string("map:count key1:100 key2:99"));

  var.Remove(key1);
  OLA_ASSERT_EQ(0, var[key1]);
  var.Remove(key1);
  OLA_ASSERT_EQ(var.Value(), string("map:count key2:99"));
  var.Remove(key2);

  // check references work
  string key3 = "key3";
  int &var1 = var[key3];
  OLA_ASSERT_EQ(0, var1);
  var1++;
  OLA_ASSERT_EQ(1, var[key3]);
  OLA_ASSERT_EQ(var.Value(), string("map:count key3:1"));
}

/*
 * Check the export map works correctly.
 */
void ExportMapTest::testExportMap() {
  ExportMap map;
  string bool_var_name = "bool_var";
  string int_var_name = "int_var";
  string str_var_name = "str_var";
  string map_var_name = "map_var";
  string map_var_label = "label";
  BoolVariable *bool_var = map.GetBoolVar(bool_var_name);
  IntegerVariable *int_var = map.GetIntegerVar(int_var_name);
  StringVariable *str_var = map.GetStringVar(str_var_name);
  StringMap *map_var = map.GetStringMapVar(map_var_name, map_var_label);

  OLA_ASSERT_EQ(bool_var->Name(), bool_var_name);
  OLA_ASSERT_EQ(int_var->Name(), int_var_name);
  OLA_ASSERT_EQ(str_var->Name(), str_var_name);
  OLA_ASSERT_EQ(map_var->Name(), map_var_name);
  OLA_ASSERT_EQ(map_var->Label(), map_var_label);

  map_var = map.GetStringMapVar(map_var_name);
  OLA_ASSERT_EQ(map_var->Name(), map_var_name);
  OLA_ASSERT_EQ(map_var->Label(), map_var_label);

  vector<BaseVariable*> variables = map.AllVariables();
  OLA_ASSERT_EQ(variables.size(), (size_t) 4);
}
