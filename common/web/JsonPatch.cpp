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
 * JsonPatch.cpp
 * Implementation of RFC 6902.
 * Copyright (C) 2014 Simon Newton
 */
#include "ola/web/JsonPatch.h"

#include <memory>
#include <string>
#include <vector>

#include "ola/Logging.h"
#include "ola/stl/STLUtils.h"
#include "ola/web/Json.h"
#include "ola/web/JsonPointer.h"

namespace ola {
namespace web {

using std::string;

namespace {

string LastToken(const JsonPointer &pointer) {
  return pointer.TokenAt(pointer.TokenCount() - 2);
}

JsonValue *GetParent(JsonValue *value, const JsonPointer &pointer) {
  JsonPointer parent_pointer(pointer);
  parent_pointer.Pop();
  return value->LookupElement(parent_pointer);
}

/**
 * Most ops have an if-object-then, else-if-array-then clause. This puts all
 * the grunt work in a single method
 */
class ObjectOrArrayAction {
 public:
  virtual ~ObjectOrArrayAction() {}

  bool TakeActionOn(JsonValue *value, const JsonPointer &target);

  // Implement these with the specific actions
  virtual bool Object(JsonObject *object, const string &key) = 0;
  virtual bool ArrayIndex(JsonArray *array, uint32_t index) = 0;
  // Called when the index is '-'
  virtual bool ArrayLast(JsonArray *array) = 0;
};

bool ObjectOrArrayAction::TakeActionOn(JsonValue *value,
                                       const JsonPointer &target) {
  JsonValue *parent = GetParent(value, target);
  if (!parent) {
    return false;
  }
  const string key = LastToken(target);

  JsonObject *object = ObjectCast(parent);
  if (object) {
    return Object(object, key);
  }

  JsonArray *array = ArrayCast(parent);
  if (array) {
    if (key == "-") {
      return ArrayLast(array);
    }

    uint32_t index;
    if (!StringToInt(key, &index)) {
      return false;
    }
    return ArrayIndex(array, index);
  }
  return false;
}

class AddAction : public ObjectOrArrayAction {
 public:
  explicit AddAction(const JsonValue *value_to_clone)
      : m_value(value_to_clone) {}

  bool Object(JsonObject *object, const string &key) {
    object->AddValue(key, m_value->Clone());
    return true;
  }

  bool ArrayIndex(JsonArray *array, uint32_t index) {
    return array->InsertElementAt(index, m_value->Clone());
  }

  bool ArrayLast(JsonArray *array) {
    array->AppendValue(m_value->Clone());
    return true;
  }

 private:
  const JsonValue *m_value;
};

class RemoveAction : public ObjectOrArrayAction {
 public:
  bool Object(JsonObject *object, const string &key) {
    return object->Remove(key);
  }

  bool ArrayIndex(JsonArray *array, uint32_t index) {
    return array->RemoveElementAt(index);
  }

  bool ArrayLast(JsonArray *array) {
    if (array->IsEmpty()) {
      return false;
    }

    array->RemoveElementAt(array->Size() - 1);
    return true;
  }
};

class ReplaceAction : public ObjectOrArrayAction {
 public:
  explicit ReplaceAction(const JsonValue *value) : m_value(value) {}

  bool Object(JsonObject *object, const string &key) {
    return object->ReplaceValue(key, m_value->Clone());
  }

  bool ArrayIndex(JsonArray *array, uint32_t index) {
    return array->ReplaceElementAt(index, m_value->Clone());
  }

  bool ArrayLast(JsonArray *array) {
    if (array->IsEmpty()) {
      return false;
    }

    array->ReplaceElementAt(array->Size() - 1, m_value->Clone());
    return true;
  }
 private:
  const JsonValue *m_value;
};

bool AddOp(const JsonPointer &target, JsonValue **root,
           const JsonValue *value_to_clone) {
  if (!target.IsValid()) {
    return false;
  }

  if (target.TokenCount() == 1) {
    // Add also operates as replace as per the spec.
    // Make a copy before we delete, since the value_to_clone may be within the
    // root.
    JsonValue *new_value = value_to_clone ? value_to_clone->Clone() : NULL;
    if (*root) {
      delete *root;
    }
    *root = new_value;
    return true;
  }

  // If we're not operating on the root, NULLs aren't allowed.
  if (*root == NULL || value_to_clone == NULL) {
    return false;
  }

  AddAction action(value_to_clone);
  return action.TakeActionOn(*root, target);
}

}  // namespace

bool JsonPatchAddOp::Apply(JsonValue **value) const {
  return AddOp(m_pointer, value, m_value.get());
}

bool JsonPatchRemoveOp::Apply(JsonValue **value) const {
  if (!m_pointer.IsValid()) {
    return false;
  }

  if (m_pointer.TokenCount() == 1) {
    delete *value;
    *value = NULL;
    return true;
  }

  if (*value == NULL) {
    return false;
  }

  RemoveAction action;
  return action.TakeActionOn(*value, m_pointer);
}

bool JsonPatchReplaceOp::Apply(JsonValue **value) const {
  if (!m_pointer.IsValid()) {
    return false;
  }

  if (m_pointer.TokenCount() == 1) {
    delete *value;
    *value = m_value.get() ? m_value->Clone() : NULL;
    return true;
  }

  // If we're not operating on the root, NULLs aren't allowed.
  if (*value == NULL || m_value.get() == NULL) {
    return false;
  }

  ReplaceAction action(m_value.get());
  return action.TakeActionOn(*value, m_pointer);
}

bool JsonPatchMoveOp::Apply(JsonValue **value) const {
  if (!m_to.IsValid() || !m_from.IsValid()) {
    return false;
  }

  if (m_from == m_to) {
    return true;
  }

  if (m_from.IsPrefixOf(m_to)) {
    return false;
  }

  JsonValue *src_parent = GetParent(*value, m_from);
  if (!src_parent) {
    return false;
  }

  const string last_token = LastToken(m_from);
  JsonPointer child_ptr("/" + last_token);
  JsonValue *source = src_parent->LookupElement(child_ptr);
  if (!source) {
    return false;
  }

  if (!AddOp(m_to, value, source)) {
    return false;
  }

  if (m_to.IsPrefixOf(m_from)) {
    // At this point the original has already been destroyed during the Add
    return true;
  }

  RemoveAction action;
  if (!action.TakeActionOn(src_parent, child_ptr)) {
    OLA_WARN << "Remove-after-move returned false!";
  }
  return true;
}

bool JsonPatchCopyOp::Apply(JsonValue **value) const {
  if (!m_to.IsValid() || !m_from.IsValid()) {
    return false;
  }

  if (m_from == m_to) {
    return true;
  }

  if (*value == NULL) {
    return false;
  }

  JsonValue *src_parent = GetParent(*value, m_from);
  if (!src_parent) {
    return false;
  }

  const string last_token = LastToken(m_from);
  JsonPointer child_ptr("/" + last_token);
  JsonValue *source = src_parent->LookupElement(child_ptr);
  if (!source) {
    return false;
  }

  return AddOp(m_to, value, source);
}

bool JsonPatchTestOp::Apply(JsonValue **value) const {
  if (!m_pointer.IsValid()) {
    return false;
  }

  if (*value == NULL) {
    return m_pointer.TokenCount() == 1 && m_value.get() == NULL;
  }

  JsonValue *target = (*value)->LookupElement(m_pointer);
  if (!target) {
    return false;
  }
  return *target == *m_value.get();
}

JsonPatchSet::~JsonPatchSet() {
  STLDeleteElements(&m_patch_ops);
}

void JsonPatchSet::AddOp(JsonPatchOp *op) {
  m_patch_ops.push_back(op);
}

bool JsonPatchSet::Apply(JsonValue **value) const {
  PatchOps::const_iterator iter = m_patch_ops.begin();
  for (; iter != m_patch_ops.end(); ++iter) {
    if (!(*iter)->Apply(value)) {
      return false;
    }
  }
  return true;
}
}  // namespace web
}  // namespace ola
