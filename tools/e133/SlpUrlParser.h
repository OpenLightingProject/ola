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
 * SlpUrlParser.h
 * Copyright (C) 2011 Simon Newton
 */

#include <ola/rdm/UID.h>
#include <ola/network/IPV4Address.h>

#include <string>

#ifndef TOOLS_E133_SLPURLPARSER_H_
#define TOOLS_E133_SLPURLPARSER_H_

using std::string;


bool ParseSlpUrl(const string &url,
                 ola::rdm::UID *uid,
                 ola::network::IPV4Address *ip);

#endif  // TOOLS_E133_SLPURLPARSER_H_
