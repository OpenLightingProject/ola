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
#include <functional>
#include <map>
#include <sstream>
#include <string>
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

struct VariableLessThan: public std::binary_function<BaseVariable*,
                                                     BaseVariable*, bool> {
  bool operator()(BaseVariable *x, BaseVariable *y) {
    return x->Name() < y->Name();
  }
};



/*
 * Represents a bool variable
 */
class BoolVariable: public BaseVariable {
  public:
    explicit BoolVariable(const string &name)
        : BaseVariable(name),
          m_value(false) {}
    ~BoolVariable() {}

    void Set(bool value) { m_value = value; }
    bool Get() const { return m_value; }
    const string Value() const { return m_value ? "1" : "0"; }

  private:
    bool m_value;
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
    void operator++(int) { m_value++; }
    void operator--(int) { m_value--; }
    void Reset() { m_value = 0; }
    int Get() const { return m_value; }
    const string Value() const {
      stringstream out;
      out << m_value;
      return out.str();
    }

  private:
    int m_value;
};


/*
 * Represents a counter which can only be added to.
 */
class CounterVariable: public BaseVariable {
  public:
    explicit CounterVariable(const string &name)
        : BaseVariable(name),
          m_value(0) {}
    ~CounterVariable() {}

    void operator++(int) { m_value++; }
    void operator+=(unsigned int value) { m_value += value; }
    void Reset() { m_value = 0; }
    unsigned int Get() const { return m_value; }
    const string Value() const {
      stringstream out;
      out << m_value;
      return out.str();
    }

  private:
    unsigned int m_value;
};


/*
 * A Map variable holds string -> type mappings
 */
template<typename Type>
class MapVariable: public BaseVariable {
  public:
    MapVariable(const string &name, const string &label):
      BaseVariable(name),
      m_label(label) {}
    ~MapVariable() {}

    void Remove(const string &key);
    Type &operator[](const string &key);
    const string Value() const;
    const string Label() const { return m_label; }
  private:
    map<string, Type> m_variables;
    string m_label;
};

typedef MapVariable<string> StringMap;
typedef MapVariable<int> IntMap;
typedef MapVariable<unsigned int> UIntMap;


/*
 * Return a value from the Map Variable, this will create an entry in the map
 * if the variable doesn't exist.
 */
template<typename Type>
Type &MapVariable<Type>::operator[](const string &key) {
  return m_variables[key];
}


/*
 * Remove a value from the map
 * @param key the key to remove
 */
template<typename Type>
void MapVariable<Type>::Remove(const string &key) {
  typename map<string, Type>::iterator iter = m_variables.find(key);

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
    vector<BaseVariable*> AllVariables() const;

    BoolVariable *GetBoolVar(const string &name);
    IntegerVariable *GetIntegerVar(const string &name);
    CounterVariable *GetCounterVar(const string &name);
    StringVariable *GetStringVar(const string &name);

    StringMap *GetStringMapVar(const string &name, const string &label="");
    IntMap *GetIntMapVar(const string &name, const string &label="");
    UIntMap *GetUIntMapVar(const string &name, const string &label="");

  private :
    ExportMap(const ExportMap&);
    ExportMap& operator=(const ExportMap&);

    template<typename Type>
    Type *GetVar(map<string, Type*> *var_map, const string &name);

    template<typename Type>
    Type *GetMapVar(map<string, Type*> *var_map,
                    const string &name,
                    const string &label);

    template<typename Type>
    void AddVariablesToVector(vector<BaseVariable*> *variables,
                              const Type &var_map) const;

    template<typename Type>
    void DeleteVariables(Type *var_map) const;

    map<string, BoolVariable*> m_bool_variables;
    map<string, CounterVariable*> m_counter_variables;
    map<string, IntegerVariable*> m_int_variables;
    map<string, StringVariable*> m_string_variables;

    map<string, StringMap*> m_str_map_variables;
    map<string, IntMap*> m_int_map_variables;
    map<string, UIntMap*> m_uint_map_variables;
};
}  // ola
#endif  // INCLUDE_OLA_EXPORTMAP_H_
