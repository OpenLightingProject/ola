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
 * Json.cpp
 * A simple set of classes for generating JSON.
 * See http://www.json.org/
 * Copyright (C) 2012 Simon Newton
 */

#include <math.h>
#include <string>
#include <limits>
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

  void Visit(JsonString *value) { (void) value; }
  void Visit(JsonBool *value) { (void) value; }
  void Visit(JsonNull *value) { (void) value; }
  void Visit(JsonRawValue *value) { (void) value; }
  void Visit(JsonObject *value) { m_obj = value; }
  void Visit(JsonArray *value) { (void) value; }
  void Visit(JsonUInt *value) { (void) value; }
  void Visit(JsonUInt64 *value) { (void) value; }
  void Visit(JsonInt *value) { (void) value; }
  void Visit(JsonInt64 *value) { (void) value; }
  void Visit(JsonDouble *value) { (void) value; }

  JsonObject *Object() { return m_obj; }

 private:
  JsonObject *m_obj;
};

class ArrayCastVisitor : public JsonValueVisitorInterface {
 public:
  ArrayCastVisitor() : m_array(NULL) {}

  void Visit(JsonString *value) { (void) value; }
  void Visit(JsonBool *value) { (void) value; }
  void Visit(JsonNull *value) { (void) value; }
  void Visit(JsonRawValue *value) { (void) value; }
  void Visit(JsonObject *value) { (void) value; }
  void Visit(JsonArray *value) { m_array = value; }
  void Visit(JsonUInt *value) { (void) value; }
  void Visit(JsonUInt64 *value) { (void) value; }
  void Visit(JsonInt *value) { (void) value; }
  void Visit(JsonInt64 *value) { (void) value; }
  void Visit(JsonDouble *value) { (void) value; }

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
bool JsonUInt::Equals(const JsonInt &other) const {
  return CompareNumbers(m_value, other.Value()) == 0;
}

bool JsonUInt::Equals(const JsonUInt64 &other) const {
  return CompareNumbers(m_value, other.Value()) == 0;
}

bool JsonUInt::Equals(const JsonInt64 &other) const {
  return CompareNumbers(m_value, other.Value()) == 0;
}

bool JsonInt::Equals(const JsonUInt &other) const {
  return CompareNumbers(m_value, other.Value()) == 0;
}

bool JsonInt::Equals(const JsonUInt64 &other) const {
  return CompareNumbers(m_value, other.Value()) == 0;
}

bool JsonInt::Equals(const JsonInt64 &other) const {
  return CompareNumbers(m_value, other.Value()) == 0;
}

bool JsonUInt64::Equals(const JsonUInt &other) const {
  return CompareNumbers(m_value, other.Value()) == 0;
}

bool JsonUInt64::Equals(const JsonInt &other) const {
  return CompareNumbers(m_value, other.Value()) == 0;
}

bool JsonUInt64::Equals(const JsonInt64 &other) const {
  return CompareNumbers(m_value, other.Value()) == 0;
}

bool JsonInt64::Equals(const JsonUInt &other) const {
  return CompareNumbers(m_value, other.Value()) == 0;
}

bool JsonInt64::Equals(const JsonInt &other) const {
  return CompareNumbers(m_value, other.Value()) == 0;
}

bool JsonInt64::Equals(const JsonUInt64 &other) const {
  return CompareNumbers(m_value, other.Value()) == 0;
}

// Number inequality functions
int JsonUInt::Compare(const JsonUInt &other) const {
  return CompareNumbers(m_value, other.Value());
}

int JsonUInt::Compare(const JsonInt &other) const {
  return CompareNumbers(m_value, other.Value());
}

int JsonUInt::Compare(const JsonUInt64 &other) const {
  return CompareNumbers(m_value, other.Value());
}

int JsonUInt::Compare(const JsonInt64 &other) const {
  return CompareNumbers(m_value, other.Value());
}

int JsonUInt::Compare(const JsonDouble &other) const {
  return CompareNumbers(m_value, other.Value());
}

int JsonInt::Compare(const JsonUInt &other) const {
  return CompareNumbers(m_value, other.Value());
}

int JsonInt::Compare(const JsonInt &other) const {
  return CompareNumbers(m_value, other.Value());
}

int JsonInt::Compare(const JsonUInt64 &other) const {
  return CompareNumbers(m_value, other.Value());
}

int JsonInt::Compare(const JsonInt64 &other) const {
  return CompareNumbers(m_value, other.Value());
}

int JsonInt::Compare(const JsonDouble &other) const {
  return CompareNumbers(m_value, other.Value());
}

int JsonUInt64::Compare(const JsonUInt &other) const {
  return CompareNumbers(m_value, other.Value());
}

int JsonUInt64::Compare(const JsonInt &other) const {
  return CompareNumbers(m_value, other.Value());
}

int JsonUInt64::Compare(const JsonUInt64 &other) const {
  return CompareNumbers(m_value, other.Value());
}

int JsonUInt64::Compare(const JsonInt64 &other) const {
  return CompareNumbers(m_value, other.Value());
}

int JsonUInt64::Compare(const JsonDouble &other) const {
  return CompareNumbers(m_value, other.Value());
}

int JsonInt64::Compare(const JsonUInt &other) const {
  return CompareNumbers(m_value, other.Value());
}

int JsonInt64::Compare(const JsonInt &other) const {
  return CompareNumbers(m_value, other.Value());
}

int JsonInt64::Compare(const JsonUInt64 &other) const {
  return CompareNumbers(m_value, other.Value());
}

int JsonInt64::Compare(const JsonInt64 &other) const {
  return CompareNumbers(m_value, other.Value());
}

int JsonInt64::Compare(const JsonDouble &other) const {
  return CompareNumbers(m_value, other.Value());
}

int JsonDouble::Compare(const JsonUInt &other) const {
  return CompareNumbers(m_value, other.Value());
}

int JsonDouble::Compare(const JsonInt &other) const {
  return CompareNumbers(m_value, other.Value());
}

int JsonDouble::Compare(const JsonUInt64 &other) const {
  return CompareNumbers(m_value, other.Value());
}

int JsonDouble::Compare(const JsonInt64 &other) const {
  return CompareNumbers(m_value, other.Value());
}

int JsonDouble::Compare(const JsonDouble &other) const {
  return CompareNumbers(m_value, other.Value());
}

bool JsonUInt::FactorOf(const JsonUInt &value) const {
  return value.Value() % m_value == 0;
}

bool JsonUInt::FactorOf(const JsonInt &value) const {
  return value.Value() % m_value == 0;
}

bool JsonUInt::FactorOf(const JsonUInt64 &value) const {
  return value.Value() % m_value == 0;
}

bool JsonUInt::FactorOf(const JsonInt64 &value) const {
  return value.Value() % m_value == 0;
}

bool JsonUInt::FactorOf(const JsonDouble &value) const {
  return fmod(value.Value(), m_value) == 0;
}

bool JsonInt::FactorOf(const JsonUInt &value) const {
  return value.Value() % m_value == 0;
}

bool JsonInt::FactorOf(const JsonInt &value) const {
  return value.Value() % m_value == 0;
}

bool JsonInt::FactorOf(const JsonUInt64 &value) const {
  return value.Value() % m_value == 0;
}

bool JsonInt::FactorOf(const JsonInt64 &value) const {
  return value.Value() % m_value == 0;
}

bool JsonInt::FactorOf(const JsonDouble &value) const {
  return fmod(value.Value(), m_value) == 0;
}

bool JsonUInt64::FactorOf(const JsonUInt &value) const {
  return value.Value() % m_value == 0;
}

bool JsonUInt64::FactorOf(const JsonInt &value) const {
  return value.Value() % m_value == 0;
}

bool JsonUInt64::FactorOf(const JsonUInt64 &value) const {
  return value.Value() % m_value == 0;
}

bool JsonUInt64::FactorOf(const JsonInt64 &value) const {
  return value.Value() % m_value == 0;
}

bool JsonUInt64::FactorOf(const JsonDouble &value) const {
  return fmod(value.Value(), m_value) == 0;
}

bool JsonInt64::FactorOf(const JsonUInt &value) const {
  return value.Value() % m_value == 0;
}

bool JsonInt64::FactorOf(const JsonInt &value) const {
  return value.Value() % m_value == 0;
}

bool JsonInt64::FactorOf(const JsonUInt64 &value) const {
  return value.Value() % m_value == 0;
}

bool JsonInt64::FactorOf(const JsonInt64 &value) const {
  return value.Value() % m_value == 0;
}

bool JsonInt64::FactorOf(const JsonDouble &value) const {
  return fmod(value.Value(), m_value) == 0;
}

bool JsonDouble::FactorOf(const JsonUInt &value) const {
  return fmod(value.Value(), m_value) == 0;
}

bool JsonDouble::FactorOf(const JsonInt &value) const {
  return fmod(value.Value(), m_value) == 0;
}

bool JsonDouble::FactorOf(const JsonUInt64 &value) const {
  return fmod(value.Value(), m_value) == 0;
}

bool JsonDouble::FactorOf(const JsonInt64 &value) const {
  return fmod(value.Value(), m_value) == 0;
}

bool JsonDouble::FactorOf(const JsonDouble &value) const {
  return fmod(value.Value(), m_value) == 0;
}


JsonDouble::JsonDouble(double value)
    : m_value(value) {
  ostringstream str;
  str << value;
  m_as_string = str.str();
}

JsonDouble::JsonDouble(const DoubleRepresentation &rep) {
  AsDouble(rep, &m_value);
  m_as_string = AsString(rep);
}

bool JsonDouble::AsDouble(const DoubleRepresentation &rep, double *out) {
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

string JsonDouble::AsString(const DoubleRepresentation &rep) {
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

void JsonObject::Add(const string &key, const string &value) {
  STLReplaceAndDelete(&m_members, key, new JsonString(value));
}

void JsonObject::Add(const string &key, const char *value) {
  Add(key, string(value));
}

void JsonObject::Add(const string &key, unsigned int i) {
  STLReplaceAndDelete(&m_members, key, new JsonUInt(i));
}

void JsonObject::Add(const string &key, int i) {
  STLReplaceAndDelete(&m_members, key, new JsonInt(i));
}

void JsonObject::Add(const string &key, double d) {
  STLReplaceAndDelete(&m_members, key, new JsonDouble(d));
}

void JsonObject::Add(const string &key, bool value) {
  STLReplaceAndDelete(&m_members, key, new JsonBool(value));
}

void JsonObject::Add(const string &key) {
  STLReplaceAndDelete(&m_members, key, new JsonNull());
}

void JsonObject::AddRaw(const string &key, const string &value) {
  STLReplaceAndDelete(&m_members, key, new JsonRawValue(value));
}

bool JsonObject::Remove(const string &key) {
  return STLRemoveAndDelete(&m_members, key);
}

bool JsonObject::ReplaceValue(const string &key, JsonValue *value) {
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
std::ostream& operator<<(std::ostream &os, const JsonString &value) {
  return os << value.Value();
}

std::ostream& operator<<(std::ostream &os, const JsonUInt &value) {
  return os << value.Value();
}

std::ostream& operator<<(std::ostream &os, const JsonInt &value) {
  return os << value.Value();
}

std::ostream& operator<<(std::ostream &os, const JsonUInt64 &value) {
  return os << value.Value();
}

std::ostream& operator<<(std::ostream &os, const JsonInt64 &value) {
  return os << value.Value();
}

std::ostream& operator<<(std::ostream &os, const JsonDouble &value) {
  return os << value.Value();
}

std::ostream& operator<<(std::ostream &os, const JsonBool &value) {
  return os << value.Value();
}

std::ostream& operator<<(std::ostream &os, const JsonNull &) {
  return os << "null";
}

std::ostream& operator<<(std::ostream &os, const JsonRawValue &value) {
  return os << value.Value();
}

}  // namespace web
}  // namespace ola
