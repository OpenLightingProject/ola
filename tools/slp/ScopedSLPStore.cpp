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
 * ScopedSLPStore.h
 * Copyright (C) 2012 Simon Newton
 */

#include <ola/StringUtils.h>
#include <map>
#include <string>
#include <utility>

#include "tools/slp/SLPStore.h"
#include "tools/slp/SLPStrings.h"
#include "tools/slp/ScopedSLPStore.h"


namespace ola {
namespace slp {

using std::string;
using ola::ToUpper;

/**
 * Clean up.
 */
ScopedSLPStore::~ScopedSLPStore() {
  ScopedServiceMap::iterator iter = m_scopes.begin();
  for (; iter != m_scopes.end(); ++iter) {
    delete iter->second;
  }
}


/**
 * Lookup a SLPStore by scope. This creates a new store if it doesn't already
 * exist.
 */
SLPStore* ScopedSLPStore::LookupOrCreate(const string &scope) {
  string canonical_scope = SLPGetCanonicalString(scope);
  ScopedServiceMap::iterator iter = m_scopes.find(canonical_scope);
  if (iter != m_scopes.end())
    return iter->second;

  return m_scopes.insert(
    std::pair<string, SLPStore*>(canonical_scope,
                                 new SLPStore())).first->second;
}


SLPStore* ScopedSLPStore::Lookup(const string &scope) {
  ScopedServiceMap::iterator iter = m_scopes.find(SLPGetCanonicalString(scope));
  return (iter == m_scopes.end() ? NULL : iter->second);
}
}  // slp
}  // ola
