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
 * PointerTracker.cpp
 * Maintains a Json pointer from a series of parse events.
 * Copyright (C) 2014 Simon Newton
 */

#include <string>
#include <vector>

#include "ola/strings/Format.h"
#include "common/web/PointerTracker.h"

namespace ola {
namespace web {

using std::string;
using std::vector;

void PointerTracker::OpenObject() {
  IncrementIndex();

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
  if (token.property_set) {
    m_pointer->Pop();
  } else {
    token.property_set = true;
  }
  m_pointer->Push(property);
}

void PointerTracker::CloseObject() {
  if (m_tokens.empty()) {
    return;
  }

  Token &token = m_tokens.back();
  if (token.type == TOKEN_OBJECT) {
    if (token.property_set) {
      m_pointer->Pop();
    }
    m_tokens.pop_back();
  }
}

void PointerTracker::OpenArray() {
  IncrementIndex();

  Token token(TOKEN_ARRAY);
  m_tokens.push_back(token);
}

void PointerTracker::CloseArray() {
  if (m_tokens.empty()) {
    return;
  }

  if (m_tokens.back().type == TOKEN_ARRAY) {
    if (m_tokens.back().index >= 0) {
      m_pointer->Pop();
    }
    m_tokens.pop_back();
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
  if (token.index >= 0) {
    m_pointer->Pop();
  }
  token.index++;
  m_pointer->Push(ola::strings::IntToString(token.index));
}
}  // namespace web
}  // namespace ola
