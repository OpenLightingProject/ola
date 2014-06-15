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
 * JsonPatchParser.cpp
 * Create a JsonPatchSet from a string.
 * Copyright (C) 2014 Simon Newton
 */
#include "ola/web/JsonPatchParser.h"

#define __STDC_LIMIT_MACROS  // for UINT8_MAX & friends

#include <memory>
#include <string>
#include "ola/Logging.h"
#include "ola/StringUtils.h"
#include "ola/stl/STLUtils.h"
#include "ola/web/Json.h"
#include "ola/web/JsonLexer.h"

namespace ola {
namespace web {

using std::string;

const char JsonPatchParser::kPatchListError[] =
  "A JSON Patch document must be an array";
const char JsonPatchParser::kPatchElementError[] =
  "Elements within a JSON Patch array must be objects";
const char JsonPatchParser::kMissingPath[] =
  "Missing path specifier";
const char JsonPatchParser::kMissingValue[] =
  "Missing or invalid value";
const char JsonPatchParser::kMissingFrom[] =
  "Missing from specifier";
const char JsonPatchParser::kAddOp[] = "add";
const char JsonPatchParser::kCopyOp[] = "copy";
const char JsonPatchParser::kFromKey[] = "from";
const char JsonPatchParser::kMoveOp[] = "move";
const char JsonPatchParser::kOpKey[] = "op";
const char JsonPatchParser::kPathKey[] = "path";
const char JsonPatchParser::kRemoveOp[] = "remove";
const char JsonPatchParser::kReplaceOp[] = "replace";
const char JsonPatchParser::kTestOp[] = "test";
const char JsonPatchParser::kValueKey[] = "value";

void JsonPatchParser::Begin() {
  m_parser_depth = 0;
  m_error = "";
  m_key = "";
  m_state = TOP;
  m_parser.Begin();
}

void JsonPatchParser::End() {
  if (m_state != TOP) {
    SetError("Invalid JSON data");
  }
}

void JsonPatchParser::String(const string &value) {
  switch (m_state) {
    case TOP:
      SetError(kPatchListError);
      return;
    case PATCH_LIST:
      SetError(kPatchElementError);
      return;
    case PATCH:
      HandlePatchString(value);
      return;
    default:
      m_parser.String(value);
  }
}

void JsonPatchParser::Number(uint32_t value) {
  HandleNumber(value);
}

void JsonPatchParser::Number(int32_t value) {
  HandleNumber(value);
}

void JsonPatchParser::Number(uint64_t value) {
  HandleNumber(value);
}

void JsonPatchParser::Number(int64_t value) {
  HandleNumber(value);
}

void JsonPatchParser::Number(const JsonDouble::DoubleRepresentation &rep) {
  HandleNumber(rep);
}

void JsonPatchParser::Number(double value) {
  HandleNumber(value);
}

void JsonPatchParser::Bool(bool value) {
  switch (m_state) {
    case TOP:
      SetError(kPatchListError);
      return;
    case PATCH_LIST:
      SetError(kPatchElementError);
      return;
    case PATCH:
      if (m_key == kValueKey) {
        m_value.reset(new JsonBool(value));
      }
    case VALUE:
      m_parser.Bool(value);
    default:
      {}
  }
}

void JsonPatchParser::Null() {
  switch (m_state) {
    case TOP:
      SetError(kPatchListError);
      return;
    case PATCH_LIST:
      SetError(kPatchElementError);
      return;
    case PATCH:
      if (m_key == kValueKey) {
        m_value.reset(new JsonNull());
      }
    case VALUE:
      m_parser.Null();
    default:
      {}
  }
}

void JsonPatchParser::OpenArray() {
  switch (m_state) {
    case TOP:
      m_state = PATCH_LIST;
      return;
    case PATCH_LIST:
      SetError(kPatchElementError);
      return;
    case PATCH:
      m_parser_depth = 0;
      m_state = VALUE;
    case VALUE:
      m_parser_depth++;
      m_parser.OpenArray();
      return;
  }
}

void JsonPatchParser::CloseArray() {
  switch (m_state) {
    case TOP:
      return;
    case PATCH_LIST:
      m_state = TOP;
      return;
    case PATCH:
      return;
    case VALUE:
      m_parser.CloseArray();
      m_parser_depth--;
      if (m_parser_depth == 0) {
        if (m_key == kValueKey) {
          m_value.reset(m_parser.ClaimRoot());
        }
        m_state = PATCH;
      }
  }
}

void JsonPatchParser::OpenObject() {
  switch (m_state) {
    case TOP:
      SetError(kPatchListError);
      return;
    case PATCH_LIST:
      m_state = PATCH;
      m_value.reset();
      m_path.Reset();
      m_op = "";
      m_from.Reset();
      return;
    case PATCH:
      m_parser_depth = 0;
      m_state = VALUE;
    case VALUE:
      m_parser_depth++;
      m_parser.OpenObject();
      return;
  }
}

void JsonPatchParser::ObjectKey(const std::string &key) {
  if (m_state == VALUE) {
    m_parser.ObjectKey(key);
  } else {
    m_key = key;
  }
}

void JsonPatchParser::CloseObject() {
  switch (m_state) {
    case TOP:
      return;
    case PATCH_LIST:
      return;
    case PATCH:
      m_state = PATCH_LIST;
      HandlePatch();
      return;
    case VALUE:
      m_parser.CloseObject();
      m_parser_depth--;
      if (m_parser_depth == 0) {
        if (m_key == kValueKey) {
          m_value.reset(m_parser.ClaimRoot());
        }
        m_state = PATCH;
      }
  }
}

void JsonPatchParser::SetError(const string &error) {
  if (m_error.empty()) {
    m_error = error;
  }
}

string JsonPatchParser::GetError() const {
  return m_error;
}

bool JsonPatchParser::IsValid() const {
  return m_error.empty();
}

template <typename T>
void JsonPatchParser::HandleNumber(const T &value) {
  switch (m_state) {
    case TOP:
      SetError(kPatchListError);
      return;
    case PATCH_LIST:
      SetError(kPatchElementError);
      return;
    case PATCH:
      if (m_key == kValueKey) {
        m_value.reset(JsonValue::NewValue(value));
      }
    case VALUE:
      m_parser.Number(value);
    default:
      {}
  }
}

void JsonPatchParser::HandlePatchString(const std::string &value) {
  if (m_key == kOpKey) {
    m_op = value;
  } else if (m_key == kFromKey) {
    m_from.Set(value);
  } else if (m_key == kPathKey) {
    m_path.Set(value);
  } else if (m_key == kValueKey) {
    m_value.reset(new JsonString(value));
  }
}

void JsonPatchParser::HandlePatch() {
  if (!m_path.IsSet()) {
    SetError(kMissingPath);
    return;
  }

  if (m_op == kAddOp) {
    if (!m_value.get()) {
      SetError(kMissingValue);
      return;
    }
    m_patch_set->AddOp(
        new JsonPatchAddOp(JsonPointer(m_path.Value()), m_value.release()));
  } else if (m_op == kRemoveOp) {
    m_patch_set->AddOp(new JsonPatchRemoveOp(JsonPointer(m_path.Value())));
  } else if (m_op == kReplaceOp) {
    if (!m_value.get()) {
      SetError(kMissingValue);
      return;
    }
    m_patch_set->AddOp(
        new JsonPatchReplaceOp(JsonPointer(m_path.Value()), m_value.release()));
  } else if (m_op == kMoveOp) {
    if (!m_from.IsSet()) {
      SetError(kMissingFrom);
      return;
    }
    m_patch_set->AddOp(
        new JsonPatchMoveOp(JsonPointer(m_from.Value()),
                            JsonPointer(m_path.Value())));
  } else if (m_op == kCopyOp) {
    if (!m_from.IsSet()) {
      SetError(kMissingFrom);
      return;
    }
    m_patch_set->AddOp(
        new JsonPatchCopyOp(JsonPointer(m_from.Value()),
                            JsonPointer(m_path.Value())));
  } else if (m_op == kTestOp) {
    if (!m_value.get()) {
      SetError(kMissingValue);
      return;
    }
    m_patch_set->AddOp(
        new JsonPatchTestOp(JsonPointer(m_path.Value()), m_value.release()));
  } else {
    SetError("Invalid or missing 'op'");
  }
}

bool JsonPatchParser::Parse(const std::string &input,
                            JsonPatchSet *patch_set,
                            std::string *error) {
  JsonPatchParser parser(patch_set);
  bool ok = JsonLexer::Parse(input, &parser) && parser.IsValid();
  if (!ok) {
    *error = parser.GetError();
  }
  return ok;
}
}  // namespace web
}  // namespace ola
