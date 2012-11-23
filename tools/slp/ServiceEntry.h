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
#include <ola/io/BigEndianStream.h>
#include <ostream>
#include <set>
#include <string>
#include "tools/slp/SLPStrings.h"

namespace ola {
namespace slp {

using std::ostream;
using std::set;
using std::string;

/**
 * Represents a URL with the associated lifetime. Two URLEntries are equal if
 * the URL is the same.
 */
class URLEntry {
  public:
    URLEntry() {}

    URLEntry(const string &url, uint16_t lifetime)
        : m_url(url),
          m_lifetime(lifetime) {
    }

    URLEntry(const URLEntry &other)
      : m_url(other.m_url),
        m_lifetime(other.m_lifetime) {
    }

    virtual ~URLEntry() {}

    string URL() const { return m_url; }
    uint16_t Lifetime() const { return m_lifetime; }

    // Return the total size of this URL entry
    unsigned int Size() const { return 6 + m_url.size(); }

    // Write this ServiceEntry to an IOQueue
    void Write(ola::io::BigEndianOutputStreamInterface *output) const;

    URLEntry& operator=(const URLEntry &other) {
      if (this != &other) {
        m_url = other.m_url;
        m_lifetime = other.m_lifetime;
      }
      return *this;
    }

    bool operator<(const URLEntry &other) const {
      return m_url < other.m_url;
    }

    bool operator==(const URLEntry &other) const {
      return m_url == other.m_url;
    }

    bool operator!=(const URLEntry &other) const {
      return m_url != other.m_url;
    }

    virtual void ToStream(ostream &out) const {
      out << m_url << "(" << m_lifetime << ")";
    }

    friend ostream& operator<<(ostream &out, const URLEntry &entry) {
      entry.ToStream(out);
      return out;
    }

  protected:
    string m_url;
    mutable uint16_t m_lifetime;
    // TODO(simon): add auth blocks here
};


/**
 * An SLP Service Entry, which is like a URL Entry but also has associated
 * scopes.
 */
class ServiceEntry: public URLEntry {
  public:
    ServiceEntry() : URLEntry() {}

    /**
     * @param scopes a set of scopes, should be in canonical form
     * @param url the service URL
     * @param lifetime the number of seconds this service is valid for
     */
    ServiceEntry(const set<string> &scopes,
                 const string &url,
                 uint16_t lifetime)
        : URLEntry(url, lifetime),
          m_scopes(scopes) {
    }
    ~ServiceEntry() {}

    void SetLifetime(uint16_t lifetime) const { m_lifetime = lifetime; }
    const set<string>& Scopes() const { return m_scopes; }

    // Return true if the scopes for this service exactly match the given scope
    // set
    bool MatchesScopes(const set<string> &scopes) const {
      return scopes == m_scopes;
    }
    // Return true if the service's scopes match any of the provided scopes
    bool IntersectsScopes(const set<string> &scopes) const {
      return SLPSetIntersect(m_scopes, scopes);
    }

    virtual void ToStream(ostream &out) const {
      URLEntry::ToStream(out);
      out << ", [" << ola::StringJoin(",", m_scopes) << "]";
    }

  private:
    // TODO(simon): maybe optimize this as a bit vector since it's static?
    set<string> m_scopes;
    // TODO(simon): add attributes here
};

// typedef for convenience
typedef std::set<URLEntry> URLEntries;
typedef std::set<ServiceEntry> ServiceEntries;
}  // slp
}  // ola
#endif  // TOOLS_SLP_SERVICEENTRY_H_
