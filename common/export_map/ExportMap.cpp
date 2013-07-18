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
 * ExportMap.cpp
 * Exported Variables
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <algorithm>
#include <string>
#include <map>
#include <vector>
#include <iostream>
#include "ola/ExportMap.h"
#include "ola/StringUtils.h"
#include "ola/stl/STLUtils.h"

namespace ola {

using std::string;
using std::stringstream;


/*
 * Return the string representation of this map variable.
 * The form is:
 *   var_name  map:label_name key1:value1 key2:value2
 * @return the string representation of the variable.
 */
template<typename Type>
const string MapVariable<Type>::Value() const {
  stringstream value;
  value << "map:" << m_label;
  typename map<string, Type>::const_iterator iter;
  for (iter = m_variables.begin(); iter != m_variables.end(); ++iter)
    value << " " << iter->first << ":" << iter->second;
  return value.str();
}


/*
 * Strings need to be quoted
 */
template<>
const string MapVariable<string>::Value() const {
  stringstream value;
  value << "map:" << m_label;
  map<string, string>::const_iterator iter;
  for (iter = m_variables.begin(); iter != m_variables.end(); ++iter) {
    std::string var = iter->second;
    Escape(&var);
    value << " " << iter->first << ":\"" << var << "\"";
  }
  return value.str();
}


ExportMap::~ExportMap() {
  STLDeleteValues(&m_bool_variables);
  STLDeleteValues(&m_counter_variables);
  STLDeleteValues(&m_int_map_variables);
  STLDeleteValues(&m_int_variables);
  STLDeleteValues(&m_str_map_variables);
  STLDeleteValues(&m_string_variables);
  STLDeleteValues(&m_uint_map_variables);
}

BoolVariable *ExportMap::GetBoolVar(const string &name) {
  return GetVar(&m_bool_variables, name);
}

IntegerVariable *ExportMap::GetIntegerVar(const string &name) {
  return GetVar(&m_int_variables, name);
}

CounterVariable *ExportMap::GetCounterVar(const string &name) {
  return GetVar(&m_counter_variables, name);
}

StringVariable *ExportMap::GetStringVar(const string &name) {
  return GetVar(&m_string_variables, name);
}


/*
 * Lookup or create a string map variable
 * @param name the name of the variable
 * @param label the label to use for the map (optional)
 * @return a MapVariable
 */
StringMap *ExportMap::GetStringMapVar(const string &name, const string &label) {
  return GetMapVar(&m_str_map_variables, name, label);
}


/*
 * Lookup or create an int map variable
 * @param name the name of the variable
 * @param label the label to use for the map (optional)
 * @return a MapVariable
 */
IntMap *ExportMap::GetIntMapVar(const string &name, const string &label) {
  return GetMapVar(&m_int_map_variables, name, label);
}


/*
 * Lookup or create an unsigned int map variable
 * @param name the name of the variable
 * @param label the label to use for the map (optional)
 * @return a MapVariable
 */
UIntMap *ExportMap::GetUIntMapVar(const string &name, const string &label) {
  return GetMapVar(&m_uint_map_variables, name, label);
}


/*
 * Return a list of all variables.
 * @return a vector of all variables.
 */
vector<BaseVariable*> ExportMap::AllVariables() const {
  vector<BaseVariable*> variables;
  STLValues(m_bool_variables, &variables);
  STLValues(m_counter_variables, &variables);
  STLValues(m_int_map_variables, &variables);
  STLValues(m_int_variables, &variables);
  STLValues(m_str_map_variables, &variables);
  STLValues(m_string_variables, &variables);
  STLValues(m_uint_map_variables, &variables);

  sort(variables.begin(), variables.end(), VariableLessThan());
  return variables;
}


template<typename Type>
Type *ExportMap::GetVar(map<string, Type*> *var_map, const string &name) {
  typename map<string, Type*>::iterator iter;
  iter = var_map->find(name);

  if (iter == var_map->end()) {
    Type *var = new Type(name);
    (*var_map)[name] = var;
    return var;
  }
  return iter->second;
}


template<typename Type>
Type *ExportMap::GetMapVar(map<string, Type*> *var_map,
                           const string &name,
                           const string &label) {
  typename map<string, Type*>::iterator iter;
  iter = var_map->find(name);

  if (iter == var_map->end()) {
    Type *var = new Type(name, label);
    (*var_map)[name] = var;
    return var;
  }
  return iter->second;
}
}  // namespace ola
