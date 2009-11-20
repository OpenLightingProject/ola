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
 * Exported Variables
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <string>
#include <map>
#include <vector>
#include <iostream>
#include "ola/ExportMap.h"

namespace ola {

using std::string;
using std::stringstream;

const string IntegerVariable::Value() const {
  stringstream out;
  out << m_value;
  return out.str();
}


/*
 * Return the string representation of this map variable.
 *
 * The form is:
 *   var_name  map:label_name key1:value1 key2:value2
 * @return the string representation of the variable.
 */
template<typename Type>
const string MapVariable<Type>::Value() const {
  stringstream value;
  value << "map:" << m_label;
  typename map<string, Type>::const_iterator iter;
  for (iter = m_variables.begin(); iter != m_variables.end(); ++iter) {
    value << " " << iter->first << ":" << iter->second;
  }
  return value.str();
}


ExportMap::~ExportMap() {
  map<string, IntegerVariable*>::const_iterator int_iter;
  map<string, StringVariable*>::const_iterator string_iter;
  map<string, StringMap*>::const_iterator str_map_iter;
  map<string, IntMap*>::const_iterator int_map_iter;

  for (int_iter = m_int_variables.begin();
       int_iter != m_int_variables.end(); int_iter++)
    delete int_iter->second;

  for (string_iter = m_string_variables.begin();
       string_iter != m_string_variables.end(); string_iter++)
    delete string_iter->second;

  for (str_map_iter = m_str_map_variables.begin();
       str_map_iter != m_str_map_variables.end(); str_map_iter++)
    delete str_map_iter->second;

  for (int_map_iter = m_int_map_variables.begin();
       int_map_iter != m_int_map_variables.end(); int_map_iter++)
    delete int_map_iter->second;

  m_int_variables.clear();
  m_string_variables.clear();
  m_str_map_variables.clear();
  m_int_variables.clear();
}


/*
 * Lookup or create an integer variable.
 *
 * @param name the name of this variable.
 * @return an IntergerVariable
 */
IntegerVariable *ExportMap::GetIntegerVar(const string &name) {
  map<string, IntegerVariable*>::iterator iter;
  iter = m_int_variables.find(name);

  if (iter == m_int_variables.end()) {
    IntegerVariable *var = new IntegerVariable(name);
    std::pair<string, IntegerVariable*> pair(name, var);
    m_int_variables.insert(pair);
    return var;
  }
  return iter->second;
}


/*
 * Lookup or create a string variable.
 *
 * @param name the name of the variable.
 * @return a StringVariable.
 */
StringVariable *ExportMap::GetStringVar(const string &name) {
  map<string, StringVariable*>::iterator iter;
  iter = m_string_variables.find(name);

  if (iter == m_string_variables.end()) {
    StringVariable *var = new StringVariable(name);
    std::pair<string, StringVariable*> pair(name, var);
    m_string_variables.insert(pair);
    return var;
  }
  return iter->second;
}


/*
 * Lookup or create a string map variable
 *
 * @param name the name of the variable
 * @param label the label to use for the map (optional)
 * @return a MapVariable
 */
StringMap *ExportMap::GetStringMapVar(const string &name, const string &label) {
  map<string, StringMap*>::iterator iter;
  iter = m_str_map_variables.find(name);

  if (iter == m_str_map_variables.end()) {
    StringMap *var = new StringMap(name, label);
    std::pair<string, StringMap*> pair(name, var);
    m_str_map_variables.insert(pair);
    return var;
  }
  return iter->second;
}


/*
 * Lookup or create an int map variable
 *
 * @param name the name of the variable
 * @param label the label to use for the map (optional)
 * @return a MapVariable
 */
IntMap *ExportMap::GetIntMapVar(const string &name, const string &label) {
  map<string, IntMap*>::iterator iter;
  iter = m_int_map_variables.find(name);

  if (iter == m_int_map_variables.end()) {
    IntMap *var = new IntMap(name, label);
    std::pair<string, IntMap*> pair(name, var);
    m_int_map_variables.insert(pair);
    return var;
  }
  return iter->second;
}


/*
 * Return a list of all variables.
 * @return a vector of all variables.
 */
vector<BaseVariable*> ExportMap::AllVariables() const {
  vector<BaseVariable*> variables;
  map<string, IntegerVariable*>::const_iterator int_iter;
  map<string, StringVariable*>::const_iterator string_iter;
  map<string, StringMap*>::const_iterator str_map_iter;
  map<string, IntMap*>::const_iterator int_map_iter;

  for (int_iter = m_int_variables.begin();
       int_iter != m_int_variables.end(); int_iter++)
    variables.push_back(int_iter->second);

  for (string_iter = m_string_variables.begin();
       string_iter != m_string_variables.end(); string_iter++)
    variables.push_back(string_iter->second);

  for (str_map_iter = m_str_map_variables.begin();
       str_map_iter != m_str_map_variables.end(); str_map_iter++)
    variables.push_back(str_map_iter->second);

  for (int_map_iter = m_int_map_variables.begin();
       int_map_iter != m_int_map_variables.end(); int_map_iter++)
    variables.push_back(int_map_iter->second);

  return variables;
}
}  // ola
