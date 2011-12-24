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
 * ConfigCommon.h
 * Copyright (C) 2011 Simon Newton
 *
 * Common includes for both the lexer and parser.
 *
 * The contents of %union are copied to config.tab.h and included in lex.yy.c
 * and config.tab.cpp. Since our %union uses strings, vectors and classes like
 * ValueInterval, we need to include these headers in config.tab.h
 *
 * Unfortunately, bison 2.3 (which is what's installed on OS X) pre-dates %code
 * directives so I can't find a way to insert includes into config.tab.h. The
 * alternative is to put everything we need here, and include ConfigCommon.h in
 * both lex.yy.c and config.tab.cpp. It's a bit messy but it works.
 */

#ifndef TOOLS_OLA_TRIGGER_CONFIGCOMMON_H_
#define TOOLS_OLA_TRIGGER_CONFIGCOMMON_H_

#include <string>
#include <vector>
#include "tools/ola_trigger/Action.h"

using std::string;
using std::vector;

typedef vector<ValueInterval*> IntervalList;

#endif  // TOOLS_OLA_TRIGGER_CONFIGCOMMON_H_
