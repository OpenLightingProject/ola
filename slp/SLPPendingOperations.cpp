/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * SLPPendingOperations.cpp
 * Contains classes that store state for network requests.
 * Copyright (C) 2012 Simon Newton
 */

#include <ola/network/IPV4Address.h>
#include <map>
#include <string>
#include "slp/SLPPendingOperations.h"

using std::map;
using std::string;

namespace ola {
namespace slp {

PendingSrvRqst::PendingSrvRqst(
  const string &service_type,
  const ScopeSet &scopes,
  BaseCallback1<void, const URLEntries&> *callback)
    : service_type(service_type),
      callback(callback) {
    for (ScopeSet::Iterator iter = scopes.begin(); iter != scopes.end(); ++iter)
      scope_status_map[*iter] = PENDING;
}


bool PendingSrvRqst::Complete() const {
  for (ScopeStatusMap::const_iterator iter = scope_status_map.begin();
       iter != scope_status_map.end(); ++iter) {
    if (iter->second != PendingSrvRqst::COMPLETE)
      return false;
  }
  return true;
}
}  // slp
}  // ola
