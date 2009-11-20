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
 * ExportMap.h
 * Interface the ExportMap and ExportedVariables
 * Copyright (C) 2005-2008 Simon Newton
 */

#ifndef INCLUDE_OLA_EXPORTMAP_H_
#define INCLUDE_OLA_EXPORTMAP_H_

#include <stdlib.h>
#include <string>
#include <sstream>
#include <map>
#include <vector>

namespace ola {

using std::string;
using std::stringstream;
using std::map;
using std::vector;


/*
 * Base Variable
 */
class BaseVariable {
  public:
    explicit BaseVariable(const string &name): m_name(name) {}
    virtual ~BaseVariable() {}

    const string Name() const { return m_name; }
    virtual const string Value() const = 0;

  private:
    string m_name;
};


/*
 * Represents a string variable
 */
class StringVariable: public BaseVariable {
  public:
    explicit StringVariable(const string &name)
        : BaseVariable(name),
          m_value("") {}
    ~StringVariable() {}

    void Set(const string &value) { m_value = value; }
    const string Get() const { return m_value; }
    const string Value() const { return m_value; }

  private:
    string m_value;
};


/*
 * Represents a integer variable
 */
class IntegerVariable: public BaseVariable {
  public:
    explicit IntegerVariable(const string &name)
        : BaseVariable(name),
          m_value(0) {}
    ~IntegerVariable() {}

    void Set(int value) { m_value = value; }
    void Increment() { m_value++; }
    void Decrement() { m_value--; }
    void Reset() { m_value = 0; }
    int Get() const { return m_value; }
    const string Value() const;

  private:
    int m_value;
};


/*
 * A Map variable holds string -> string mappings
 */
template<typename Type>
class MapVariable: public BaseVariable {
  public:
    MapVariable(const string &name, const string &label):
      BaseVariable(name),
      m_label(label) {}
    ~MapVariable() {}

    void Set(const string &key, const Type &value);
    void Remove(const string &key);
    const Type Get(const string &key) const;
    const string Value() const;
    const string Label() const { return m_label; }
  private:
    map<string, Type> m_variables;
    string m_label;
};

typedef MapVariable<string> StringMap;
typedef MapVariable<int> IntMap;


/*
 * Set a map variable
 * @param key the key
 * @param value the value
 */
template<typename Type>
void MapVariable<Type>::Set(const string &key, const Type &value) {
  typename map<string, Type>::iterator iter;
  iter = m_variables.find(key);

  if (iter != m_variables.end())
    iter->second = value;

  std::pair<string, Type> pair(key, value);
  m_variables.insert(pair);
}


/*
 * Return a value from the Map Variable
 */
template<typename Type>
const Type MapVariable<Type>::Get(const string &key) const {
  static Type default_value;
  typename map<string, Type>::const_iterator iter;
  iter = m_variables.find(key);

  if (iter == m_variables.end()) {
    return default_value;
  }
  return iter->second;
}


/*
 * Remove a value from the map
 * @param key the key to remove
 */
template<typename Type>
void MapVariable<Type>::Remove(const string &key) {
  typename map<string, Type>::iterator iter;
  iter = m_variables.find(key);

  if (iter != m_variables.end())
    m_variables.erase(iter);
}


/*
 * Holds all the exported variables
 */
class ExportMap {
  public:
    ExportMap() {}
    ~ExportMap();

    IntegerVariable *GetIntegerVar(const string &name);
    StringVariable *GetStringVar(const string &name);
    StringMap *GetStringMapVar(const string &name, const string &label="");
    IntMap *GetIntMapVar(const string &name, const string &label="");
    vector<BaseVariable*> AllVariables() const;

  private :
    ExportMap(const ExportMap&);
    ExportMap& operator=(const ExportMap&);

    map<string, StringVariable*> m_string_variables;
    map<string, IntegerVariable*> m_int_variables;
    map<string, StringMap*> m_str_map_variables;
    map<string, IntMap*> m_int_map_variables;
};
}  // ola
#endif  // INCLUDE_OLA_EXPORTMAP_H_
