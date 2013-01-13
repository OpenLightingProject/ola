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
 * ServerCommon.h
 * Constants for the SLP Server.
 * Copyright (C) 2012 Simon Newton
 */

#ifndef TOOLS_SLP_SERVERCOMMON_H_
#define TOOLS_SLP_SERVERCOMMON_H_

#include <stdint.h>

namespace ola {
namespace slp {

// The default SLP port from RFC 2608
static const int DEFAULT_SLP_PORT = 427;

// These are all in seconds, values are from the RFC
static const uint16_t CONFIG_DA_BEAT = 3 * 60 * 60;
static const uint16_t CONFIG_DA_FIND = 900;
static const uint16_t CONFIG_MC_MAX  = 15;
static const uint16_t CONFIG_RETRY = 2;
static const uint16_t CONFIG_RETRY_MAX = 15;
static const uint16_t CONFIG_START_WAIT = 3;
static const uint16_t CONFIG_REG_ACTIVE_MIN = 1;
static const uint16_t CONFIG_REG_ACTIVE_MAX = 3;

static const char DIRECTORY_AGENT_SERVICE[] = "service:directory-agent";

static const char SERVICE_AGENT_SERVICE[] = "service:service-agent";
}  // slp
}  // ola
#endif  // TOOLS_SLP_SERVERCOMMON_H_
