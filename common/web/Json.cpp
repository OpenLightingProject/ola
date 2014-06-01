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
 * Json.cpp
 * A simple set of classes for generating JSON.
 * See http://www.json.org/
 * Copyright (C) 2012 Simon Newton
 */

#include <math.h>
#include <string>
#include <limits>
#include "ola/StringUtils.h"
#include "ola/stl/STLUtils.h"
#include "ola/web/Json.h"

namespace ola {
namespace web {

using std::ostream;
using std::ostringstream;
using std::string;

namespace {

class ObjectCastVisitor : public JsonValueVisitorInterface {
 public:
  ObjectCastVisitor() : m_obj(NULL) {}

  void Visit(JsonStringValue *value) { (void) value; }
  void Visit(JsonBoolValue *value) { (void) value; }
  void Visit(JsonNullValue *value) { (void) value; }
  void Visit(JsonRawValue *value) { (void) value; }
  void Visit(JsonObject *value) { m_obj = value; }
  void Visit(JsonArray *value) { (void) value; }
  void Visit(JsonUIntValue *value) { (void) value; }
  void Visit(JsonUInt64Value *value) { (void) value; }
  void Visit(JsonIntValue *value) { (void) value; }
  void Visit(JsonInt64Value *value) { (void) value; }
  void Visit(JsonDoubleValue *value) { (void) value; }

  JsonObject *Object() { return m_obj; }

 private:
  JsonObject *m_obj;
};

class ArrayCastVisitor : public JsonValueVisitorInterface {
 public:
  ArrayCastVisitor() : m_array(NULL) {}

  void Visit(JsonStringValue *value) { (void) value; }
  void Visit(JsonBoolValue *value) { (void) value; }
  void Visit(JsonNullValue *value) { (void) value; }
  void Visit(JsonRawValue *value) { (void) value; }
  void Visit(JsonObject *value) { (void) value; }
  void Visit(JsonArray *value) { m_array = value; }
  void Visit(JsonUIntValue *value) { (void) value; }
  void Visit(JsonUInt64Value *value) { (void) value; }
  void Visit(JsonIntValue *value) { (void) value; }
  void Visit(JsonInt64Value *value) { (void) value; }
  void Visit(JsonDoubleValue *value) { (void) value; }

  JsonArray *Array() { return m_array; }

 private:
  JsonArray *m_array;
};

}  // namespace

JsonValue* JsonValue::LookupElement(const JsonPointer &pointer) {
  JsonPointer::Iterator iter = pointer.begin();
  return LookupElementWithIter(&iter);
}

JsonValue* JsonLeafValue::LookupElementWithIter(
    JsonPointer::Iterator *iterator) {
  if (!iterator->IsValid() || !iterator->AtEnd()) {
    return NULL;
  }
  (*iterator)++;  // increment to move past the end.
  return this;
}


// Integer equality functions
namespace {

/*
 * This isn't pretty but it works. The original version used
 * numeric_limits::is_signed, numeric_limits::digits & numeric_limits::is_exact
 * to reduce the amount of code. However I couldn't get it to work without
 * signed vs unsigned warnings in gcc.
 */
template <typename T1, typename T2>
int CompareNumbers(T1 a, T2 b);

template <>
int CompareNumbers(uint32_t a, uint32_t b) {
  return (a < b) ? -1 : (a > b);
}

template <>
int CompareNumbers(uint32_t a, int32_t b) {
  if (b < 0) {
    return 1;
  }
  return (a < static_cast<uint32_t>(b)) ? -1 : (a > static_cast<uint32_t>(b));
}

template <>
int CompareNumbers(uint32_t a, uint64_t b) {
  return (static_cast<uint64_t>(a) < b) ? -1 : (static_cast<uint64_t>(a) > b);
}

template <>
int CompareNumbers(uint32_t a, int64_t b) {
  if (b < 0) {
    return 1;
  }
  return (static_cast<int64_t>(a) < b) ? -1 : (static_cast<int64_t>(a) > b);
}

template <>
int CompareNumbers(uint32_t a, double b) {
  return (static_cast<double>(a) < b) ? -1 : (static_cast<double>(a) > b);
}

template <>
int CompareNumbers(int32_t a, uint32_t b) {
  if (a < 0) {
    return -1;
  }
  return (static_cast<uint32_t>(a) < b) ? -1 : (static_cast<uint32_t>(a) > b);
}

template <>
int CompareNumbers(int32_t a, int32_t b) {
  return (a < b) ? -1 : (a > b);
}

template <>
int CompareNumbers(int32_t a, uint64_t b) {
  if (a < 0) {
    return -1;
  }
  return (static_cast<uint64_t>(a) < b) ? -1 : (static_cast<uint64_t>(a) > b);
}

template <>
int CompareNumbers(int32_t a, int64_t b) {
  return (static_cast<int64_t>(a) < b) ? -1 : (static_cast<int64_t>(a) > b);
}

template <>
int CompareNumbers(int32_t a, double b) {
  return (static_cast<double>(a) < b) ? -1 : (static_cast<double>(a) > b);
}

template <>
int CompareNumbers(uint64_t a, uint32_t b) {
  return (a < static_cast<uint64_t>(b)) ? -1 : (a > static_cast<uint64_t>(b));
}

template <>
int CompareNumbers(uint64_t a, int32_t b) {
  if (b < 0) {
    return 1;
  }
  return (a < static_cast<uint64_t>(b)) ? -1 : (a > static_cast<uint64_t>(b));
}

template <>
int CompareNumbers(uint64_t a, uint64_t b) {
  return (a < b) ? -1 : (a > b);
}

template <>
int CompareNumbers(uint64_t a, int64_t b) {
  if (b < 0) {
    return 1;
  }
  return (a < static_cast<uint64_t>(b)) ? -1 : (a > static_cast<uint64_t>(b));
}

template <>
int CompareNumbers(uint64_t a, double b) {
  return (static_cast<double>(a) < b) ? -1 : (static_cast<double>(a) > b);
}

template <>
int CompareNumbers(int64_t a, uint32_t b) {
  if (a < 0) {
    return -1;
  }
  return (a < static_cast<int64_t>(b)) ? -1 : (a > static_cast<int64_t>(b));
}

template <>
int CompareNumbers(int64_t a, int32_t b) {
  return (static_cast<int64_t>(a) < b) ? -1 : (static_cast<int64_t>(a) > b);
}

template <>
int CompareNumbers(int64_t a, uint64_t b) {
  if (a < 0) {
    return -1;
  }
  return (static_cast<uint64_t>(a) < b) ? -1 : (static_cast<uint64_t>(a) > b);
}

template <>
int CompareNumbers(int64_t a, int64_t b) {
  return (a < b) ? -1 : (a > b);
}

template <>
int CompareNumbers(int64_t a, double b) {
  return (static_cast<double>(a) < b) ? -1 : (static_cast<double>(a) > b);
}

template <>
int CompareNumbers(double a, uint32_t b) {
  return (a < static_cast<double>(b)) ? -1 : (a > static_cast<double>(b));
}

template <>
int CompareNumbers(double a, int32_t b) {
  return (a < static_cast<double>(b)) ? -1 : (a > static_cast<double>(b));
}

template <>
int CompareNumbers(double a, uint64_t b) {
  return (a < static_cast<double>(b)) ? -1 : (a > static_cast<double>(b));
}

template <>
int CompareNumbers(double a, int64_t b) {
  return (a < static_cast<double>(b)) ? -1 : (a > static_cast<double>(b));
}

template <>
int CompareNumbers(double a, double b) {
  return (a < b) ? -1 : (a > b);
}
}  // namespace


// Number equality functions
bool JsonUIntValue::Equals(const JsonIntValue &other) const {
  return CompareNumbers(m_value, other.Value()) == 0;
}

bool JsonUIntValue::Equals(const JsonUInt64Value &other) const {
  return CompareNumbers(m_value, other.Value()) == 0;
}

bool JsonUIntValue::Equals(const JsonInt64Value &other) const {
  return CompareNumbers(m_value, other.Value()) == 0;
}

bool JsonIntValue::Equals(const JsonUIntValue &other) const {
  return CompareNumbers(m_value, other.Value()) == 0;
}

bool JsonIntValue::Equals(const JsonUInt64Value &other) const {
  return CompareNumbers(m_value, other.Value()) == 0;
}

bool JsonIntValue::Equals(const JsonInt64Value &other) const {
  return CompareNumbers(m_value, other.Value()) == 0;
}

bool JsonUInt64Value::Equals(const JsonUIntValue &other) const {
  return CompareNumbers(m_value, other.Value()) == 0;
}

bool JsonUInt64Value::Equals(const JsonIntValue &other) const {
  return CompareNumbers(m_value, other.Value()) == 0;
}

bool JsonUInt64Value::Equals(const JsonInt64Value &other) const {
  return CompareNumbers(m_value, other.Value()) == 0;
}

bool JsonInt64Value::Equals(const JsonUIntValue &other) const {
  return CompareNumbers(m_value, other.Value()) == 0;
}

bool JsonInt64Value::Equals(const JsonIntValue &other) const {
  return CompareNumbers(m_value, other.Value()) == 0;
}

bool JsonInt64Value::Equals(const JsonUInt64Value &other) const {
  return CompareNumbers(m_value, other.Value()) == 0;
}

// Number inequality functions
int JsonUIntValue::Compare(const JsonUIntValue &other) const {
  return CompareNumbers(m_value, other.Value());
}

int JsonUIntValue::Compare(const JsonIntValue &other) const {
  return CompareNumbers(m_value, other.Value());
}

int JsonUIntValue::Compare(const JsonUInt64Value &other) const {
  return CompareNumbers(m_value, other.Value());
}

int JsonUIntValue::Compare(const JsonInt64Value &other) const {
  return CompareNumbers(m_value, other.Value());
}

int JsonUIntValue::Compare(const JsonDoubleValue &other) const {
  return CompareNumbers(m_value, other.Value());
}

int JsonIntValue::Compare(const JsonUIntValue &other) const {
  return CompareNumbers(m_value, other.Value());
}

int JsonIntValue::Compare(const JsonIntValue &other) const {
  return CompareNumbers(m_value, other.Value());
}

int JsonIntValue::Compare(const JsonUInt64Value &other) const {
  return CompareNumbers(m_value, other.Value());
}

int JsonIntValue::Compare(const JsonInt64Value &other) const {
  return CompareNumbers(m_value, other.Value());
}

int JsonIntValue::Compare(const JsonDoubleValue &other) const {
  return CompareNumbers(m_value, other.Value());
}

int JsonUInt64Value::Compare(const JsonUIntValue &other) const {
  return CompareNumbers(m_value, other.Value());
}

int JsonUInt64Value::Compare(const JsonIntValue &other) const {
  return CompareNumbers(m_value, other.Value());
}

int JsonUInt64Value::Compare(const JsonUInt64Value &other) const {
  return CompareNumbers(m_value, other.Value());
}

int JsonUInt64Value::Compare(const JsonInt64Value &other) const {
  return CompareNumbers(m_value, other.Value());
}

int JsonUInt64Value::Compare(const JsonDoubleValue &other) const {
  return CompareNumbers(m_value, other.Value());
}

int JsonInt64Value::Compare(const JsonUIntValue &other) const {
  return CompareNumbers(m_value, other.Value());
}

int JsonInt64Value::Compare(const JsonIntValue &other) const {
  return CompareNumbers(m_value, other.Value());
}

int JsonInt64Value::Compare(const JsonUInt64Value &other) const {
  return CompareNumbers(m_value, other.Value());
}

int JsonInt64Value::Compare(const JsonInt64Value &other) const {
  return CompareNumbers(m_value, other.Value());
}

int JsonInt64Value::Compare(const JsonDoubleValue &other) const {
  return CompareNumbers(m_value, other.Value());
}

int JsonDoubleValue::Compare(const JsonUIntValue &other) const {
  return CompareNumbers(m_value, other.Value());
}

int JsonDoubleValue::Compare(const JsonIntValue &other) const {
  return CompareNumbers(m_value, other.Value());
}

int JsonDoubleValue::Compare(const JsonUInt64Value &other) const {
  return CompareNumbers(m_value, other.Value());
}

int JsonDoubleValue::Compare(const JsonInt64Value &other) const {
  return CompareNumbers(m_value, other.Value());
}

int JsonDoubleValue::Compare(const JsonDoubleValue &other) const {
  return CompareNumbers(m_value, other.Value());
}

bool JsonUIntValue::FactorOf(const JsonUIntValue &value) const {
  return value.Value() % m_value == 0;
}

bool JsonUIntValue::FactorOf(const JsonIntValue &value) const {
  return value.Value() % m_value == 0;
}

bool JsonUIntValue::FactorOf(const JsonUInt64Value &value) const {
  return value.Value() % m_value == 0;
}

bool JsonUIntValue::FactorOf(const JsonInt64Value &value) const {
  return value.Value() % m_value == 0;
}

bool JsonUIntValue::FactorOf(const JsonDoubleValue &value) const {
  return fmod(value.Value(), m_value) == 0;
}

bool JsonIntValue::FactorOf(const JsonUIntValue &value) const {
  return value.Value() % m_value == 0;
}

bool JsonIntValue::FactorOf(const JsonIntValue &value) const {
  return value.Value() % m_value == 0;
}

bool JsonIntValue::FactorOf(const JsonUInt64Value &value) const {
  return value.Value() % m_value == 0;
}

bool JsonIntValue::FactorOf(const JsonInt64Value &value) const {
  return value.Value() % m_value == 0;
}

bool JsonIntValue::FactorOf(const JsonDoubleValue &value) const {
  return fmod(value.Value(), m_value) == 0;
}

bool JsonUInt64Value::FactorOf(const JsonUIntValue &value) const {
  return value.Value() % m_value == 0;
}

bool JsonUInt64Value::FactorOf(const JsonIntValue &value) const {
  return value.Value() % m_value == 0;
}

bool JsonUInt64Value::FactorOf(const JsonUInt64Value &value) const {
  return value.Value() % m_value == 0;
}

bool JsonUInt64Value::FactorOf(const JsonInt64Value &value) const {
  return value.Value() % m_value == 0;
}

bool JsonUInt64Value::FactorOf(const JsonDoubleValue &value) const {
  return fmod(value.Value(), m_value) == 0;
}

bool JsonInt64Value::FactorOf(const JsonUIntValue &value) const {
  return value.Value() % m_value == 0;
}

bool JsonInt64Value::FactorOf(const JsonIntValue &value) const {
  return value.Value() % m_value == 0;
}

bool JsonInt64Value::FactorOf(const JsonUInt64Value &value) const {
  return value.Value() % m_value == 0;
}

bool JsonInt64Value::FactorOf(const JsonInt64Value &value) const {
  return value.Value() % m_value == 0;
}

bool JsonInt64Value::FactorOf(const JsonDoubleValue &value) const {
  return fmod(value.Value(), m_value) == 0;
}

bool JsonDoubleValue::FactorOf(const JsonUIntValue &value) const {
  return fmod(value.Value(), m_value) == 0;
}

bool JsonDoubleValue::FactorOf(const JsonIntValue &value) const {
  return fmod(value.Value(), m_value) == 0;
}

bool JsonDoubleValue::FactorOf(const JsonUInt64Value &value) const {
  return fmod(value.Value(), m_value) == 0;
}

bool JsonDoubleValue::FactorOf(const JsonInt64Value &value) const {
  return fmod(value.Value(), m_value) == 0;
}

bool JsonDoubleValue::FactorOf(const JsonDoubleValue &value) const {
  return fmod(value.Value(), m_value) == 0;
}


JsonDoubleValue::JsonDoubleValue(double value)
    : m_value(value) {
  ostringstream str;
  str << value;
  m_as_string = str.str();
}

JsonDoubleValue::JsonDoubleValue(const DoubleRepresentation &rep) {
  AsDouble(rep, &m_value);
  m_as_string = AsString(rep);
}

bool JsonDoubleValue::AsDouble(const DoubleRepresentation &rep, double *out) {
  // TODO(simon): Check the limits here.
  double d = rep.fractional;
  while (d >= 1.0) {
    d /= 10.0;
  }
  for (unsigned int i = 0; i < rep.leading_fractional_zeros; i++) {
    d /= 10;
  }

  d += rep.full;
  d *= pow(10, rep.exponent);
  if (rep.is_negative && d != 0.0) {
    d *= -1;
  }
  *out = d;
  return true;
}

string JsonDoubleValue::AsString(const DoubleRepresentation &rep) {
  // Populate the string member
  if (rep.full == 0 && rep.fractional == 0) {
    return "0";
  }

  ostringstream output;
  if (rep.is_negative) {
    output << "-";
  }
  output << rep.full;
  if (rep.fractional) {
    output << ".";
    if (rep.leading_fractional_zeros) {
      output << string(rep.leading_fractional_zeros, '0');
    }
    output << rep.fractional;
  }
  if (rep.exponent) {
    output << "e" << rep.exponent;
  }
  return output.str();
}

JsonObject::~JsonObject() {
  STLDeleteValues(&m_members);
}

JsonValue* JsonObject::LookupElementWithIter(JsonPointer::Iterator *iterator) {
  if (!iterator->IsValid()) {
    return NULL;
  }

  if (iterator->AtEnd()) {
    return this;
  }

  const string token = **iterator;
  (*iterator)++;
  JsonValue *value = STLFindOrNull(m_members, token);
  if (value) {
    return value->LookupElementWithIter(iterator);
  } else {
    return NULL;
  }
}

bool JsonObject::Equals(const JsonObject &other) const {
  if (m_members.size() != other.m_members.size()) {
    return false;
  }

  MemberMap::const_iterator our_iter = m_members.begin();
  MemberMap::const_iterator other_iter = other.m_members.begin();
  for (; our_iter != m_members.end() && other_iter != other.m_members.end();
       our_iter++, other_iter++) {
    if (our_iter->first != other_iter->first ||
        *(our_iter->second) != *(other_iter->second)) {
      return false;
    }
  }
  return true;
}

void JsonObject::Add(const std::string &key, const std::string &value) {
  STLReplaceAndDelete(&m_members, key, new JsonStringValue(value));
}

void JsonObject::Add(const std::string &key, const char *value) {
  Add(key, string(value));
}

void JsonObject::Add(const std::string &key, unsigned int i) {
  STLReplaceAndDelete(&m_members, key, new JsonUIntValue(i));
}

void JsonObject::Add(const std::string &key, int i) {
  STLReplaceAndDelete(&m_members, key, new JsonIntValue(i));
}

void JsonObject::Add(const std::string &key, double d) {
  STLReplaceAndDelete(&m_members, key, new JsonDoubleValue(d));
}

void JsonObject::Add(const std::string &key, bool value) {
  STLReplaceAndDelete(&m_members, key, new JsonBoolValue(value));
}

void JsonObject::Add(const std::string &key) {
  STLReplaceAndDelete(&m_members, key, new JsonNullValue());
}

void JsonObject::AddRaw(const std::string &key, const std::string &value) {
  STLReplaceAndDelete(&m_members, key, new JsonRawValue(value));
}

bool JsonObject::Remove(const std::string &key) {
  return STLRemoveAndDelete(&m_members, key);
}

bool JsonObject::ReplaceValue(const std::string &key, JsonValue *value) {
  MemberMap::iterator iter = m_members.find(key);
  if (iter == m_members.end()) {
    delete value;
    return false;
  } else {
    delete iter->second;
    iter->second = value;
    return true;
  }
}

JsonObject* JsonObject::AddObject(const string &key) {
  JsonObject *obj = new JsonObject();
  STLReplaceAndDelete(&m_members, key, obj);
  return obj;
}

JsonArray* JsonObject::AddArray(const string &key) {
  JsonArray *array = new JsonArray();
  STLReplaceAndDelete(&m_members, key, array);
  return array;
}

void JsonObject::AddValue(const string &key, JsonValue *value) {
  STLReplaceAndDelete(&m_members, key, value);
}

JsonValue* JsonObject::Clone() const {
  JsonObject *object = new JsonObject();
  MemberMap::const_iterator iter = m_members.begin();
  for (; iter != m_members.end(); ++iter) {
    object->AddValue(iter->first, iter->second->Clone());
  }
  return object;
}

void JsonObject::VisitProperties(JsonObjectPropertyVisitor *visitor) const {
  MemberMap::const_iterator iter = m_members.begin();
  for (; iter != m_members.end(); ++iter) {
    visitor->VisitProperty(iter->first, *(iter->second));
  }
}

JsonArray::~JsonArray() {
  STLDeleteElements(&m_values);
}

JsonValue* JsonArray::LookupElementWithIter(JsonPointer::Iterator *iterator) {
  if (!iterator->IsValid()) {
    return NULL;
  }

  if (iterator->AtEnd()) {
    return this;
  }

  unsigned int index;
  if (!StringToInt(**iterator, &index, true)) {
    (*iterator)++;
    return NULL;
  }
  (*iterator)++;

  if (index < m_values.size()) {
    return m_values[index]->LookupElementWithIter(iterator);
  } else {
    return NULL;
  }
}

bool JsonArray::Equals(const JsonArray &other) const {
  if (m_values.size() != other.m_values.size()) {
    return false;
  }

  ValuesVector::const_iterator our_iter = m_values.begin();
  ValuesVector::const_iterator other_iter = other.m_values.begin();
  for (; our_iter != m_values.end() && other_iter != other.m_values.end();
       our_iter++, other_iter++) {
    if (**our_iter != **other_iter) {
      return false;
    }
  }
  return true;
}

bool JsonArray::RemoveElementAt(uint32_t index) {
  if (index < m_values.size()) {
    ValuesVector::iterator iter = m_values.begin() + index;
    delete *iter;
    m_values.erase(iter);
    return true;
  }
  return false;
}

bool JsonArray::ReplaceElementAt(uint32_t index, JsonValue *value) {
  if (index < m_values.size()) {
    ValuesVector::iterator iter = m_values.begin() + index;
    delete *iter;
    *iter = value;
    return true;
  }
  // Ownership is transferred, so it's up to us to delete it.
  delete value;
  return false;
}

bool JsonArray::InsertElementAt(uint32_t index, JsonValue *value) {
  if (index < m_values.size()) {
    ValuesVector::iterator iter = m_values.begin() + index;
    m_values.insert(iter, value);
    return true;
  }
  // Ownership is transferred, so it's up to us to delete it.
  delete value;
  return false;
}

JsonValue* JsonArray::Clone() const {
  JsonArray *array = new JsonArray();
  ValuesVector::const_iterator iter = m_values.begin();
  for (; iter != m_values.end(); iter++) {
    array->AppendValue((*iter)->Clone());
  }
  return array;
}

const JsonValue *JsonArray::ElementAt(unsigned int i) const {
  if (i < m_values.size()) {
    return m_values[i];
  } else {
    return NULL;
  }
}

JsonObject* ObjectCast(JsonValue *value) {
  // Benchmarks for 100M operations
  // dynamic_cast<> : 1.8s
  // Type() method & static_cast<>: 0.39s
  // typeip(value) == ..  & static_cast<> : 0.34s
  // &typeif(value) == .. &static_cast<>: 0.30s
  // Visitor pattern : 2.18s
  //
  // The visitor pattern is more costly since it requires 2 vtable lookups.
  // If performance becomes an issue we should consider static_cast<> here.
  ObjectCastVisitor visitor;
  value->Accept(&visitor);
  return visitor.Object();
}

JsonArray* ArrayCast(JsonValue *value) {
  ArrayCastVisitor visitor;
  value->Accept(&visitor);
  return visitor.Array();
}

// operator<<
std::ostream& operator<<(std::ostream &os, const JsonStringValue &value) {
  return os << value.Value();
}

std::ostream& operator<<(std::ostream &os, const JsonUIntValue &value) {
  return os << value.Value();
}

std::ostream& operator<<(std::ostream &os, const JsonIntValue &value) {
  return os << value.Value();
}

std::ostream& operator<<(std::ostream &os, const JsonUInt64Value &value) {
  return os << value.Value();
}

std::ostream& operator<<(std::ostream &os, const JsonInt64Value &value) {
  return os << value.Value();
}

std::ostream& operator<<(std::ostream &os, const JsonDoubleValue &value) {
  return os << value.Value();
}

std::ostream& operator<<(std::ostream &os, const JsonBoolValue &value) {
  return os << value.Value();
}

std::ostream& operator<<(std::ostream &os, const JsonNullValue &) {
  return os << "null";
}

std::ostream& operator<<(std::ostream &os, const JsonRawValue &value) {
  return os << value.Value();
}

}  // namespace web
}  // namespace ola
