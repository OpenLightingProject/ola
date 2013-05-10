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
 * RegistrationFileParser.cpp
 * Copyright (C) 2012 Simon Newton
 */

#include <errno.h>
#include <ola/Logging.h>
#include <ola/StringUtils.h>
#include <string.h>

#include <fstream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "slp/RegistrationFileParser.h"
#include "slp/SLPStrings.h"
#include "slp/ServiceEntry.h"

namespace ola {
namespace slp {

using std::ifstream;
using std::map;
using std::set;
using std::string;
using std::vector;


/**
 * Parse a registration file and extract the services.
 * TODO(simon): make this 2614 compliant.
 *
 * Format is:
 *   scope1,scope2  url  lifetime
 */
bool RegistrationFileParser::ParseFile(const string &filename,
                                       ServiceEntries *services) const {
  ifstream services_file(filename.c_str());

  if (!services_file.is_open()) {
    OLA_WARN << "Could not open " << filename << ": " << strerror(errno);
    return false;
  }

  bool result = ParseStream(&services_file, services);
  services_file.close();
  return result;
}


/**
 * Same as ParseFile but this takes an istream.
 */
bool RegistrationFileParser::ParseStream(std::istream *input,
                                         ServiceEntries *services) const {
  // The set of URLS we've already seen, stored in canonical form
  set<string> seen_urls;

  string line;
  while (getline(*input, line)) {
    OLA_INFO << line;
    StringTrim(&line);
    if (line.empty() || line.at(0) == '#' || line.at(0) == ';')
      continue;

    vector<string> tokens;
    SplitLine(line, &tokens);

    if (tokens.size() < 3) {
      OLA_INFO << "Skipping line: " << line;
      continue;
    }

    uint16_t lifetime;
    if (!StringToInt(tokens[2], &lifetime)) {
      OLA_INFO << "Invalid lifetime " << line;
      continue;
    }

    const string scopes = tokens[0];
    const string url = tokens[1];

    if (seen_urls.find(url) != seen_urls.end()) {
      OLA_WARN << url
               << " appears more than once in service registration file";
      continue;
    }
    seen_urls.insert(url);

    ServiceEntry service(scopes, url, lifetime);
    services->push_back(service);
  }
  return true;
}


void RegistrationFileParser::SplitLine(const string &line,
                                       vector<string> *tokens) const {
  string::size_type start_offset = 0;
  string::size_type end_offset = 0;

  while (end_offset != string::npos) {
    end_offset = line.find_first_of(" \t", start_offset);
    string token;
    if (end_offset == string::npos)
      token = line.substr(start_offset, line.size() - start_offset);
    else
      token = line.substr(start_offset, end_offset - start_offset);
    start_offset = end_offset + 1 > line.size() ? string::npos :
                   end_offset + 1;
    if (!token.empty())
      tokens->push_back(token);
  }
}
}  // namespace slp
}  // namespace ola
