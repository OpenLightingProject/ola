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
 * DiscoveryAgent.h
 * The Interface for DNS-SD Registration & Discovery
 * Copyright (C) 2013 Simon Newton
 */

#ifndef OLAD_DISCOVERYAGENT_H_
#define OLAD_DISCOVERYAGENT_H_

#include <stdint.h>
#include <ola/base/Macro.h>
#include <string>
#include <map>

namespace ola {

/**
 * @class The interface to DNS-SD operations like register, browse etc.
 */
class DiscoveryAgentInterface {
  public:
    virtual ~DiscoveryAgentInterface() {}

    virtual bool Init() = 0;

    struct RegisterOptions {
      typedef std::map<std::string, std::string> TxtData;

      int if_index;
      std::string domain;
      std::string txt_record;
      TxtData txt_data;

      RegisterOptions()
          : if_index(0),  // 0 is any
            domain(""),
            txt_record("") {
      }

      RegisterOptions(const RegisterOptions &options)
          : if_index(options.if_index),
            domain(options.domain),
            txt_record(options.txt_record),
            txt_data(options.txt_data) {
      }
    };

    // Think about what sort of error handling we want here
    /**
     * @brief Register a service
     * @param service_name the name of the service
     * @param type the service type
     * @param port the port the service is on
     * @param options extra options that control registration.
     */
    virtual void RegisterService(const std::string &service_name,
                                 const std::string &type,
                                 uint16_t port,
                                 const RegisterOptions &options) = 0;
};

/**
 * @class A Factory which produces implementations of DiscoveryAgentInterface.
 * The exact type of object returns depends on what implementation of DNS-SD was
 * available at build time.
 */
class DiscoveryAgentFactory {
 public:
    DiscoveryAgentFactory() {}

    DiscoveryAgentInterface* New();

 private:
  DISALLOW_COPY_AND_ASSIGN(DiscoveryAgentFactory);
};
}  // namespace ola
#endif  // OLAD_DISCOVERYAGENT_H_
