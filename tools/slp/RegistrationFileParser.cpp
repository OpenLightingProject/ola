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
 * RegistrationFileParser.cpp
 * Copyright (C) 2012 Simon Newton
 */

#include <errno.h>
#include <ola/Logging.h>
#include <ola/StringUtils.h>
#include <string.h>

#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "tools/slp/RegistrationFileParser.h"
#include "tools/slp/SLPStrings.h"
#include "tools/slp/ScopedSLPStore.h"
#include "tools/slp/URLEntry.h"

namespace ola {
namespace slp {

using std::map;
using std::ifstream;
using std::string;
using std::vector;


/**
 * Parse a registration file and extract the services.
 * TODO(simon): make this 2614 compliant.
 *
 * Format is:
 *   scope,service-type,url,lifetime
 */
bool RegistrationFileParser::ParseFile(const string &filename,
                                       ServicesMap *services) const {
  ifstream services_file(filename.c_str());

  if (!services_file.is_open()) {
    OLA_WARN << "Could not open " << filename << ": " << strerror(errno);
    return false;
  }

  string line;
  while (getline(services_file, line)) {
    StringTrim(&line);
    if (line.empty() || line.at(0) == '#' || line.at(0) == ';')
      continue;

    vector<string> tokens;
    StringSplit(line, tokens, ",");

    if (tokens.size() != 4) {
      OLA_INFO << "Skipping line: " << line;
      continue;
    }

    uint16_t lifetime;
    if (!StringToInt(tokens[3], &lifetime)) {
      OLA_INFO << "Invalid lifetime " << line;
      continue;
    }

    Insert(services, tokens[0], tokens[1], tokens[2], lifetime);
  }
  return true;
}


/**
 * Insert an entry into our service map
 */
void RegistrationFileParser::Insert(ServicesMap *services,
                                    const string &scope,
                                    const string &service_type,
                                    const string &url,
                                    uint16_t lifetime) const {
  string canonical_scope = SLPGetCanonicalString(scope);
  ScopeServicePair p(canonical_scope, service_type);
  URLEntries *urls = &((*services)[p]);
  URLEntry entry(url, lifetime);

  URLEntries::iterator iter = urls->find(entry);
  if (iter == urls->end()) {
    urls->insert(entry);
  } else {
    if (lifetime > iter->Lifetime())
      iter->Lifetime(lifetime);
  }
}
}  // slp
}  // ola
