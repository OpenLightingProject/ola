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
 * AvahiDiscoveryAgent.cpp
 * The Bonjour implementation of DiscoveryAgentInterface.
 * Copyright (C) 2013 Simon Newton
 */

#include "olad/AvahiDiscoveryAgent.h"

#include <memory>
#include <string>

namespace ola {

using std::auto_ptr;
using std::string;


AvahiDiscoveryAgent::AvahiDiscoveryAgent() {
}

AvahiDiscoveryAgent::~AvahiDiscoveryAgent() {
}

bool AvahiDiscoveryAgent::Init() {
  return true;
}

void AvahiDiscoveryAgent::RegisterService(const string &service_name,
                                          const string &type,
                                          uint16_t port,
                                          const RegisterOptions &options) {
  (void) service_name;
  (void) type;
  (void) port;
  (void) options;
}
}  // namespace ola
