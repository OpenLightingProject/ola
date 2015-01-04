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
 * JsonPointer.cpp
 * An implementation of RFC 6901.
 * Copyright (C) 2014 Simon Newton
 */

#include <string>
#include "ola/web/JsonPointer.h"
#include "ola/StringUtils.h"

namespace ola {
namespace web {

using std::string;
using std::vector;

JsonPointer::JsonPointer()
    : m_is_valid(true) {
}

JsonPointer::JsonPointer(const JsonPointer &other)
    : m_is_valid(other.m_is_valid),
      m_tokens(other.m_tokens) {
}

JsonPointer::JsonPointer(const string &path)
    : m_is_valid(true) {
  if (path.empty()) {
    return;
  }

  if (path[0] != '/') {
    m_is_valid = false;
    return;
  }

  Tokens escaped_tokens;
  StringSplit(path.substr(1), &escaped_tokens, "/");

  Tokens::const_iterator iter = escaped_tokens.begin();
  for (; iter != escaped_tokens.end(); ++iter) {
    m_tokens.push_back(UnEscapeString(*iter));
  }
}

bool JsonPointer::operator==(const JsonPointer &other) const {
  return m_tokens == other.m_tokens;
}

void JsonPointer::Push(const string &token) {
  m_tokens.push_back(token);
}

void JsonPointer::Pop() {
  if (!m_tokens.empty()) {
    m_tokens.pop_back();
  }
}

string JsonPointer::ToString() const {
  string path;
  if (!m_tokens.empty()) {
    Tokens::const_iterator iter = m_tokens.begin();
    path.push_back('/');
    while (iter != m_tokens.end()) {
      path.append(EscapeString(*iter++));
      if (iter != m_tokens.end()) {
        path.push_back('/');
      }
    }
  }
  return path;
}

bool JsonPointer::IsPrefixOf(const JsonPointer &other) const {
  if (!(IsValid() && other.IsValid())) {
    return false;
  }

  Tokens::const_iterator our_iter = m_tokens.begin();
  Tokens::const_iterator other_iter = other.m_tokens.begin();

  for (; our_iter != m_tokens.end() && other_iter != other.m_tokens.end();
       our_iter++, other_iter++) {
    if (*our_iter != *other_iter) {
      return false;
    }
  }
  return other_iter != other.m_tokens.end();
}

string JsonPointer::EscapeString(const string &input) {
  string escaped_property;
  escaped_property.reserve(input.size());
  for (string::const_iterator iter = input.begin(); iter != input.end();
       ++iter) {
    switch (*iter) {
      case '~':
        escaped_property.push_back(*iter);
        escaped_property.push_back('0');
        break;
      case '/':
        escaped_property.push_back('~');
        escaped_property.push_back('1');
        break;
      default:
        escaped_property.push_back(*iter);
    }
  }
  return escaped_property;
}

string JsonPointer::UnEscapeString(const string &input) {
  string token = input;
  size_t pos = 0;
  // Section 4 of the RFC explains why we do it in this order.
  while ((pos = token.find("~1")) != string::npos) {
    token[pos] = '/';
    token.erase(pos + 1, 1);
  }
  while ((pos = token.find("~0")) != string::npos) {
    token[pos] = '~';
    token.erase(pos + 1, 1);
  }
  return token;
}
}  // namespace web
}  // namespace ola
