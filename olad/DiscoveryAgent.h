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
 * @brief The interface to DNS-SD operations like register, browse etc.
 */
class DiscoveryAgentInterface {
 public:
  virtual ~DiscoveryAgentInterface() {}

  /**
   * @brief Initialize the DiscoveryAgent.
   */
  virtual bool Init() = 0;

  /**
   * @brief Options for the RegisterService method
   *
   * This controls options like the interface index, domain and TXT record
   * data.
   */
  struct RegisterOptions {
    /**
     * @typedef TxtData
     * The data type that stores the key : values for the TXT record.
     */
    typedef std::map<std::string, std::string> TxtData;

    /**
     * @brief A constant which represents all Interfaces.
     */
    static const int ALL_INTERFACES = 0;

    int if_index;  /**< The interface index to register on */
    /**
     * @brief The domain to use.
     *
     * The empty string uses the system default domain.
     */
    std::string domain;
    TxtData txt_data;   /**< The TXT record data. */

    RegisterOptions()
        : if_index(ALL_INTERFACES),
          domain("") {
    }

    RegisterOptions(const RegisterOptions &options)
        : if_index(options.if_index),
          domain(options.domain),
          txt_data(options.txt_data) {
    }
  };

  /**
   * @brief Register a service
   * @param service_name the name of the service
   * @param type the service type
   * @param port the port the service is on
   * @param options extra options that control registration.
   */
  // TODO(simon): Think about what sort of error handling we want here
  virtual void RegisterService(const std::string &service_name,
                               const std::string &type,
                               uint16_t port,
                               const RegisterOptions &options) = 0;
};

/**
 * @brief A Factory which produces implementations of DiscoveryAgentInterface.
 *
 * The exact type of object returns depends on what implementation of DNS-SD was
 * available at build time.
 */
class DiscoveryAgentFactory {
 public:
  DiscoveryAgentFactory() {}

  /**
   * @brief Create a new DiscoveryAgent.
   *
   * This returns a DiscoveryAgent appropriate for the platform. It can
   * either be a BonjourDiscoveryAgent or a AvahiDiscoveryAgent.
   */
  DiscoveryAgentInterface* New();

 private:
  DISALLOW_COPY_AND_ASSIGN(DiscoveryAgentFactory);
};
}  // namespace ola
#endif  // OLAD_DISCOVERYAGENT_H_
