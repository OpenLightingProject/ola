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
 * URLEntry.h
 * Copyright (C) 2012 Simon Newton
 */

#ifndef TOOLS_SLP_URLENTRY_H_
#define TOOLS_SLP_URLENTRY_H_

#include <ola/io/IOQueue.h>

#include <string>
#include <vector>

using ola::io::IOQueue;
using std::string;

namespace ola {
namespace slp {

/**
 * Represents an SLP URLEntry.
 */
class URLEntry {
  public:
    URLEntry(uint16_t lifetime, const string &url)
        : m_lifetime(lifetime),
          m_url(url) {
    }
    ~URLEntry() {}

    // Return the total size of this URL entry
    unsigned int Size() const { return 6 + m_url.size(); }

    // Write this URLEntry to an IOQueue
    void Write(IOQueue *ioqueue) const;

  private:
    uint16_t m_lifetime;
    string m_url;
};

// typedef for convenience
typedef std::vector<URLEntry> URLEntries;
}  // slp
}  // ola
#endif  // TOOLS_SLP_URLENTRY_H_
