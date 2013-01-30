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
 * SLPUtil.cpp
 * Utility functions for SLP
 * Copyright (C) 2012 Simon Newton
 */

#include <string>
#include "tools/slp/SLPUtil.h"

namespace ola {
namespace slp {

using std::string;

string SLPErrorToString(uint16_t error) {
  switch (error) {
    case SLP_OK:
      return "Ok";
    case LANGUAGE_NOT_SUPPORTED:
      return "Language not supported";
    case PARSE_ERROR:
      return "Parse error";
    case INVALID_REGISTRATION:
      return "Invalid registration";
    case SCOPE_NOT_SUPPORTED:
      return "Scope not supported";
    case AUTHENTICATION_UNKNOWN:
      return "Authentication Unknown";
    case AUTHENTICATION_ABSENT:
      return "Authentication Absent";
    case AUTHENTICATION_FAILED:
      return "Authentication Failed";
    case VER_NOT_SUPPORTED:
      return "Version not Supported";
    case INTERNAL_ERROR:
      return "Internal Error";
    case DA_BUSY_NOW:
      return "DA Busy";
    case OPTION_NOT_UNDERSTOOD:
      return "Option not Understood";
    case INVALID_UPDATE:
      return "Invalid Update";
    case MSG_NOT_SUPPORTED:
      return "Message not Supported";
    case REFRESH_REJECTED:
      return "Refresh Rejected";
    default:
      return "Unknown error code";
  }
}
}  // slp
}  // ola
