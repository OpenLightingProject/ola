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
 * SLPStore.h
 * Copyright (C) 2012 Simon Newton
 */

#ifndef TOOLS_SLP_SLPSTORE_H_
#define TOOLS_SLP_SLPSTORE_H_

#include <ola/Clock.h>
#include <ola/io/BigEndianStream.h>

#include <map>
#include <string>
#include <set>

#include "tools/slp/ServiceEntry.h"
#include "tools/slp/SLPStrings.h"


namespace ola {
namespace slp {

using ola::TimeStamp;
using std::map;
using std::set;
using std::string;

/**
 * Holds the registrations for services and ages & cleans them as appropriate.
 * Each service registration has an associated lifetime (age). Openslp ages the
 * entire database every 15 seconds which doesn't provide a good client
 * experience.
 *
 * We take a different approach, by opportunistically aging the database
 * whenever insert or lookup is called. If it's been more than
 * a second since the last aging event for a service, we age all entries.
 *
 *  We store a map of canonical service name to ServiceList structures. Each
 *  ServiceList has a timestamp of when it was last aged / cleaned and a set of
 *  ServiceEntries.
 *
 * To be correct, whil
 *  amortizes the cost of aging
 *
 * Clean() should be called periodically to age & remove any entries for
 * services where there have no been any Insert/Remove/Lookup requests. Not
 * calling Clean() won't result in incorrect results, rather memory use will
 * grow over time.
 *
 * For E1.33 we'll have:
 *   - single scope
 *   - two services
 *   - many URLs.
 */
class SLPStore {
  public:
    SLPStore() {}
    ~SLPStore();

    // Return codes from the Insert & Lookup methods.
    typedef enum {
      OK,
      SCOPE_MISMATCH
    } ReturnCode;

    ReturnCode Insert(const TimeStamp &now, const ServiceEntry &entry);

    ReturnCode Remove(const ServiceEntry &entry);

    bool BulkInsert(const TimeStamp &now, const ServiceEntries &services);

    // We provide two lookup methods, the first returns ServiceEntries, the
    // second returns URLEntries. You should almost always use the latter.
    void Lookup(const TimeStamp &now,
                const set<string> &scopes,
                const string &service,
                ServiceEntries *output,
                unsigned int limit = 0);
    void Lookup(const TimeStamp &now,
                 const set<string> &scopes,
                 const string &service,
                 URLEntries *output,
                 unsigned int limit = 0);

    void Clean(const TimeStamp &now);

    void Reset();

    void Dump(const TimeStamp &now);

    unsigned int ServiceCount() const {
      return m_services.size();
    }

  private:
    typedef struct {
      TimeStamp last_cleaned;
      ServiceEntries entries;
    } ServiceList;

    typedef map<string, ServiceList*> ServiceMap;

    // for our use, # of services will be small so a map is a better bet.
    ServiceMap m_services;

    void MaybeCleanURLList(const TimeStamp &now, ServiceList *service_list);
    ServiceMap::iterator Populate(const TimeStamp &now, const string &service);
    ReturnCode InsertOrUpdateEntry(ServiceEntries *entries,
                                   const ServiceEntry &entry);
    template<typename Container>
    void InternalLookup(const TimeStamp &now,
                        const set<string> &scopes,
                        const string &service,
                        Container *output,
                        unsigned int limit);
};
}  // slp
}  // ola
#endif  // TOOLS_SLP_SLPSTORE_H_
