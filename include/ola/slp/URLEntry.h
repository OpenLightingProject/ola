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
 * URLEntry.h
 * The object which holds a url and lifetime.
 * Copyright (C) 2012 Simon Newton
 */

#ifndef INCLUDE_OLA_SLP_URLENTRY_H_
#define INCLUDE_OLA_SLP_URLENTRY_H_

#include <ola/io/BigEndianStream.h>
#include <ostream>
#include <string>
#include <vector>

namespace ola {
namespace slp {

/**
 * Represents a URL with the an associated lifetime. The URL cannot be changed
 * once the object is created. This object is cheap to copy so it can be used
 * in STL containers. It doesn't have an ordering defined though.
 */
class URLEntry {
 public:
    URLEntry() {}

    /**
     * Create a new URLEntry
     * @param url the url string
     * @param lifetime the lifetime in seconds.
     */
    URLEntry(const std::string &url, uint16_t lifetime)
        : m_url(url),
          m_lifetime(lifetime) {
    }

    ~URLEntry() {}

    std::string url() const { return m_url; }
    uint16_t lifetime() const { return m_lifetime; }
    void set_lifetime(uint16_t lifetime) { m_lifetime = lifetime; }

    /*
     * Age this URL by the given number of seconds
     * @returns true if this url has now expired, false otherwise.
     */
    bool AgeLifetime(uint16_t seconds) {
      if (m_lifetime <= seconds) {
        m_lifetime = 0;
        return true;
      }
      m_lifetime-= seconds;
      return false;
    }

    // Return the total size of this URL entry as it appears on the wire
    unsigned int PackedSize() const { return 6 + m_url.size(); }

    // Write this ServiceEntry to an IOQueue
    void Write(ola::io::BigEndianOutputStreamInterface *output) const;

    // equality is based on the url only
    bool operator==(const URLEntry &other) const {
      return m_url == other.m_url;
    }

    bool operator!=(const URLEntry &other) const {
      return m_url != other.m_url;
    }

    URLEntry& operator=(const URLEntry &other) {
      if (this != &other) {
        m_url = other.m_url;
        m_lifetime = other.m_lifetime;
      }
      return *this;
    }

    void ToStream(std::ostream *out) const {
      *out << m_url << "(" << m_lifetime << ")";
    }

    std::string ToString() const {
      std::ostringstream str;
      ToStream(&str);
      return str.str();
    }

    friend std::ostream& operator<<(std::ostream &out, const URLEntry &entry) {
      entry.ToStream(&out);
      return out;
    }

 protected:
    std::string m_url;
    uint16_t m_lifetime;
    // TODO(simon): add auth blocks here
};

// typedef for convenience
typedef std::vector<URLEntry> URLEntries;
}  // namespace slp
}  // namespace ola
#endif  // INCLUDE_OLA_SLP_URLENTRY_H_
