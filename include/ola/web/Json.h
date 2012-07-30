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
 * Json.h
 * A simple set of classes for generating JSON.
 * See http://www.json.org/
 * Copyright (C) 2012 Simon Newton
 */

#ifndef INCLUDE_OLA_WEB_JSON_H_
#define INCLUDE_OLA_WEB_JSON_H_

#include <ola/StringUtils.h>
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
 * The base class for json values
 */
class JsonValue {
  public:
    virtual ~JsonValue() {}
    virtual void ToString(ostream *output, unsigned int indent) const = 0;

  protected:
    void Indent(ostream *output, unsigned int indent) const {
      *output << string(indent, ' ');
    }

    static const unsigned int INDENT = 2;
};


/**
 * A string value.
 */
class JsonStringValue: public JsonValue {
  public:
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
 * A unsigned int value.
 */
class JsonUIntValue: public JsonValue {
  public:
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
 * A signed int value.
 */
class JsonIntValue: public JsonValue {
  public:
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
 * A Bool value
 */
class JsonBoolValue: public JsonValue {
  public:
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
 * The null value
 */
class JsonNullValue: public JsonValue {
  public:
    explicit JsonNullValue() {}

    void ToString(ostream *output, unsigned int) const {
      *output << "null";
    }
};


/**
 * A raw value, useful if you want to cheat.
 */
class JsonRawValue: public JsonValue {
  public:
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
 * A Json object.
 * Since key names tend to reuse the same strings, it would be nice to intern
 * the strings here. That's a future optimization for someone.
 */
class JsonObject: public JsonValue {
  public:
    JsonObject() {}
    ~JsonObject();

    // if the same value is added twice we'll override it
    void Add(const string &key, const string &value);
    void Add(const string &key, const char *value);
    void Add(const string &key, unsigned int i);
    void Add(const string &key, int i);
    void Add(const string &key, bool value);
    void Add(const string &key);

    JsonObject* AddObject(const string &key);
    class JsonArray* AddArray(const string &key);

    void AddRaw(const string &key, const string &value);

    void ToString(ostream *output, unsigned int indent) const;

  private:
    typedef map<string, JsonValue*> MemberMap;
    MemberMap m_members;

    JsonObject(const JsonObject&);
    JsonObject& operator=(const JsonObject&);

    inline void FreeIfExists(const string &key) const;
};


/**
 * An array of Json values.
 * Rather than adding JsonValue objects directly (and creating circular
 * references, dealing with memory management etc.) we provide helper methods.
 */
class JsonArray: public JsonValue {
  public:
    JsonArray()
        : m_complex_type(false) {
    }
    ~JsonArray();

    void Append(const string &value) {
      m_values.push_back(new JsonStringValue(value));
    }

    void Append(const char *value) {
      m_values.push_back(new JsonStringValue(value));
    }

    void Append(unsigned int i) {
      m_values.push_back(new JsonUIntValue(i));
    }

    void Append(int i) {
      m_values.push_back(new JsonIntValue(i));
    }

    void Append(bool value) {
      m_values.push_back(new JsonBoolValue(value));
    }

    void Append() {
      m_values.push_back(new JsonNullValue());
    }

    JsonObject* AppendObject() {
      JsonObject *obj = new JsonObject();
      m_values.push_back(obj);
      m_complex_type = true;
      return obj;
    }

    JsonArray* AppendArray() {
      JsonArray *array = new JsonArray();
      m_values.push_back(array);
      m_complex_type = true;
      return array;
    }

    void AppendRaw(const string &value) {
      m_values.push_back(new JsonRawValue(value));
    }

    void ToString(ostream *output, unsigned int indent) const;

  private:
    typedef vector<JsonValue*> ValuesVector;
    ValuesVector m_values;
    // true if this array contains a nested object or array
    bool m_complex_type;

    JsonArray(const JsonArray&);
    JsonArray& operator=(const JsonArray&);
};


class JsonWriter {
  public:
    static void Write(ostream *output, const JsonValue &obj);
    static string AsString(const JsonValue &obj);
};
}  // web
}  // ola
#endif  // INCLUDE_OLA_WEB_JSON_H_
