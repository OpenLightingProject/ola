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
 * Json.h
 * A simple set of classes for generating JSON.
 * See http://www.json.org/
 * Copyright (C) 2012 Simon Newton
 */

/**
 * @defgroup json JSON
 * @brief A JSON formatter.
 *
 * @examplepara
 * ~~~~~~~~~~~~~~~~~~~~~
 * #include <ola/web/Json.h>
 *
 * JsonObject obj;
 * obj.Add("name", "simon");
 * obj.Add("age", 10);
 * obj.Add("male", true);
 * JsonArray *friends = obj.AddArray("friends");
 * friends->Add("Peter");
 * friends->Add("Bob");
 * friends->Add("Jane");
 * const string output = JsonWriter::AsString(json);
 * ~~~~~~~~~~~~~~~~~~~~~
 *
 * @addtogroup json
 * @{
 * @file Json.h
 * @brief Header file for the JSON formatter.
 * @}
 */

#ifndef INCLUDE_OLA_WEB_JSON_H_
#define INCLUDE_OLA_WEB_JSON_H_

#include <ola/StringUtils.h>
#include <ola/base/Macro.h>
#include <stdint.h>
#include <map>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

namespace ola {
namespace web {

/**
 * @addtogroup json
 * @{
 */

class JsonValueVisitorInterface;
class JsonObjectPropertyVisitor;

/**
 * @brief The base class for JSON values.
 */
class JsonValue {
 public:
  virtual ~JsonValue() {}

  /**
   * @brief The Accept method for the visitor pattern.
   * This can be used to traverse the Json Tree in a type-safe manner.
   */
  virtual void Accept(JsonValueVisitorInterface *visitor) const = 0;
};


/**
 * @brief A string value.
 */
class JsonStringValue: public JsonValue {
 public:
  /**
   * @brief Create a new JsonStringValue
   * @param value the string to use.
   */
  explicit JsonStringValue(const std::string &value)
      : m_value(value) {
  }

  const std::string& Value() const { return m_value; }

  void Accept(JsonValueVisitorInterface *visitor) const;

 private:
  const std::string m_value;

  DISALLOW_COPY_AND_ASSIGN(JsonStringValue);
};


/**
 * @brief An unsigned int value.
 */
class JsonUIntValue: public JsonValue {
 public:
  /**
   * @brief Create a new JsonUIntValue
   * @param value the unsigned int to use.
   */
  explicit JsonUIntValue(unsigned int value)
      : m_value(value) {
  }

  void Accept(JsonValueVisitorInterface *visitor) const;

  unsigned int Value() const { return m_value; }

 private:
  const unsigned int m_value;

  DISALLOW_COPY_AND_ASSIGN(JsonUIntValue);
};


/**
 * @brief A signed int value.
 */
class JsonIntValue: public JsonValue {
 public:
  /**
   * @brief Create a new JsonIntValue
   * @param value the int to use.
   */
  explicit JsonIntValue(int value)
      : m_value(value) {
  }

  void Accept(JsonValueVisitorInterface *visitor) const;

  int Value() const { return m_value; }

 private:
  const int m_value;

  DISALLOW_COPY_AND_ASSIGN(JsonIntValue);
};

/**
 * @brief An unsigned int 64 value.
 */
class JsonUInt64Value: public JsonValue {
 public:
  /**
   * @brief Create a new JsonUInt64Value
   * @param value the unsigned int 64 to use.
   */
  explicit JsonUInt64Value(uint64_t value)
      : m_value(value) {
  }

  void Accept(JsonValueVisitorInterface *visitor) const;

  uint64_t Value() const { return m_value; }

 private:
  const uint64_t m_value;

  DISALLOW_COPY_AND_ASSIGN(JsonUInt64Value);
};


/**
 * @brief A signed int 64 value.
 */
class JsonInt64Value: public JsonValue {
 public:
  /**
   * @brief Create a new JsonInt64Value
   * @param value the int 64 to use.
   */
  explicit JsonInt64Value(int64_t value)
      : m_value(value) {
  }

  void Accept(JsonValueVisitorInterface *visitor) const;

  int64_t Value() const { return m_value; }

 private:
  const int64_t m_value;

  DISALLOW_COPY_AND_ASSIGN(JsonInt64Value);
};

/**
 * @brief A double value.
 */
class JsonDoubleValue: public JsonValue {
 public:
  /**
   * @brief Create a new JsonDoubleValue
   * @param value the double to use.
   */
  explicit JsonDoubleValue(long double value)
      : m_value(value) {
  }

  void Accept(JsonValueVisitorInterface *visitor) const;

  long double Value() const { return m_value; }

 private:
  const long double m_value;

  DISALLOW_COPY_AND_ASSIGN(JsonDoubleValue);
};


/**
 * @brief A Bool value
 */
class JsonBoolValue: public JsonValue {
 public:
  /**
   * @brief Create a new JsonBoolValue
   * @param value the bool to use.
   */
  explicit JsonBoolValue(bool value)
      : m_value(value) {
  }

  void Accept(JsonValueVisitorInterface *visitor) const;

  bool Value() const { return m_value; }

 private:
  const bool m_value;

  DISALLOW_COPY_AND_ASSIGN(JsonBoolValue);
};


/**
 * @brief The null value
 */
class JsonNullValue: public JsonValue {
 public:
    /**
     * @brief Create a new JsonNullValue
     */
    explicit JsonNullValue() {}

    void Accept(JsonValueVisitorInterface *visitor) const;

 private:
  DISALLOW_COPY_AND_ASSIGN(JsonNullValue);
};


/**
 * @brief A raw value, useful if you want to cheat.
 */
class JsonRawValue: public JsonValue {
 public:
  /*
   * @brief Create a new JsonRawValue
   * @param value the raw data to insert.
   */
  explicit JsonRawValue(const std::string &value)
    : m_value(value) {
  }

  void Accept(JsonValueVisitorInterface *visitor) const;

  const std::string& Value() const { return m_value; }

 private:
  const std::string m_value;

  DISALLOW_COPY_AND_ASSIGN(JsonRawValue);
};


/**
 * @brief A JSON object.
 * JSON Objects are key : value mappings, similar to dictionaries in Python.
 *
 * If the same key is added more than once, the latest value wins.
 *
 * @todo Since key names tend to reuse the same strings, it would be nice to
 * intern the strings here. That's a future optimization for someone.
 */
class JsonObject: public JsonValue {
 public:
  /**
   * @brief Create a new JsonObject
   */
  JsonObject() {}
  ~JsonObject();

  /**
   * @brief Add a key to string mapping.
   * @param key the key to set.
   * @param value the value to add.
   */
  void Add(const std::string &key, const std::string &value);

  /**
   * @brief Set the given key to a string value.
   * @param key the key to set.
   * @param value the value to add
   */
  void Add(const std::string &key, const char *value);

  /**
   * @brief Set the given key to a unsigned int value.
   * @param key the key to set.
   * @param i the value to add
   */
  void Add(const std::string &key, unsigned int i);

  /**
   * @brief Set the given key to a int value.
   * @param key the key to set.
   * @param i the value to add
   */
  void Add(const std::string &key, int i);

  /**
   * @brief Set the given key to a bool value.
   * @param key the key to set.
   * @param value the value to add
   */
  void Add(const std::string &key, bool value);

  /**
   * @brief Set the given key to a null value.
   * @param key the key to set.
   */
  void Add(const std::string &key);

  /**
   * @brief Set the given key to a JsonObject.
   * @param key the key to add
   * @returns the new JsonObject.
   */
  JsonObject* AddObject(const std::string &key);

  /**
   * @brief Set the given key to a JsonArray.
   * @param key the key to add
   * @returns the new JsonObject.
   */
  class JsonArray* AddArray(const std::string &key);

  /**
   * @brief Set the key to the supplied JsonValue.
   * @param key the key to add
   * @param value the JsonValue object, ownership is transferred.
   */
  void AddValue(const std::string &key, const JsonValue *value);

  /**
   * @brief Set the given key to a raw value.
   * @param key the key to add
   * @param value the raw value to append.
   */
  void AddRaw(const std::string &key, const std::string &value);

  void Accept(JsonValueVisitorInterface *visitor) const;

  bool IsEmpty() const { return m_members.empty(); }

  void VisitProperties(JsonObjectPropertyVisitor *visitor) const;

 private:
  typedef std::map<std::string, const JsonValue*> MemberMap;
  MemberMap m_members;

  DISALLOW_COPY_AND_ASSIGN(JsonObject);
};


/**
 * @brief An array of JSON values.
 * Arrays in JSON can contain values of different types.
 */
class JsonArray: public JsonValue {
 public:
  JsonArray() : m_complex_type(false) {}
  ~JsonArray();

  /**
   * @brief Append a string value to the array
   * @param value the value to append
   */
  void Append(const std::string &value) {
    m_values.push_back(new JsonStringValue(value));
  }

  /**
   * @brief Append a string value to the array
   * @param value the value to append
   */
  void Append(const char *value) {
    m_values.push_back(new JsonStringValue(value));
  }

  /**
   * @brief Append an unsigned int value to the array
   * @param i the value to append
   */
  void Append(unsigned int i) {
    m_values.push_back(new JsonUIntValue(i));
  }

  /**
   * @brief Append an int value to the array
   * @param i the value to append
   */
  void Append(int i) {
    m_values.push_back(new JsonIntValue(i));
  }

  /**
   * @brief Append a bool value to the array
   * @param value the value to append
   */
  void Append(bool value) {
    m_values.push_back(new JsonBoolValue(value));
  }

  /**
   * @brief Append a null value to the array
   */
  void Append() {
    m_values.push_back(new JsonNullValue());
  }

  /**
   * @brief Append a JsonValue. Takes ownership of the pointer.
   */
  void Append(const JsonValue *value) {
    m_values.push_back(value);
  }

  /**
   * @brief Append a JsonObject to the array
   * @returns the new JsonObject. Ownership is not transferred and the
   * pointer is valid for the lifetime of this JsonArray.
   */
  JsonObject* AppendObject() {
    JsonObject *obj = new JsonObject();
    m_values.push_back(obj);
    m_complex_type = true;
    return obj;
  }

  /**
   * @brief Append a JsonArray to the array
   * @returns the new JsonArray. Ownership is not transferred and the
   * pointer is valid for the lifetime of this JsonArray.
   */
  JsonArray* AppendArray() {
    JsonArray *array = new JsonArray();
    m_values.push_back(array);
    m_complex_type = true;
    return array;
  }

  /**
   * @brief Append a raw value to the array
   */
  void AppendRaw(const std::string &value) {
    m_values.push_back(new JsonRawValue(value));
  }

  void Accept(JsonValueVisitorInterface *visitor) const;

  unsigned int Size() const { return m_values.size(); }
  const JsonValue *ElementAt(unsigned int i) const;

  bool IsComplexType() const { return m_complex_type; }

 private:
  typedef std::vector<const JsonValue*> ValuesVector;
  ValuesVector m_values;

  // true if this array contains a nested object or array
  bool m_complex_type;

  DISALLOW_COPY_AND_ASSIGN(JsonArray);
};


class JsonObjectPropertyVisitor {
 public:
  virtual ~JsonObjectPropertyVisitor() {}

  virtual void VisitProperty(const std::string &property,
                             const JsonValue &value) = 0;
};

/**
 * @brief The interface for the JsonValueVisitor class.
 */
class JsonValueVisitorInterface {
 public:
  virtual ~JsonValueVisitorInterface() {}

  virtual void Visit(const JsonStringValue &value) = 0;
  virtual void Visit(const JsonBoolValue &value) = 0;
  virtual void Visit(const JsonNullValue &value) = 0;
  virtual void Visit(const JsonRawValue &value) = 0;
  virtual void Visit(const JsonObject &value) = 0;
  virtual void Visit(const JsonArray &value) = 0;
  virtual void Visit(const JsonUIntValue &value) = 0;
  virtual void Visit(const JsonUInt64Value &value) = 0;
  virtual void Visit(const JsonIntValue &value) = 0;
  virtual void Visit(const JsonInt64Value &value) = 0;
  virtual void Visit(const JsonDoubleValue &value) = 0;
};


/**
 * @brief A class that writes a JsonValue to an output stream.
 */
class JsonWriter : public JsonValueVisitorInterface, JsonObjectPropertyVisitor {
 public:
  explicit JsonWriter(std::ostream *output)
      : m_output(output),
        m_indent(0),
        m_separator("") {
  }

  /**
   * @brief Write the string representation of the JsonValue to a ostream.
   * @param output the ostream to write to
   * @param value the JsonValue to serialize.
   */
  static void Write(std::ostream *output, const JsonValue &value);

  /**
   * @brief Get the string representation of the JsonValue.
   * @param value the JsonValue to serialize.
   */
  static std::string AsString(const JsonValue &value);

  void Visit(const JsonStringValue &value);
  void Visit(const JsonBoolValue &value);
  void Visit(const JsonNullValue &value);
  void Visit(const JsonRawValue &value);
  void Visit(const JsonObject &value);
  void Visit(const JsonArray &value);
  void Visit(const JsonUIntValue &value);
  void Visit(const JsonUInt64Value &value);
  void Visit(const JsonIntValue &value);
  void Visit(const JsonInt64Value &value);
  void Visit(const JsonDoubleValue &value);

  void VisitProperty(const std::string &property, const JsonValue &value);

 private:
  std::ostream *m_output;
  unsigned int m_indent;
  std::string m_separator;

  /**
   * @brief the default indent level
   */
  static const unsigned int DEFAULT_INDENT = 2;
};
/**@}*/
}  // namespace web
}  // namespace ola
#endif  // INCLUDE_OLA_WEB_JSON_H_
