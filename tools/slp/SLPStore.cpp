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
 * SLPStore.cpp
 * Copyright (C) 2012 Simon Newton
 */

#include <ola/Logging.h>
#include <set>
#include <string>
#include <utility>
#include "ola/stl/STLUtils.h"
#include "tools/slp/SLPStore.h"
#include "tools/slp/SLPStrings.h"


using ola::TimeStamp;
using ola::TimeInterval;
using std::map;
using std::string;
using std::pair;

namespace ola {
namespace slp {


SLPStore::~SLPStore() {
  STLDeleteValues(m_services);
}


/**
 * Insert (or update) an entry in the store.
 * @param now the current time
 * @param entry the entry to insert
 * @returns either OK or SCOPE_MISMATCH
 */
SLPStore::ReturnCode SLPStore::Insert(const TimeStamp &now,
                                      const ServiceEntry &entry) {
  string service = SLPServiceFromURL(entry.URL());
  ServiceMap::iterator iter = m_services.find(service);
  if (iter == m_services.end())
    iter = Populate(now, service);
  else
    MaybeCleanURLList(now, iter->second);
  return InsertOrUpdateEntry(&(iter->second->entries), entry);
}


/**
 * Remove an entry from the Store.
 * @param entry the ServiceEntry to remove
 * @returns SCOPE_MISMATCH if the scopes do not match the scopes that the entry
 *   was registered with. Otherwise return OK.
 */
SLPStore::ReturnCode SLPStore::Remove(const ServiceEntry &entry) {
  ServiceMap::iterator iter = m_services.find(SLPServiceFromURL(entry.URL()));
  if (iter == m_services.end())
    return OK;

  ServiceEntries &entries = iter->second->entries;
  ServiceEntries::iterator service_iter = entries.find(entry);
  if (service_iter == entries.end())
    return OK;

  if (service_iter->MatchesScopes(entry.Scopes())) {
    entries.erase(service_iter);
    if (entries.empty()) {
      delete iter->second;
      m_services.erase(iter);
    }
    return OK;
  } else {
    return SCOPE_MISMATCH;
  }
}


/**
 * Insert a set of URLEntries into the store. This assumes that all
 * ServiceEntries have the same canonical service.
 * @param now the current time
 * @param services the list of ServiceEntries to insert
 * @returns true if all ServiceEntries were added, false if one or more was
 *   skipped.
 */
bool SLPStore::BulkInsert(const TimeStamp &now,
                             const ServiceEntries &services) {
  if (services.empty())
    return OK;
  bool ok = true;

  // use the service from the first entry
  ServiceEntries::const_iterator service_iter = services.begin();
  string service = SLPServiceFromURL(service_iter->URL());

  ServiceMap::iterator iter = m_services.find(service);
  if (iter == m_services.end())
    iter = Populate(now, service);
  else
    MaybeCleanURLList(now, iter->second);

  for (; service_iter != services.end(); ++service_iter) {
    string this_service = SLPServiceFromURL(service_iter->URL());
    if (this_service == service) {
      InsertOrUpdateEntry(&(iter->second->entries), *service_iter);
    } else {
      OLA_WARN << "Service for " << service_iter->URL() << " does not match "
               << service;
      ok = false;
    }
  }
  return ok;
}


/**
 * Look up entries by service type.
 * @param now the current time
 * @param scopes the scopes to search
 * @param service the service name, does not need to be caonicalized.
 * @param output a pointer to a list of ServiceEntries to populate
 * @param limit if non-0, limit the number of entries returned
 */
void SLPStore::Lookup(const TimeStamp &now,
                      const set<string> &scopes,
                      const string &service,
                      ServiceEntries *output,
                      unsigned int limit) {
  InternalLookup(now, scopes, service, output, limit);
}


/**
 * Look up entries by service type.
 * @param now the current time
 * @param scopes the scopes to search
 * @param service the service name, does not need to be caonicalized.
 * @param output a pointer to a list of ServiceEntries to populate
 * @param limit if non-0, limit the number of entries returned
 */
void SLPStore::Lookup(const TimeStamp &now,
                      const set<string> &scopes,
                      const string &service,
                      URLEntries *output,
                      unsigned int limit) {
  InternalLookup(now, scopes, service, output, limit);
}


/**
 * Clean out expired entries from the table.
 * @param now the current time
 */
void SLPStore::Clean(const TimeStamp &now) {
  // We may want to clean this in slices.
  ServiceMap::iterator iter = m_services.begin();
  while (iter != m_services.end()) {
    ServiceMap::iterator current = iter;
    ++iter;
    MaybeCleanURLList(now, current->second);
    if (current->second->entries.empty()) {
      delete current->second;
      m_services.erase(current);
    }
  }
}


/**
 * Delete all entries from this store
 */
void SLPStore::Reset() {
  STLDeleteValues(m_services);
}


/**
 * Dump out the contents of the store.
 */
void SLPStore::Dump(const TimeStamp &now) {
  ServiceMap::iterator iter = m_services.begin();

  for (; iter != m_services.end(); ++iter) {
    MaybeCleanURLList(now, iter->second);

    OLA_INFO << iter->first;
    const ServiceList *service_list = iter->second;
    ServiceEntries::const_iterator service_iter = service_list->entries.begin();
    for (; service_iter != service_list->entries.end(); ++service_iter) {
      OLA_INFO << "  " << *service_iter;
    }
  }
}


/**
 * Age the list of URLEntries and remove expired entries if more than a second
 * has elapsed since the last cleaning time.
 */
void SLPStore::MaybeCleanURLList(const TimeStamp &now,
                                 ServiceList *service_list) {
  int64_t elapsed_seconds = (now - service_list->last_cleaned).Seconds();
  if (elapsed_seconds == 0)
    return;

  ServiceEntries::iterator service_iter = service_list->entries.begin();

  while (service_iter != service_list->entries.end()) {
    ServiceEntries::iterator current = service_iter;
    service_iter++;
    if (current->Lifetime() <= elapsed_seconds) {
      service_list->entries.erase(current);
    } else {
      current->SetLifetime(current->Lifetime() - elapsed_seconds);
    }
  }
  service_list->last_cleaned = now;
}


/**
 * Setup the ServiceList for a particular service
 */
SLPStore::ServiceMap::iterator SLPStore::Populate(const TimeStamp &now,
                                                  const string &service) {
  ServiceList *service_list = new ServiceList;
  service_list->last_cleaned = now;
  return m_services.insert(
      pair<string, ServiceList*>(service, service_list)).first;
}


/**
 * Either insert this entry or update the existing one (if the lifetime is
 * greater).
 * @returns Either OK or SCOPE_MISMATCH.
 */
SLPStore::ReturnCode SLPStore::InsertOrUpdateEntry(
    ServiceEntries *entries,
    const ServiceEntry &entry) {
  ServiceEntries::iterator iter = entries->find(entry);
  if (iter == entries->end()) {
    entries->insert(entry);
    return OK;
  } else if (!iter->MatchesScopes(entry.Scopes())) {
    return SCOPE_MISMATCH;
  } else {
    if (entry.Lifetime() > iter->Lifetime())
      iter->SetLifetime(entry.Lifetime());
    return OK;
  }
}


template<typename Container>
void SLPStore::InternalLookup(const TimeStamp &now,
                              const set<string> &scopes,
                              const string &service,
                              Container *output,
                              unsigned int limit) {
  ServiceMap::iterator iter = m_services.find(SLPServiceFromURL(service));
  if (iter == m_services.end())
    return;

  // TODO(simon): fold this into one loop
  MaybeCleanURLList(now, iter->second);

  ServiceEntries::iterator service_iter = iter->second->entries.begin();
  for (unsigned int i = 0; service_iter != iter->second->entries.end();
       i++, ++service_iter) {
    if (!service_iter->IntersectsScopes(scopes))
      continue;
    if (limit && i >= limit)
      break;
    output->insert(*service_iter);
  }
}
}  // slp
}  // ola
