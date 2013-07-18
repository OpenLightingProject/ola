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
 * ExportMap.h
 * Interface the ExportMap and ExportedVariables
 * Copyright (C) 2005-2008 Simon Newton
 */

/**
 * @file ExportMap.h
 * @brief Export variables on the http server.
 *
 * Exported variables can be used to expose the internal state on the /debug
 * page of the webserver. This allows real time debugging and monitoring of the
 * applications.
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

/**
 * @class BaseVariable <ola/ExportMap.h>
 * @brief The base variable class.
 *
 * All other exported variables derive from this.
 */
class BaseVariable {
  public:
    /**
     * @brief Create a new BaseVariable.
     * @param name the variable name.
     */
    explicit BaseVariable(const string &name): m_name(name) {}

    /**
     * The Destructor.
     */
    virtual ~BaseVariable() {}

    /**
     * @brief Return the name of this variable.
     * @returns the variable name.
     */
    const string Name() const { return m_name; }

    /**
     * @brief Return the value of the variable as a string.
     * @returns the value of the variable.
     */
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


/**
 * @class BoolVariable <ola/ExportMap.h>
 * @brief A boolean variable.
 */
class BoolVariable: public BaseVariable {
  public:
    /**
     * @brief Create a new BoolVariable.
     * @param name the variable name.
     */
    explicit BoolVariable(const string &name)
        : BaseVariable(name),
          m_value(false) {}
    ~BoolVariable() {}

    /**
     * @brief Set the value of the variable.
     * @param value the new value.
     */
    void Set(bool value) { m_value = value; }

    /**
     * @brief Get the value of this variable.
     * @return the value of the boolean variable.
     */
    bool Get() const { return m_value; }

    /**
     * @brief Get the value of this variable as a string.
     * @return the value of the boolean variable.
     *
     * Booleans are represented by a 1 or 0.
     */
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
    void Set(const string &key, Type value);
    Type &operator[](const string &key);
    const string Value() const;
    const string Label() const { return m_label; }

  protected:
    map<string, Type> m_variables;

  private:
    string m_label;
};

typedef MapVariable<string> StringMap;


/**
 * An map of integer values. This provides an increment operation.
 */
class IntMap: public MapVariable<int> {
  public:
    IntMap(const string &name, const string &label)
        : MapVariable<int>(name, label) {
    }

    void Increment(const string &key) {
      m_variables[key]++;
    }
};


/**
 * An IntMap. This provides an increment operation.
 */
class UIntMap: public MapVariable<unsigned int> {
  public:
    UIntMap(const string &name, const string &label)
        : MapVariable<unsigned int>(name, label) {
    }

    void Increment(const string &key) {
      m_variables[key]++;
    }
};


/*
 * Return a value from the Map Variable, this will create an entry in the map
 * if the variable doesn't exist.
 */
template<typename Type>
Type &MapVariable<Type>::operator[](const string &key) {
  return m_variables[key];
}


/*
 * Set a value in the Map variable.
 */
template<typename Type>
void MapVariable<Type>::Set(const string &key, Type value) {
  m_variables[key] = value;
}


/**
 * Remove a value from the map
 * @param key the key to remove
 */
template<typename Type>
void MapVariable<Type>::Remove(const string &key) {
  typename map<string, Type>::iterator iter = m_variables.find(key);

  if (iter != m_variables.end())
    m_variables.erase(iter);
}


/**
 * @brief A container for the exported variables.
 *
 */
class ExportMap {
  public:
    ExportMap() {}
    ~ExportMap();

    /**
     * @brief Lookup or create a BoolVariable.
     * @param name the name of this variable.
     * @return a pointer to the BoolVariable.
     *
     * The variable is created if it doesn't already exist. The pointer is
     * valid for the lifetime of the ExportMap.
     */
    BoolVariable *GetBoolVar(const string &name);

    /**
     * @brief Lookup or create an IntegerVariable.
     * @param name the name of this variable.
     * @return an IntegerVariable.
     *
     * The variable is created if it doesn't already exist. The pointer is
     * valid for the lifetime of the ExportMap.
     */
    IntegerVariable *GetIntegerVar(const string &name);

    /**
     * @brief Lookup or create a CounterVariable.
     * @param name the name of this variable.
     * @return a CounterVariable.
     *
     * The variable is created if it doesn't already exist. The pointer is
     * valid for the lifetime of the ExportMap.
     */
    CounterVariable *GetCounterVar(const string &name);

    /**
     * @brief Lookup or create a StringVariable.
     * @param name the name of this variable.
     * @return a StringVariable.
     *
     * The variable is created if it doesn't already exist. The pointer is
     * valid for the lifetime of the ExportMap.
     */
    StringVariable *GetStringVar(const string &name);

    StringMap *GetStringMapVar(const string &name, const string &label="");
    IntMap *GetIntMapVar(const string &name, const string &label="");
    UIntMap *GetUIntMapVar(const string &name, const string &label="");

    /**
     * @brief Fetch a list of all known variables.
     * @returns a vector of all variables.
     */
    vector<BaseVariable*> AllVariables() const;

  private :
    ExportMap(const ExportMap&);
    ExportMap& operator=(const ExportMap&);

    template<typename Type>
    Type *GetVar(map<string, Type*> *var_map, const string &name);

    template<typename Type>
    Type *GetMapVar(map<string, Type*> *var_map,
                    const string &name,
                    const string &label);

    map<string, BoolVariable*> m_bool_variables;
    map<string, CounterVariable*> m_counter_variables;
    map<string, IntegerVariable*> m_int_variables;
    map<string, StringVariable*> m_string_variables;

    map<string, StringMap*> m_str_map_variables;
    map<string, IntMap*> m_int_map_variables;
    map<string, UIntMap*> m_uint_map_variables;
};
}  // namespace ola
#endif  // INCLUDE_OLA_EXPORTMAP_H_
