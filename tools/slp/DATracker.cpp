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
 * DATracker.cpp
 * Copyright (C) 2012 Simon Newton
 */

#include <ola/stl/STLUtils.h>
#include <algorithm>
#include <string>
#include <set>
#include <vector>
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
  STLDeleteValues(&m_new_da_callbacks);
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
    ScopeSet scopes(da_advert.scope_list);

    IPV4Address address;
    if (!AddressFromURL(da_advert.url, &address))
      return;

    if (address != source.Host())
      // just warn about this for now
      OLA_WARN << "Parsed address for " << da_advert.url
               << " does not match source address of " << address;

    InternalDirectoryAgent agent(scopes, da_advert.url, address,
                                 da_advert.boot_timestamp);
    m_agents[da_advert.url] = agent;
    RunCallbacks(agent);
    return;
  }

  // We already knew about this one.
  if (da_advert.boot_timestamp < iter->second.BootTimestamp()) {
    OLA_WARN << "DA at " << da_advert.url << " used an earlier boot timestamp."
             << " Got " << da_advert.boot_timestamp << ", previously had "
             << iter->second.BootTimestamp();
  } else if (da_advert.boot_timestamp > iter->second.BootTimestamp()) {
    OLA_INFO << "DA " << da_advert.url << " has rebooted, boot_timestamp was "
             << iter->second.BootTimestamp() << ", now "
             << da_advert.boot_timestamp;
    iter->second.SetBootTimestamp(da_advert.boot_timestamp);
    RunCallbacks(iter->second);
  } else {
    // boot time is equal, see if the scopes changed.
    ScopeSet new_scopes(da_advert.scope_list);
    if (iter->second.scopes() != new_scopes) {
      OLA_INFO << "DA " << da_advert.url << " changed scopes from " <<
        iter->second.scopes() << " to " << new_scopes;
      iter->second.set_scopes(new_scopes);
      RunCallbacks(iter->second);
    }
  }
}


/**
 * Get a list of all the directory agents we know about.
 * @param output the vector to append into to.
 */
void DATracker::GetDirectoryAgents(vector<DirectoryAgent> *output) {
  for (DAMap::const_iterator iter = m_agents.begin();
       iter != m_agents.end(); ++iter) {
    output->push_back(iter->second);
  }
}


/**
 * For a given set of scopes, get the list of DAs that support at least one of
 * these scopes.
 */
void DATracker::GetDAsForScopes(const ScopeSet &scopes,
                                vector<DirectoryAgent> *output) {
  for (DAMap::const_iterator iter = m_agents.begin();
       iter != m_agents.end(); ++iter) {
    ScopeSet intersection = iter->second.scopes().Intersection(scopes);
    if (!intersection.empty())
      output->push_back(iter->second);
  }
}


/**
 * For the given set of scopes, return the fewest DAs that cover as many scopes
 * as possible.
 */
void DATracker::GetMinimalCoveringList(const ScopeSet &scopes,
                                       vector<DirectoryAgent> *output) {
  // This is a NP-complete problem, see
  // http://en.wikipedia.org/wiki/Set_cover_problem
  // We use a greedy algorithm.
  // We optimize the common case where one DA matches all our scopes.
  ScopeSet scopes_to_cover = scopes;
  DAMap::const_iterator largest_iter;

  while (!scopes_to_cover.empty()) {
    largest_iter = m_agents.end();
    unsigned int max_intersection_count = 0;

    for (DAMap::const_iterator iter = m_agents.begin(); iter != m_agents.end();
         ++iter) {
      unsigned int intersection_count =
        iter->second.scopes().IntersectionCount(scopes_to_cover);

      if (intersection_count == scopes_to_cover.size()) {
        // return quickly
        output->push_back(iter->second);
        return;
      }

      if (intersection_count > max_intersection_count) {
        max_intersection_count = intersection_count;
        largest_iter = iter;
      }
    }

    if (largest_iter == m_agents.end())
      // no more DAs cover these scopes
      break;

    // otherwise we have a DA that covers at least some of the scopes
    output->push_back(largest_iter->second);
    scopes_to_cover.DifferenceUpdate(largest_iter->second.scopes());
  }
}


/**
 * Lookup a DA by URL
 * @param da_url the DA's URL
 * @param da a pointer to a DirectoryAgent object to populate.
 */
bool DATracker::LookupDA(const string &da_url, DirectoryAgent *da) {
  DAMap::const_iterator iter = m_agents.find(da_url);
  if (iter == m_agents.end())
    return false;

  *da = iter->second;
  return true;
}


/**
 * Mark a DA as bad
 */
void DATracker::MarkAsBad(const string &da_url) {
  DAMap::iterator iter = m_agents.find(da_url);
  if (iter == m_agents.end())
    return;

  OLA_INFO << "Marking " << da_url << " as bad";
  m_agents.erase(iter);
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
