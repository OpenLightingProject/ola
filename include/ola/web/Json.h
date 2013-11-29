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
#include <map>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

namespace ola {
namespace web {

using std::map;
using std::ostream;
using std::string;
using std::stringstream;
using std::vector;

/**
 * @addtogroup json
 * @{
 */

/**
 * @brief The base class for JSON values.
 */
class JsonValue {
 public:
    virtual ~JsonValue() {}

    /**
     * @brief Output the string representation of this value to an ostream
     * @param output the ostream to output to.
     * @param indent the number of spaces to prepend.
     */
    virtual void ToString(ostream *output, unsigned int indent) const = 0;

 protected:
    /**
     * @brief Append the give number of spaces to the output stream.
     * @param output the ostream to append to
     * @param indent the number of spaces to append
     */
    void Indent(ostream *output, unsigned int indent) const {
      *output << string(indent, ' ');
    }

    /**
     * @brief the default indent level
     */
    static const unsigned int DEFAULT_INDENT = 2;
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
    explicit JsonStringValue(const string &value)
        : m_value(value) {
    }

    void ToString(ostream *output, unsigned int) const {
      *output << '"' << EscapeString(m_value) << '"';
    }

 private:
    const string m_value;
};


/**
 * @brief A unsigned int value.
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

    void ToString(ostream *output, unsigned int) const {
      *output << m_value;
    }

 private:
    const unsigned int m_value;
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

    void ToString(ostream *output, unsigned int) const {
      *output << m_value;
    }

 private:
    const int m_value;
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

    void ToString(ostream *output, unsigned int) const {
      *output << (m_value ? "true" : "false");
    }

 private:
    const bool m_value;
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

    void ToString(ostream *output, unsigned int) const {
      *output << "null";
    }
};


/**
 * @brief A raw value, useful if you want to cheat.
 */
class JsonRawValue: public JsonValue {
 public:
    /**
     * @brief Create a new JsonRawValue
     * @param value the raw data to insert.
     */
    explicit JsonRawValue(const string &value)
      : m_value(value) {
    }

    void ToString(ostream *output, unsigned int) const {
      *output << m_value;
    }

 private:
    const string m_value;
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
    void Add(const string &key, const string &value);

    /**
     * @brief Set the given key to a string value.
     * @param key the key to set.
     * @param value the value to add
     */
    void Add(const string &key, const char *value);

    /**
     * @brief Set the given key to a unsigned int value.
     * @param key the key to set.
     * @param i the value to add
     */
    void Add(const string &key, unsigned int i);

    /**
     * @brief Set the given key to a int value.
     * @param key the key to set.
     * @param i the value to add
     */
    void Add(const string &key, int i);

    /**
     * @brief Set the given key to a bool value.
     * @param key the key to set.
     * @param value the value to add
     */
    void Add(const string &key, bool value);

    /**
     * @brief Set the given key to a null value.
     * @param key the key to set.
     */
    void Add(const string &key);

    /**
     * @brief Set the given key to a JsonObject.
     * @param key the key to add
     * @returns the new JsonObject.
     */
    JsonObject* AddObject(const string &key);

    /**
     * @brief Set the given key to a JsonArray.
     * @param key the key to add
     * @returns the new JsonObject.
     */
    class JsonArray* AddArray(const string &key);

    /**
     * @brief Set the given key to a raw value.
     * @param key the key to add
     * @param value the raw value to append.
     */
    void AddRaw(const string &key, const string &value);

    void ToString(ostream *output, unsigned int indent) const;

 private:
    typedef map<string, JsonValue*> MemberMap;
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
    void Append(const string &value) {
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
    void AppendRaw(const string &value) {
      m_values.push_back(new JsonRawValue(value));
    }

    void ToString(ostream *output, unsigned int indent) const;

 private:
    typedef vector<JsonValue*> ValuesVector;
    ValuesVector m_values;
    // true if this array contains a nested object or array
    bool m_complex_type;

    DISALLOW_COPY_AND_ASSIGN(JsonArray);
};

/**
 * @brief A class that writes a JsonValue to an output stream.
 */
class JsonWriter {
 public:
    /**
     * @brief Write the string representation of the JsonValue to a ostream.
     * @param output the ostream to write to
     * @param value the JsonValue to serialize.
     */
    static void Write(ostream *output, const JsonValue &value);

    /**
     * @brief Get the string representation of the JsonValue.
     * @param value the JsonValue to serialize.
     */
    static string AsString(const JsonValue &value);
};
/**@}*/
}  // namespace web
}  // namespace ola
#endif  // INCLUDE_OLA_WEB_JSON_H_
