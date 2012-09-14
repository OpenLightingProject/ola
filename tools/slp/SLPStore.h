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

#include <map>
#include <string>
#include <set>

#include "tools/slp/URLEntry.h"


namespace ola {
namespace slp {

using ola::TimeStamp;
using std::map;
using std::string;

/**
 * Holds the registrations for services in a single scope.
 */
class SLPStore {
  public:
    SLPStore() {}
    ~SLPStore();

    void Insert(const TimeStamp &now,
                const string &service,
                const URLEntry &entry);

    void BulkInsert(const TimeStamp &now,
                    const string &service,
                    const URLEntries &urls);

    void Lookup(const TimeStamp &now,
                const string &service,
                URLEntries *output);

    void Clean(const TimeStamp &now);

    void Dump(const TimeStamp &now);

  private:
    typedef struct {
      TimeStamp last_cleaned;
      URLEntries urls;
    } ServiceList;

    typedef map<string, ServiceList*> ServiceMap;

    // for our use, # of services will be small so a map is a better bet.
    ServiceMap m_services;

    void CleanURLList(const TimeStamp &now, ServiceList *service_list);
    ServiceMap::iterator Populate(const TimeStamp &now, const string &service);
    void InsertOrUpdateEntry(URLEntries *entries, const URLEntry &entry);
};
}  // slp
}  // ola
#endif  // TOOLS_SLP_SLPSTORE_H_
