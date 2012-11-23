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
 * DATracker.cpp
 * Copyright (C) 2012 Simon Newton
 */

#include <ola/stl/STLUtils.h>
#include <string>
#include <set>
#include "tools/slp/DATracker.h"
#include "tools/slp/SLPStrings.h"
#include "tools/slp/ServerCommon.h"

namespace ola {
namespace slp {

using std::map;
using std::set;
using std::string;

string InitDAServicePrefix() {
  string s(DIRECTORY_AGENT_SERVICE);
  s.append("://");
  return s;
}

const string DATracker::DA_SERVICE_PREFIX = InitDAServicePrefix();


/**
 * Clean up
 */
DATracker::~DATracker() {
  STLDeleteValues(m_new_da_callbacks);
}


/**
 * Add a callback which is run whenever we find a new DA. Ownership of the
 * callback is transferred to the DATracker, but you can hold onto the pointer
 * to remove the callback later.
 */
void DATracker::AddNewDACallback(NewDACallback *callback) {
  m_new_da_callbacks.insert(callback);
}


/**
 * Remove a previously registered callback.
 */
void DATracker::RemoveNewDACallback(NewDACallback *callback) {
  DACallbacks::iterator iter = m_new_da_callbacks.find(callback);
  if (iter != m_new_da_callbacks.end()) {
    delete *iter;
    m_new_da_callbacks.erase(iter);
  }
}


/**
 * Called when we receive a DAAdvert
 */
void DATracker::NewDAAdvert(const DAAdvertPacket &da_advert,
                            const IPV4SocketAddress &source) {
  if (da_advert.error_code)
    return;

  DAMap::iterator iter = m_agents.find(da_advert.url);

  if (!da_advert.boot_timestamp) {
    // the DA is going down.
    OLA_INFO << "DA " << da_advert.url << " is going down";
    if (iter != m_agents.end())
      m_agents.erase(iter);
    return;
  }

  if (iter == m_agents.end()) {
    OLA_INFO << "New DA " << da_advert.url;
    set<string> canonical_scopes;
    SLPReduceList(da_advert.scope_list, &canonical_scopes);

    IPV4Address address;
    if (!AddressFromURL(da_advert.url, &address))
      return;

    if (address != source.Host())
      // just warn about this for now
      OLA_WARN << "Parsed address for " << da_advert.url
               << " does not match source address of " << address;

    InternalDirectoryAgent agent(canonical_scopes, da_advert.url,
                                 address, da_advert.boot_timestamp);
    m_agents[da_advert.url] = agent;
    RunCallbacks(agent);
    return;
  }

  // We already knew about this one.
  OLA_INFO << "got update from DA";
  if (da_advert.boot_timestamp < iter->second.BootTimestamp()) {
    OLA_WARN << "DA at " << da_advert.url << " used an earlier boot timestamp."
             << " Got " << da_advert.boot_timestamp << ", previously had "
             << iter->second.BootTimestamp();
  } else if (da_advert.boot_timestamp > iter->second.BootTimestamp()) {
    iter->second.SetBootTimestamp(da_advert.boot_timestamp);
    RunCallbacks(iter->second);
  }
}


/**
 * Get a list of all the directory agents we know about.
 * @param output the set to insert into to.
 */
void DATracker::GetDirectoryAgents(set<DirectoryAgent> *output) {
  for (DAMap::const_iterator iter = m_agents.begin();
       iter != m_agents.end(); ++iter) {
    output->insert(iter->second);
  }
}


/**
 * Run all the callbacks when there is a new DA.
 */
void DATracker::RunCallbacks(const DirectoryAgent &agent) {
  for (DACallbacks::iterator iter = m_new_da_callbacks.begin();
       iter != m_new_da_callbacks.end(); ++iter) {
    (*iter)->Run(agent);
  }
}


/**
 * Extract the IP address from a DA's URL.
 */
bool DATracker::AddressFromURL(const string &url, IPV4Address *address) {
  static const size_t PREFIX_LENGTH = DA_SERVICE_PREFIX.size();

  bool ok = false;
  if (0 == url.compare(0, PREFIX_LENGTH, DA_SERVICE_PREFIX)) {
    ok = IPV4Address::FromString(url.substr(PREFIX_LENGTH), address);
    if (!ok)
      OLA_WARN << "Failed to extract IP from " << url.substr(PREFIX_LENGTH);
  } else {
    OLA_WARN << url << " did not start with " << DA_SERVICE_PREFIX;
  }
  return ok;
}
}  // slp
}  // ola
