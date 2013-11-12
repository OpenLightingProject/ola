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
 * AvahiDiscoveryAgent.h
 * The Avahi implementation of DiscoveryAgentInterface.
 * Copyright (C) 2013 Simon Newton
 */

#ifndef OLAD_AVAHIDISCOVERYAGENT_H_
#define OLAD_AVAHIDISCOVERYAGENT_H_

#include <avahi-client/client.h>

#include <ola/base/Macro.h>
#include <string>

#include "olad/DiscoveryAgent.h"

namespace ola {

/**
 * @class An implementation of DiscoveryAgentInterface that uses the Avahi
 * client library.
 */
class AvahiDiscoveryAgent : public DiscoveryAgentInterface {
  public:
    AvahiDiscoveryAgent();
    ~AvahiDiscoveryAgent();

    bool Init();

    void RegisterService(const std::string &service_name,
                         const std::string &type,
                         uint16_t port,
                         const RegisterOptions &options);

  private:
    DISALLOW_COPY_AND_ASSIGN(AvahiDiscoveryAgent);
};
}  // namespace ola
#endif  // OLAD_AVAHIDISCOVERYAGENT_H_
