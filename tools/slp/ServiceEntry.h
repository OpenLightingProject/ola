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
 * ServiceEntry.h
 * Contains information about a service (similar to whats in a SrvReg message)
 * Copyright (C) 2012 Simon Newton
 */

#ifndef TOOLS_SLP_SERVICEENTRY_H_
#define TOOLS_SLP_SERVICEENTRY_H_

#include <ola/Logging.h>
#include <ola/StringUtils.h>
#include <ostream>
#include <string>
#include <vector>
#include "tools/slp/SLPStrings.h"
#include "tools/slp/ScopeSet.h"
#include "tools/slp/URLEntry.h"

namespace ola {
namespace slp {

using std::ostream;
using std::string;


/**
 * An SLP Service Entry, which is like a URL Entry but also has an associated
 * scopes set.
 */
class ServiceEntry {
  public:
    ServiceEntry(const ServiceEntry &other)
        : m_local(other.m_local),
          m_url(other.m_url),
          m_service_type(other.m_service_type),
          m_scopes(other.m_scopes) {
    }

    /**
     * @param scopes a set of scopes.
     * @param service_type the service-type
     * @param url the service URL
     * @param lifetime the number of seconds this service is valid for
     * @param local true if this service was from a local registration
     */
    ServiceEntry(const ScopeSet &scopes,
                 const string &service_type,
                 const string &url,
                 uint16_t lifetime,
                 bool local = false)
        : m_local(local),
          m_url(url, lifetime),
          m_service_type(service_type),
          m_scopes(scopes) {
    }

    /**
     * Similar to above but this creates the scope set from a string.
     * @param scopes a set of scopes, comma separated
     * @param service_type the service-type
     * @param url the service URL
     * @param lifetime the number of seconds this service is valid for
     * @param local true if this service was from a local registration
     */
    ServiceEntry(const string &scopes,
                 const string &service_type,
                 const string &url,
                 uint16_t lifetime,
                 bool local = false)
        : m_local(local),
          m_url(url, lifetime),
          m_service_type(service_type),
          m_scopes(scopes) {
    }

    /**
     * This constructors derrives the service type from the url.
     * @param scopes a set of scopes, comma separated
     * @param url the service URL
     * @param lifetime the number of seconds this service is valid for
     * @param local true if this service was from a local registration
     */
    ServiceEntry(const ScopeSet &scopes,
                 const string &url,
                 uint16_t lifetime,
                 bool local = false)
        : m_local(local),
          m_url(url, lifetime),
          m_service_type(SLPServiceFromURL(url)),
          m_scopes(scopes) {
    }

    /**
     * This constructors derrives the service type from the url.
     * @param scopes a set of scopes, comma separated
     * @param url the service URL
     * @param lifetime the number of seconds this service is valid for
     * @param local true if this service was from a local registration
     */
    ServiceEntry(const string &scopes,
                 const string &url,
                 uint16_t lifetime,
                 bool local = false)
        : m_local(local),
          m_url(url, lifetime),
          m_service_type(SLPServiceFromURL(url)),
          m_scopes(scopes) {
    }

    ~ServiceEntry() {}

    const URLEntry& url() const { return m_url; }
    URLEntry& mutable_url() { return m_url; }
    // The service-type, without the 'service:' at the beginning
    string service_type() const { return m_service_type; }
    const ScopeSet& scopes() const { return m_scopes; }
    bool local() const { return m_local; }
    void set_local(bool local) { m_local = local; }

    // a shortcut to get the url string, rather than using .url().url()
    string url_string() const { return m_url.url(); }

    bool operator==(const ServiceEntry &other) const {
      return (m_service_type == other.m_service_type && m_url == other.m_url &&
              m_local == other.m_local);
    }

    bool operator!=(const ServiceEntry &other) const {
      return !(*this == other);
    }

    ServiceEntry& operator=(const ServiceEntry &other) {
      if (this != &other) {
        m_local = other.m_local;
        m_url = other.m_url;
        m_service_type = other.m_service_type;
        m_scopes = other.m_scopes;
      }
      return *this;
    }

    string ToString() const {
      std::ostringstream str;
      ToStream(str);
      return str.str();
    }

    void ToStream(ostream &out) const {
      out << m_url << ", [" << m_scopes << "]" << (m_local ? " LOCAL" : "");
    }

    friend ostream& operator<<(ostream &out, const ServiceEntry &service) {
      service.ToStream(out);
      return out;
    }

  private:
    bool m_local;  // true if this service originated locally
    URLEntry m_url;
    string m_service_type;
    ScopeSet m_scopes;
};

// typedef for convenience
typedef std::vector<ServiceEntry> ServiceEntries;
}  // slp
}  // ola
#endif  // TOOLS_SLP_SERVICEENTRY_H_
