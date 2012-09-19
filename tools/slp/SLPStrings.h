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
 * SLPString.h
 * Utility functions for dealing with strings & scopes in SLP.
 * Copyright (C) 2012 Simon Newton
 */

#ifndef TOOLS_SLP_SLPSTRINGS_H_
#define TOOLS_SLP_SLPSTRINGS_H_

#include <set>
#include <string>
#include <vector>

namespace ola {
namespace slp {

using std::set;
using std::string;
using std::vector;

void SLPStringEscape(string *str);
void SLPStringUnescape(string *str);
void SLPCanonicalizeString(string *str);
string SLPGetCanonicalString(const string &str);
bool SLPStringCanonicalizeAndCompare(const string &s1, const string s2);
bool SLPSetIntersect(const set<string> &one, const set<string> &two);
void SLPReduceList(const vector<string> &input, set<string> *output);
bool SLPScopesMatch(const vector<string> &scopes_v,
                    const set<string> &scopes_s);
void SLPStripService(string *str);
}  // slp
}  // ola
#endif  // TOOLS_SLP_SLPSTRINGS_H_
