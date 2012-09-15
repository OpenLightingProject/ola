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
 * RegistrationFileParser.h
 * Copyright (C) 2012 Simon Newton
 */

#ifndef TOOLS_SLP_REGISTRATIONFILEPARSER_H_
#define TOOLS_SLP_REGISTRATIONFILEPARSER_H_

#include <map>
#include <string>
#include <utility>

#include "tools/slp/URLEntry.h"

namespace ola {
namespace slp {

using std::map;
using std::string;
using std::pair;

/**
 * Parse a registration file and extract the services.
 */
class RegistrationFileParser {
  public:
    RegistrationFileParser() {}
    ~RegistrationFileParser() {}

    // Map of <scope, service> -> urls
    typedef pair<string, string> ScopeServicePair;
    typedef map<ScopeServicePair, URLEntries> ServicesMap;

    bool ParseFile(const string &file, ServicesMap *services) const;

  private:
    void Insert(ServicesMap *services,
                const string &scope,
                const string &service_type,
                const string &url,
                uint16_t lifetime) const;
};
}  // slp
}  // ola
#endif  // TOOLS_SLP_REGISTRATIONFILEPARSER_H_
