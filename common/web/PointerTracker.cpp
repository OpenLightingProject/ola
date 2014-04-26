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
 * PointerTracker.h
 * Maintains a Json pointer from a series of parse events.
 * Copyright (C) 2014 Simon Newton
 */

#include "common/web/PointerTracker.h"

#include <string>
#include <vector>

namespace ola {
namespace web {

using std::string;
using std::vector;

void PointerTracker::OpenObject() {
  if (!m_tokens.empty()) {
    Token &token = m_tokens.back();
    if (token.type == TOKEN_ARRAY) {
      token.index++;
      UpdatePath();
    }
  }

  Token token(TOKEN_OBJECT);
  m_tokens.push_back(token);
}

void PointerTracker::SetProperty(const string &property) {
  if (m_tokens.empty()) {
    return;
  }

  Token &token = m_tokens.back();
  if (token.type != TOKEN_OBJECT) {
    return;
  }
  string escaped_property = EscapeString(property);
  token.property = escaped_property;
  UpdatePath();
}

void PointerTracker::CloseObject() {
  if (m_tokens.empty()) {
    return;
  }

  if (m_tokens.back().type == TOKEN_OBJECT) {
    m_tokens.pop_back();
    UpdatePath();
  }
}

void PointerTracker::OpenArray() {
  if (!m_tokens.empty()) {
    Token &token = m_tokens.back();
    if (token.type == TOKEN_ARRAY) {
      token.index++;
      UpdatePath();
    }
  }
  Token token(TOKEN_ARRAY);
  m_tokens.push_back(token);
}

void PointerTracker::CloseArray() {
  if (m_tokens.empty()) {
    return;
  }

  if (m_tokens.back().type == TOKEN_ARRAY) {
    m_tokens.pop_back();
    UpdatePath();
  }
}

void PointerTracker::IncrementIndex() {
  if (m_tokens.empty()) {
    return;
  }

  Token &token = m_tokens.back();
  if (token.type != TOKEN_ARRAY) {
    return;
  }
  token.index++;
  UpdatePath();
}

const string PointerTracker::GetPointer() const {
  return m_cached_path;
}

void PointerTracker::UpdatePath() {
  if (m_tokens.empty()) {
    m_cached_path = "";
  } else {
    m_str.str("");
    vector<Token>::const_iterator iter = m_tokens.begin();
    for (; iter != m_tokens.end(); ++iter) {
      m_str << "/";
      if (iter->type == TOKEN_OBJECT) {
        m_str << iter->property;
      } else if (iter->type == TOKEN_ARRAY) {
        m_str << iter->index;
      }
    }
    m_cached_path = m_str.str();
  }
}

string PointerTracker::EscapeString(const string &input) {
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
}  // namespace web
}  // namespace ola
