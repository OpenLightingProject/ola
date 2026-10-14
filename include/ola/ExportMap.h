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
 * ExportMap.h
 * Interface the ExportMap and ExportedVariables
 * Copyright (C) 2005 Simon Newton
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

#include <ola/base/Macro.h>
#include <ola/StringUtils.h>
#include <stdlib.h>

#include <functional>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace ola {

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
  explicit BaseVariable(const std::string &name): m_name(name) {}

  /**
   * The Destructor.
   */
  virtual ~BaseVariable() {}

  /**
   * @brief Return the name of this variable.
   * @returns the variable name.
   */
  const std::string Name() const { return m_name; }

  /**
   * @brief Return the value of the variable as a string.
   * @returns the value of the variable.
   */
  virtual const std::string Value() const = 0;

 private:
  std::string m_name;
};

struct VariableLessThan {
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
  explicit BoolVariable(const std::string &name)
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
  const std::string Value() const { return m_value ? "1" : "0"; }

 private:
  bool m_value;
};


/*
 * Represents a string variable
 */
class StringVariable: public BaseVariable {
 public:
  explicit StringVariable(const std::string &name)
      : BaseVariable(name),
        m_value("") {}
  ~StringVariable() {}

  void Set(const std::string &value) { m_value = value; }
  const std::string Get() const { return m_value; }
  const std::string Value() const { return m_value; }

 private:
  std::string m_value;
};


/*
 * Represents a integer variable
 */
class IntegerVariable: public BaseVariable {
 public:
  explicit IntegerVariable(const std::string &name)
      : BaseVariable(name),
        m_value(0) {}
  ~IntegerVariable() {}

  void Set(int value) { m_value = value; }
  void operator++(int) { m_value++; }
  void operator--(int) { m_value--; }
  void Reset() { m_value = 0; }
  int Get() const { return m_value; }
  const std::string Value() const {
    std::ostringstream out;
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
  explicit CounterVariable(const std::string &name)
      : BaseVariable(name),
        m_value(0) {}
  ~CounterVariable() {}

  void operator++(int) { m_value++; }
  void operator+=(unsigned int value) { m_value += value; }
  void Reset() { m_value = 0; }
  unsigned int Get() const { return m_value; }
  const std::string Value() const {
    std::ostringstream out;
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
  MapVariable(const std::string &name, const std::string &label)
      : BaseVariable(name),
        m_label(label) {}
  ~MapVariable() {}

  void Remove(const std::string &key);
  void Set(const std::string &key, Type value);
  Type &operator[](const std::string &key);
  const std::string Value() const;
  const std::string Label() const { return m_label; }

 protected:
  std::map<std::string, Type> m_variables;

 private:
  std::string m_label;
};

typedef MapVariable<std::string> StringMap;


/**
 * An map of integer values. This provides an increment operation.
 */
class IntMap: public MapVariable<int> {
 public:
  IntMap(const std::string &name, const std::string &label)
      : MapVariable<int>(name, label) {}

  void Increment(const std::string &key) {
    m_variables[key]++;
  }
};


/**
 * An IntMap. This provides an increment operation.
 */
class UIntMap: public MapVariable<unsigned int> {
 public:
  UIntMap(const std::string &name, const std::string &label)
      : MapVariable<unsigned int>(name, label) {}

  void Increment(const std::string &key) {
    m_variables[key]++;
  }
};


/*
 * Return a value from the Map Variable, this will create an entry in the map
 * if the variable doesn't exist.
 */
template<typename Type>
Type &MapVariable<Type>::operator[](const std::string &key) {
  return m_variables[key];
}


/*
 * Set a value in the Map variable.
 */
template<typename Type>
void MapVariable<Type>::Set(const std::string &key, Type value) {
  m_variables[key] = value;
}


/**
 * Remove a value from the map
 * @param key the key to remove
 */
template<typename Type>
void MapVariable<Type>::Remove(const std::string &key) {
  typename std::map<std::string, Type>::iterator iter = m_variables.find(key);

  if (iter != m_variables.end())
    m_variables.erase(iter);
}

/*
 * Return the string representation of this map variable.
 * The form is:
 *   var_name  map:label_name key1:value1 key2:value2
 * @return the string representation of the variable.
 */
template<typename Type>
inline const std::string MapVariable<Type>::Value() const {
  std::ostringstream value;
  value << "map:" << m_label;
  typename std::map<std::string, Type>::const_iterator iter;
  for (iter = m_variables.begin(); iter != m_variables.end(); ++iter)
    value << " " << iter->first << ":" << iter->second;
  return value.str();
}


/*
 * Strings need to be quoted
 */
template<>
inline const std::string MapVariable<std::string>::Value() const {
  std::ostringstream value;
  value << "map:" << m_label;
  std::map<std::string, std::string>::const_iterator iter;
  for (iter = m_variables.begin(); iter != m_variables.end(); ++iter) {
    std::string var = iter->second;
    Escape(&var);
    value << " " << iter->first << ":\"" << var << "\"";
  }
  return value.str();
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
  BoolVariable *GetBoolVar(const std::string &name);

  /**
   * @brief Lookup or create an IntegerVariable.
   * @param name the name of this variable.
   * @return an IntegerVariable.
   *
   * The variable is created if it doesn't already exist. The pointer is
   * valid for the lifetime of the ExportMap.
   */
  IntegerVariable *GetIntegerVar(const std::string &name);

  /**
   * @brief Lookup or create a CounterVariable.
   * @param name the name of this variable.
   * @return a CounterVariable.
   *
   * The variable is created if it doesn't already exist. The pointer is
   * valid for the lifetime of the ExportMap.
   */
  CounterVariable *GetCounterVar(const std::string &name);

  /**
   * @brief Lookup or create a StringVariable.
   * @param name the name of this variable.
   * @return a StringVariable.
   *
   * The variable is created if it doesn't already exist. The pointer is
   * valid for the lifetime of the ExportMap.
   */
  StringVariable *GetStringVar(const std::string &name);

  StringMap *GetStringMapVar(const std::string &name,
                             const std::string &label = "");
  IntMap *GetIntMapVar(const std::string &name, const std::string &label = "");
  UIntMap *GetUIntMapVar(const std::string &name,
                         const std::string &label = "");

  /**
   * @brief Fetch a list of all known variables.
   * @returns a vector of all variables.
   */
  std::vector<BaseVariable*> AllVariables() const;

 private :
  template<typename Type>
  Type *GetVar(std::map<std::string, Type*> *var_map,
               const std::string &name);

  template<typename Type>
  Type *GetMapVar(std::map<std::string, Type*> *var_map,
                  const std::string &name,
                  const std::string &label);

  std::map<std::string, BoolVariable*> m_bool_variables;
  std::map<std::string, CounterVariable*> m_counter_variables;
  std::map<std::string, IntegerVariable*> m_int_variables;
  std::map<std::string, StringVariable*> m_string_variables;

  std::map<std::string, StringMap*> m_str_map_variables;
  std::map<std::string, IntMap*> m_int_map_variables;
  std::map<std::string, UIntMap*> m_uint_map_variables;

  DISALLOW_COPY_AND_ASSIGN(ExportMap);
};
}  // namespace ola
#endif  // INCLUDE_OLA_EXPORTMAP_H_
