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

#include <ola/Clock.h>
#include <ola/Logging.h>
#include <ola/StringUtils.h>
#include <ola/io/BigEndianStream.h>
#include <ola/network/IPV4Address.h>
#include <map>
#include <ostream>
#include <set>
#include <string>
#include <vector>
#include "tools/slp/SLPStrings.h"
#include "tools/slp/ScopeSet.h"

namespace ola {
namespace slp {

using ola::TimeStamp;
using ola::network::IPV4Address;
using std::ostream;
using std::map;
using std::set;
using std::string;
using std::vector;

/**
 * Represents a URL with the an associated lifetime. The URL cannot be changed
 * once the object is created. This object is cheap to copy so it can be used
 * in STL containers. It doesn't have an ordering defined though.
 */
class URLEntry {
  public:
    URLEntry() {}

    URLEntry(const string &url, uint16_t lifetime)
        : m_url(url),
          m_lifetime(lifetime) {
    }

    ~URLEntry() {}

    string url() const { return m_url; }
    uint16_t lifetime() const { return m_lifetime; }
    void set_lifetime(uint16_t lifetime) { m_lifetime = lifetime; }

    // Return the total size of this URL entry
    unsigned int PackedSize() const { return 6 + m_url.size(); }

    // Write this ServiceEntry to an IOQueue
    void Write(ola::io::BigEndianOutputStreamInterface *output) const;

    URLEntry& operator=(const URLEntry &other) {
      if (this != &other) {
        m_url = other.m_url;
        m_lifetime = other.m_lifetime;
      }
      return *this;
    }

    void ToStream(ostream &out) const {
      out << m_url << "(" << m_lifetime << ")";
    }

    string ToString() const {
      std::ostringstream str;
      ToStream(str);
      return str.str();
    }

    friend ostream& operator<<(ostream &out, const URLEntry &entry) {
      entry.ToStream(out);
      return out;
    }

  protected:
    string m_url;
    uint16_t m_lifetime;
    // TODO(simon): add auth blocks here
};


// typedef for convenience
typedef std::vector<URLEntry> URLEntries;


/**
 * An SLP Service Entry, which is like a URL Entry but also has an associated
 * scopes set.
 */
class ServiceEntry {
  public:
    ServiceEntry(const ServiceEntry &other)
        : m_url(other.m_url),
          m_service_type(other.m_service_type),
          m_scopes(other.m_scopes) {
    }

    /**
     * @param scopes a set of scopes.
     * @param service_type the service-type
     * @param url the service URL
     * @param lifetime the number of seconds this service is valid for
     */
    ServiceEntry(const ScopeSet &scopes,
                 const string &service_type,
                 const string &url,
                 uint16_t lifetime)
        : m_url(url, lifetime),
          m_service_type(service_type),
          m_scopes(scopes) {
    }

    /**
     * Similar to above but this creates the scope set from a string.
     * @param scopes a set of scopes, comma separated
     * @param service_type the service-type
     * @param url the service URL
     * @param lifetime the number of seconds this service is valid for
     */
    ServiceEntry(const string &scopes,
                 const string &service_type,
                 const string &url,
                 uint16_t lifetime)
        : m_url(url, lifetime),
          m_service_type(service_type),
          m_scopes(scopes) {
    }

    /**
     * This constructors derrives the service type from the url.
     * @param scopes a set of scopes, comma separated
     * @param url the service URL
     * @param lifetime the number of seconds this service is valid for
     */
    ServiceEntry(const ScopeSet &scopes,
                 const string &url,
                 uint16_t lifetime)
        : m_url(url, lifetime),
          m_service_type(SLPServiceFromURL(url)),
          m_scopes(scopes) {
    }

    /**
     * This constructors derrives the service type from the url.
     * @param scopes a set of scopes, comma separated
     * @param url the service URL
     * @param lifetime the number of seconds this service is valid for
     */
    ServiceEntry(const string &scopes,
                 const string &url,
                 uint16_t lifetime)
        : m_url(url, lifetime),
          m_service_type(SLPServiceFromURL(url)),
          m_scopes(scopes) {
    }

    ~ServiceEntry() {}

    const URLEntry& url() const { return m_url; }
    URLEntry& mutable_url() { return m_url; }
    // The service-type, without the 'service:' at the beginning
    string service_type() const { return m_service_type; }
    const ScopeSet& scopes() const { return m_scopes; }

    string ToString() const {
      std::ostringstream str;
      ToStream(str);
      return str.str();
    }

    void ToStream(ostream &out) const {
      out << m_url << ", [" << m_scopes << "]";
    }

    friend ostream& operator<<(ostream &out, const ServiceEntry &service) {
      service.ToStream(out);
      return out;
    }

  private:
    URLEntry m_url;
    string m_service_type;
    ScopeSet m_scopes;
};

// typedef for convenience
typedef std::vector<ServiceEntry> ServiceEntries;


/**
 * A Local Service Entry has everything a ServiceEntry has, but is also tracks
 * which DAs it has been registered with. This is heavier than ServiceEntry so
 * we disallow thie copy and assignment operators.
 */
class LocalServiceEntry {
  public:
    /**
     * Create a new LocalServiceEntry
     * @param service the ServiceEntry that this LocalServiceEntry is
     *   associated with.
     */
    explicit LocalServiceEntry(const ServiceEntry &service)
        : m_service(service) {
    }

    const ServiceEntry& service() const { return m_service; }

    // These control DA registration state
    void UpdateDA(const IPV4Address &address, const TimeStamp &expires_in);
    void RemoveDA(const IPV4Address &address);

    void RegisteredDAs(vector<IPV4Address> *output) const;
    void OldRegistrations(const TimeStamp &limit,
                          vector<IPV4Address> *output) const;

    void SetLifetime(uint16_t lifetime, const TimeStamp &now);
    bool HasExpired(const TimeStamp &now) const { return now > m_expires_at; }

    void ToStream(ostream &out) const;

    friend ostream& operator<<(ostream &out, const LocalServiceEntry &service) {
      service.ToStream(out);
      return out;
    }

  private:
    // Maps IPV4Addresses of DAs to when our registrations expire.
    typedef map<IPV4Address, TimeStamp> DATimeMap;

    ServiceEntry m_service;
    TimeStamp m_expires_at;
    DATimeMap m_registered_das;

    LocalServiceEntry(const LocalServiceEntry&);
    LocalServiceEntry& operator=(const LocalServiceEntry&);
};
}  // slp
}  // ola
#endif  // TOOLS_SLP_SERVICEENTRY_H_
