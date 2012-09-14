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
#include <string>
#include <utility>
#include "tools/slp/SLPStore.h"


using ola::TimeStamp;
using ola::TimeInterval;
using std::map;
using std::string;
using std::pair;

namespace ola {
namespace slp {


SLPStore::~SLPStore() {
  ServiceMap::iterator iter = m_services.begin();
  for (; iter != m_services.end(); ++iter)
    delete iter->second;
}


/**
 * Insert (or update) an entry in the store
 */
void SLPStore::Insert(const TimeStamp &now,
                      const string &service,
                      const URLEntry &entry) {
  ServiceMap::iterator iter = m_services.find(service);
  if (iter == m_services.end())
    iter = Populate(now, service);
  else
    CleanURLList(now, iter->second);
  InsertOrUpdateEntry(&(iter->second->urls), entry);
}


/**
 * Insert a set of URLEntries into the store
 */
void SLPStore::BulkInsert(const TimeStamp &now,
                          const string &service,
                          const URLEntries &urls) {
  ServiceMap::iterator iter = m_services.find(service);
  if (iter == m_services.end())
    iter = Populate(now, service);
  else
    CleanURLList(now, iter->second);

  URLEntries::const_iterator url_iter = urls.begin();
  for (; url_iter != urls.end(); ++url_iter)
    InsertOrUpdateEntry(&(iter->second->urls), *url_iter);
}


/**
 * Look up entries by service type.
 */
void SLPStore::Lookup(const TimeStamp &now,
                      const string &service,
                      URLEntries *output) {
  ServiceMap::iterator iter = m_services.find(service);
  if (iter == m_services.end())
    return;

  CleanURLList(now, iter->second);

  URLEntries::iterator url_iter = iter->second->urls.begin();
  for (; url_iter != iter->second->urls.end(); ++url_iter)
    output->insert(*url_iter);
}


/**
 * Clean out expired entries from the table.
 */
void SLPStore::Clean(const TimeStamp &now) {
  // We may want to clean this in slices.
  ServiceMap::iterator iter = m_services.begin();
  while (iter != m_services.end()) {
    ServiceMap::iterator current = iter;
    ++iter;
    CleanURLList(now, current->second);
    if (current->second->urls.empty())
      m_services.erase(current);
  }
}


/**
 * Dump out the contents of the store.
 */
void SLPStore::Dump(const TimeStamp &now) {
  ServiceMap::iterator iter = m_services.begin();

  for (; iter != m_services.end(); ++iter) {
    CleanURLList(now, iter->second);

    OLA_INFO << iter->first;
    const ServiceList *service_list = iter->second;
    URLEntries::const_iterator url_iter = service_list->urls.begin();
    for (; url_iter != service_list->urls.end(); ++url_iter) {
      OLA_INFO << "  " << *url_iter;
    }
  }
}


/**
 * Age the list of URLEntries and remove expired entires
 */
void SLPStore::CleanURLList(const TimeStamp &now, ServiceList *service_list) {
  int64_t elapsed_seconds = (now - service_list->last_cleaned).Seconds();
  URLEntries::iterator url_iter = service_list->urls.begin();

  while (url_iter != service_list->urls.end()) {
    URLEntries::iterator current = url_iter;
    url_iter++;
    if (current->Lifetime() <= elapsed_seconds) {
      service_list->urls.erase(current);
    } else {
      current->Lifetime(current->Lifetime() - elapsed_seconds);
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
 * Either insert this antry of update the existing one (if the lifetime is
 * greater).
 */
void SLPStore::InsertOrUpdateEntry(URLEntries *entries, const URLEntry &entry) {
  URLEntries::iterator url_iter = entries->find(entry);
  if (url_iter == entries->end()) {
    entries->insert(entry);
  } else {
    if (entry.Lifetime() > url_iter->Lifetime())
      url_iter->Lifetime(entry.Lifetime());
  }
}
}  // slp
}  // ola
