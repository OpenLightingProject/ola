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
 * Holds SLP Service Registations.
 * Copyright (C) 2012 Simon Newton
 */

#ifndef TOOLS_SLP_SLPSTORE_H_
#define TOOLS_SLP_SLPSTORE_H_

#include <ola/Clock.h>

#include <map>
#include <string>
#include <vector>

#include "tools/slp/SLPPacketConstants.h"
#include "tools/slp/ScopeSet.h"
#include "tools/slp/ServiceEntry.h"


namespace ola {
namespace slp {

using ola::TimeStamp;
using std::map;
using std::string;
using std::vector;

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
 * We store a map of canonical service name to ServiceList structures. Each
 * ServiceList has a timestamp of when it was last aged / cleaned and a list of
 * ServiceEntries.
 *
 * Clean() should be called periodically to age & remove any entries for
 * services where there have no been any Insert / Lookup requests. Not
 * calling Clean() won't result in incorrect results, but memory use will
 * grow over time.
 *
 * For the E1.33 case we'll have:
 *   - single scope
 *   - two services
 *   - many URLs.
 */
class SLPStore {
  public:
    SLPStore() {}
    ~SLPStore();

    unsigned int ServiceTypeCount() const { return m_services.size(); }

    // Return codes from various methods.
    typedef enum {
      OK,
      SCOPE_MISMATCH,
      NOT_FOUND,
    } ReturnCode;

    slp_error_code_t Insert(const TimeStamp &now, const ServiceEntry &entry);
    slp_error_code_t Insert(const TimeStamp &now, const ServiceEntry &entry,
                      bool fresh);
    ReturnCode Remove(const ServiceEntry &entry);

    void Lookup(const TimeStamp &now,
                const ScopeSet &scopes,
                const string &service_type,
                URLEntries *output,
                unsigned int limit = 0);

    bool ScopesConflict(const TimeStamp &now, const ServiceEntry &service);
    ReturnCode CheckIfScopesMatch(const TimeStamp &now,
                                  const ServiceEntry &service);
    void GetLocalServices(const TimeStamp &now, const ScopeSet &scopes,
                          ServiceEntries *services);

    void Clean(const TimeStamp &now);
    void Reset();
    void Dump(const TimeStamp &now);

  private:
    typedef vector<ServiceEntry*> ServiceEntryVector;
    typedef struct {
      TimeStamp last_cleaned;
      ServiceEntryVector services;
    } ServiceList;
    // map of service-type to a list of ServiceEntries
    typedef map<string, ServiceList*> ServiceMap;

    ServiceMap m_services;

    void MaybeCleanURLList(const TimeStamp &now, ServiceList *service_list);
    ServiceMap::iterator Populate(const TimeStamp &now, const string &service);
    ServiceEntryVector::iterator FindService(ServiceEntryVector *services,
                                             const string &url);
    slp_error_code_t InsertOrUpdateEntry(ServiceEntryVector *services,
                                         const ServiceEntry &service,
                                         bool fresh);
};
}  // slp
}  // ola
#endif  // TOOLS_SLP_SLPSTORE_H_
