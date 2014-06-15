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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * DiscoveryAgent.cpp
 * The Interface for DNS-SD Registration & Discovery
 * Copyright (C) 2013 Simon Newton
 */
#include "olad/DiscoveryAgent.h"

#if HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_DNSSD
#include "olad/BonjourDiscoveryAgent.h"
#endif

#ifdef HAVE_AVAHI
#include "olad/AvahiDiscoveryAgent.h"
#endif

namespace ola {

DiscoveryAgentInterface* DiscoveryAgentFactory::New() {
#ifdef HAVE_DNSSD
  return new BonjourDiscoveryAgent();
#endif
#ifdef HAVE_AVAHI
  return new AvahiDiscoveryAgent();
#endif
  return NULL;
}
}  // namespace ola
