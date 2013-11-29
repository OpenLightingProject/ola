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
 * RegistrationFileParser.h
 * Copyright (C) 2012 Simon Newton
 */

#ifndef SLP_REGISTRATIONFILEPARSER_H_
#define SLP_REGISTRATIONFILEPARSER_H_

#include <istream>
#include <string>
#include <vector>

#include "slp/ServiceEntry.h"

namespace ola {
namespace slp {

using std::string;

/**
 * Parse a registration file and extract the services.
 */
class RegistrationFileParser {
 public:
    RegistrationFileParser() {}
    ~RegistrationFileParser() {}

    bool ParseFile(const string &file, ServiceEntries *services) const;
    bool ParseStream(std::istream *input, ServiceEntries *services) const;

 private:
    void SplitLine(const string &line, std::vector<string> *tokens) const;
};
}  // namespace slp
}  // namespace ola
#endif  // SLP_REGISTRATIONFILEPARSER_H_
