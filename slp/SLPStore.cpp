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
 * SLPStore.cpp
 * Copyright (C) 2012 Simon Newton
 */

#include <ola/Logging.h>
#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include "ola/stl/STLUtils.h"
#include "slp/SLPStore.h"
#include "slp/SLPStrings.h"


using ola::TimeStamp;
using ola::TimeInterval;
using std::cout;
using std::endl;
using std::map;
using std::pair;
using std::string;

namespace ola {
namespace slp {


SLPStore::~SLPStore() {
  Reset();
}


/**
 * Insert (or update) an entry in the store.
 * @param now the current time
 * @param entry the entry to insert
 * @returns SLP_OK or SCOPE_NOT_SUPPORTED.
 */
slp_error_code_t SLPStore::Insert(const TimeStamp &now,
                                      const ServiceEntry &service) {
  return Insert(now, service, true);
}


/**
 * Insert (or update) an entry in the store.
 * @param now the current time
 * @param entry the entry to insert
 * @returns SLP_OK, INVALID_UPDATE or SCOPE_NOT_SUPPORTED.
 */
slp_error_code_t SLPStore::Insert(const TimeStamp &now,
                                      const ServiceEntry &service,
                                      bool fresh) {
  ServiceMap::iterator iter = m_services.find(service.service_type());
  if (iter == m_services.end())
    iter = Populate(now, service.service_type());
  else
    MaybeCleanURLList(now, iter->second);
  return InsertOrUpdateEntry(&(iter->second->services), service, fresh);
}


/**
 * Remove an entry from the Store.
 * @param entry the ServiceEntry to remove
 * @returns SLP_OK or SCOPE_NOT_SUPPORTED.
 */
slp_error_code_t SLPStore::Remove(const ServiceEntry &service) {
  ServiceMap::iterator iter = m_services.find(service.service_type());
  if (iter == m_services.end())
    return SLP_OK;

  ServiceEntryVector &services = iter->second->services;
  ServiceEntryVector::iterator service_iter = FindService(&services,
                                                          service.url_string());
  if (service_iter == services.end())
    return SLP_OK;

  if ((*service_iter)->scopes() == service.scopes()) {
    delete *service_iter;
    services.erase(service_iter);
    if (services.empty()) {
      delete iter->second;
      m_services.erase(iter);
    }
    return SLP_OK;
  } else {
    return SCOPE_NOT_SUPPORTED;
  }
}


/**
 * Look up entries by service type.
 * @param now the current time
 * @param scopes the scopes to search
 * @param service_type the service name, does not need to be caonicalized.
 * @param output a pointer to a list of ServiceEntries to populate
 * @param limit if non-0, limit the number of entries returned
 */
void SLPStore::Lookup(const TimeStamp &now,
                      const ScopeSet &scopes,
                      const string &raw_service_type,
                      URLEntries *output,
                      unsigned int limit) {
  string service_type = raw_service_type;
  SLPCanonicalizeString(&service_type);
  ServiceMap::iterator iter = m_services.find(service_type);
  if (iter == m_services.end())
    return;

  // TODO(simon): fold this into one loop
  MaybeCleanURLList(now, iter->second);

  ServiceEntryVector::iterator service_iter = iter->second->services.begin();
  for (unsigned int i = 0; service_iter != iter->second->services.end();
       i++, ++service_iter) {
    if (!(*service_iter)->scopes().Intersects(scopes))
      continue;
    if (limit && i >= limit)
      break;
    output->push_back((*service_iter)->url());
  }
}


/**
 * Lookup a service, and check if the scopes match what we have in the store.
 * @returns
 *   NOT_FOUND if the service isn't already in the store
 *   SCOPE_MISMATCH if the scopes don't match what's in the store
 *   OK if it's safe to remove.
 */
SLPStore::ReturnCode SLPStore::CheckIfScopesMatch(const TimeStamp &now,
                                                  const ServiceEntry &service) {
  ServiceMap::iterator iter = m_services.find(service.service_type());
  if (iter == m_services.end())
    return NOT_FOUND;

  int64_t elapsed_seconds = (now - iter->second->last_cleaned).Seconds();
  ServiceEntryVector &services = iter->second->services;

  for (ServiceEntryVector::iterator service_iter = services.begin();
       service_iter != services.end(); ++service_iter) {
    if ((*service_iter)->url_string() != service.url_string())
      continue;
    // we have a match
    if ((*service_iter)->url().lifetime() <= elapsed_seconds)
      return NOT_FOUND;

    return (*service_iter)->scopes() == service.scopes() ? OK : SCOPE_MISMATCH;
  }
  return NOT_FOUND;
}


/**
 * Get the set of all local services in the given scopes.
 */
void SLPStore::GetLocalServices(const TimeStamp &now,
                                const ScopeSet &scopes,
                                ServiceEntries *local_services) {
  ServiceMap::iterator iter = m_services.begin();
  for (; iter != m_services.end(); ++iter) {
    int64_t elapsed_seconds = (now - iter->second->last_cleaned).Seconds();
    ServiceEntryVector &services = iter->second->services;
    for (ServiceEntryVector::iterator service_iter = services.begin();
         service_iter != services.end(); ++service_iter) {
      if (!(*service_iter)->local() ||
          (*service_iter)->url().lifetime() <= elapsed_seconds)
        // skip non-local and expired services
        continue;
      if ((*service_iter)->scopes().Intersects(scopes)) {
        local_services->push_back(**service_iter);
        URLEntry &entry = local_services->back().mutable_url();
        entry.AgeLifetime(elapsed_seconds);
      }
    }
  }
}


/**
 * Get a list of all the service types for particular scopes.
 * This doesn't need to be super accurate so we don't bother checking lifetimes
 * here.
 */
void SLPStore::GetAllServiceTypes(const ScopeSet &scopes,
                                  vector<string> *service_types) {
  ServiceMap::iterator iter = m_services.begin();
  for (; iter != m_services.end(); ++iter) {
    if (AnyServiceMatchesScopes(scopes, iter->second->services))
      service_types->push_back(iter->first);
  }
}


/**
 * Get a list of service types based on a naming authority filter.
 * @param naming_authority the filter to apply. An empty string returns all
 * iana services
 */
void SLPStore::GetServiceTypesByNamingAuth(const string &naming_authority,
                                           const ScopeSet &scopes,
                                           vector<string> *service_types) {
  ServiceMap::iterator iter = m_services.begin();
  for (; iter != m_services.end(); ++iter) {
    const string &service_type = iter->first;
    size_t pos = service_type.find_last_of('.');
    string naming_auth;
    if (pos != string::npos)
      naming_auth = service_type.substr(pos + 1);
    if (naming_auth == naming_authority &&
        AnyServiceMatchesScopes(scopes, iter->second->services))
      service_types->push_back(iter->first);
  }
}


/**
 * Clean out expired entries from the table.
 * @param now the current time
 */
void SLPStore::Clean(const TimeStamp &now) {
  // We may want to clean this in slices.
  for (ServiceMap::iterator iter = m_services.begin();
       iter != m_services.end();) {
    MaybeCleanURLList(now, iter->second);
    if (iter->second->services.empty()) {
      delete iter->second;
      m_services.erase(iter++);
    } else {
      iter++;
    }
  }
}


/**
 * Delete all entries from this store
 */
void SLPStore::Reset() {
  for (ServiceMap::iterator iter = m_services.begin();
        iter != m_services.end(); ++iter) {
    STLDeleteElements(&(iter->second->services));
    delete iter->second;
  }
}


/**
 * Dump out the contents of the store.
 */
void SLPStore::Dump(const TimeStamp &now) {
  for (ServiceMap::iterator iter = m_services.begin();
       iter != m_services.end(); ++iter) {
    MaybeCleanURLList(now, iter->second);

    cout << iter->first << endl;
    const ServiceList *service_list = iter->second;
    ServiceEntryVector::const_iterator service_iter =
      service_list->services.begin();
    for (; service_iter != service_list->services.end(); ++service_iter)
      cout << "  " << **service_iter << endl;
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

  ServiceEntryVector::iterator service_iter = service_list->services.begin();
  while (service_iter != service_list->services.end()) {
    if ((*service_iter)->url().lifetime() <= elapsed_seconds) {
      delete *service_iter;
      service_iter = service_list->services.erase(service_iter);
    } else {
      (*service_iter)->mutable_url().AgeLifetime(elapsed_seconds);
      ++service_iter;
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
 * Find a service with a particular URL in a vector of services. or return NULL
 * Linear with the number of services.
 */
SLPStore::ServiceEntryVector::iterator SLPStore::FindService(
        ServiceEntryVector *services,
        const string &url) {
  ServiceEntryVector::iterator iter = services->begin();
  for (; iter != services->end(); ++iter) {
    if (url == (*iter)->url_string())
      return iter;
  }
  return iter;
}


/**
 * Either insert this entry or update the existing one (if the lifetime is
 * greater).
 * @returns SLP_OK, INVALID_UPDATE or SCOPE_NOT_SUPPORTED.
 */
slp_error_code_t SLPStore::InsertOrUpdateEntry(
    ServiceEntryVector *services,
    const ServiceEntry &service,
    bool fresh) {
  ServiceEntryVector::iterator iter = FindService(services,
                                                  service.url_string());
  if (iter == services->end()) {
    if (!fresh)
      return INVALID_UPDATE;

    services->push_back(new ServiceEntry(service));
    return SLP_OK;
  } else if ((*iter)->scopes() != (service.scopes())) {
    return SCOPE_NOT_SUPPORTED;
  } else {
    if (service.url().lifetime() > (*iter)->url().lifetime())
      (*iter)->mutable_url().set_lifetime(service.url().lifetime());
    return SLP_OK;
  }
}


/**
 * Return true if any of the services in the service vector match any of the
 * scopes in the scope list.
 */
bool SLPStore::AnyServiceMatchesScopes(const ScopeSet &scopes,
                                       const ServiceEntryVector &services) {
  for (ServiceEntryVector::const_iterator service_iter = services.begin();
       service_iter != services.end(); ++service_iter) {
    if ((*service_iter)->scopes().Intersects(scopes))
      return true;
  }
  return false;
}
}  // namespace namespace slp
}  // namespace namespace ola
