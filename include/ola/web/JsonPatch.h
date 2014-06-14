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
 * JsonPatch.h
 * Implementation of RFC 6902.
 * Copyright (C) 2014 Simon Newton
 */

/**
 * @addtogroup json
 * @{
 * @file JsonPatch.h
 * @brief Implementation of JSON Patch (RFC 6902).
 * @}
 */

#ifndef INCLUDE_OLA_WEB_JSONPATCH_H_
#define INCLUDE_OLA_WEB_JSONPATCH_H_

#include <ola/base/Macro.h>
#include <ola/web/Json.h>
#include <ola/web/JsonPointer.h>
#include <memory>
#include <string>
#include <vector>

namespace ola {
namespace web {

/**
 * @addtogroup json
 * @{
 */

class JsonPatchSet;

/**
 * @brief A class to serialize a JSONValue to text.
 */
class JsonPatchOp {
 public:
  virtual ~JsonPatchOp() {}

  /**
   * @brief Apply the patch operation to the value.
   * @param value A pointer to a JsonValue object. This may be modified,
   * replaced or deleted entirely by the patch operation.
   * @returns True if the patch was sucessfully applied, false otherwise.
   */
  virtual bool Apply(JsonValue **value) const = 0;
};

/**
 * @brief Add a JsonValue
 */
class JsonPatchAddOp : public JsonPatchOp {
 public:
  /**
   * @brief Add the JsonValue to the specified path.
   * @param path The path to add the value at.
   * @param value the value to add, ownership is transferred.
   */
  JsonPatchAddOp(const JsonPointer &path, const JsonValue *value)
      : m_pointer(path),
        m_value(value) {
  }

  bool Apply(JsonValue **value) const;

 private:
  JsonPointer m_pointer;
  std::auto_ptr<const JsonValue> m_value;

  DISALLOW_COPY_AND_ASSIGN(JsonPatchAddOp);
};

/**
 * @brief Remove the value at the specifed path.
 */
class JsonPatchRemoveOp : public JsonPatchOp {
 public:
  /**
   * @brief Add the JsonValue to the specified path.
   * @param path The path to remove.
   */
  explicit JsonPatchRemoveOp(const JsonPointer &path)
      : m_pointer(path) {
  }

  bool Apply(JsonValue **value) const;

 private:
  const JsonPointer m_pointer;

  DISALLOW_COPY_AND_ASSIGN(JsonPatchRemoveOp);
};

/**
 * @brief Replace the value at the specifed path.
 */
class JsonPatchReplaceOp : public JsonPatchOp {
 public:
  /**
   * @brief Replace the JsonValue at the specified path.
   * @param path The path to replace the value at.
   * @param value The value to replace with, ownership is transferred.
   */
  JsonPatchReplaceOp(const JsonPointer &path, const JsonValue *value)
      : m_pointer(path),
        m_value(value) {
  }

  bool Apply(JsonValue **value) const;

 private:
  const JsonPointer m_pointer;
  std::auto_ptr<const JsonValue> m_value;

  DISALLOW_COPY_AND_ASSIGN(JsonPatchReplaceOp);
};

/**
 * @brief Move a value from one location to another.
 */
class JsonPatchMoveOp : public JsonPatchOp {
 public:
  /**
   * @brief Move a value from one location to another.
   * @param from The path to move from.
   * @param to The path to move to.
   */
  JsonPatchMoveOp(const JsonPointer &from, const JsonPointer &to)
      : m_from(from),
        m_to(to) {
  }

  bool Apply(JsonValue **value) const;

 private:
  JsonPointer m_from;
  JsonPointer m_to;

  DISALLOW_COPY_AND_ASSIGN(JsonPatchMoveOp);
};

/**
 * @brief Copy a value from one location to another.
 */
class JsonPatchCopyOp : public JsonPatchOp {
 public:
  /**
   * @brief Copy a value from one location to another.
   * @param from The path to copy from.
   * @param to The path to copy to.
   */
  JsonPatchCopyOp(const JsonPointer &from, const JsonPointer &to)
    : m_from(from),
      m_to(to) {
  }

  bool Apply(JsonValue **value) const;

 private:
  JsonPointer m_from;
  JsonPointer m_to;

  DISALLOW_COPY_AND_ASSIGN(JsonPatchCopyOp);
};

/**
 * @brief Test a path matches the specified value.
 */
class JsonPatchTestOp : public JsonPatchOp {
 public:
  JsonPatchTestOp(const JsonPointer &path, const JsonValue *value)
      : m_pointer(path),
        m_value(value) {
  }

  bool Apply(JsonValue **value) const;

 private:
  JsonPointer m_pointer;
  std::auto_ptr<const JsonValue> m_value;

  DISALLOW_COPY_AND_ASSIGN(JsonPatchTestOp);
};


/**
 * @brief An ordered collection of JsonPatchOps.
 */
class JsonPatchSet {
 public:
  JsonPatchSet() {}
  ~JsonPatchSet();

  /**
   * @brief Add a patch operation to the set
   * @param op the operation to add, ownership is transferred.
   */
  void AddOp(JsonPatchOp *op);

  /**
   * @brief Apply this patch set to a value.
   *
   * Don't call this directly, instead use JsonData::Apply().
   */
  bool Apply(JsonValue **value) const;

  bool Empty() const { return m_patch_ops.empty(); }

 private:
  typedef std::vector<JsonPatchOp*> PatchOps;

  PatchOps m_patch_ops;
};
/**@}*/
}  // namespace web
}  // namespace ola
#endif  // INCLUDE_OLA_WEB_JSONPATCH_H_
