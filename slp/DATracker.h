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
 * DATracker.h
 * Copyright (C) 2012 Simon Newton
 */

#ifndef SLP_DATRACKER_H_
#define SLP_DATRACKER_H_

#include <ola/Callback.h>
#include <ola/network/IPV4Address.h>
#include <ola/network/SocketAddress.h>
#include <map>
#include <string>
#include <set>
#include <vector>
#include "slp/SLPPacketParser.h"
#include "slp/ScopeSet.h"

namespace ola {
namespace slp {

using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;
using std::map;
using std::set;
using std::string;
using std::vector;

/**
 * Represents a DA
 */
class DirectoryAgent {
  public:
    DirectoryAgent()
        : m_boot_time(0) {
    }

    DirectoryAgent(const ScopeSet &scopes,
                   const string &url,
                   const IPV4Address &address,
                   uint32_t boot_timestamp)
        : m_boot_time(boot_timestamp),
          m_scopes(scopes),
          m_url(url),
          m_address(address),
          m_min_refresh_internal(0) {
    }

    virtual ~DirectoryAgent() {}

    ScopeSet scopes() const { return m_scopes; }
    void set_scopes(const ScopeSet &scopes) { m_scopes = scopes; }

    string URL() const { return m_url; }
    IPV4Address IPAddress() const { return m_address; }
    uint32_t BootTimestamp() const { return m_boot_time; }
    uint32_t MinRefreshInteral() const { return m_min_refresh_internal; }

    DirectoryAgent& operator=(const DirectoryAgent &other) {
      if (this != &other) {
        m_scopes = other.m_scopes;
        m_url = other.m_url;
        m_address = other.m_address;
        m_boot_time = other.m_boot_time;
        m_min_refresh_internal = other.m_min_refresh_internal;
      }
      return *this;
    }

    // Sort by URL
    bool operator<(const DirectoryAgent &other) const {
      return m_url < other.m_url;
    }

    // Note this doesn't compare scopes
    bool operator==(const DirectoryAgent &other) const {
      return m_url == other.m_url;
    }

    bool operator!=(const DirectoryAgent &other) const {
      return m_url != other.m_url;
    }

    virtual void ToStream(ostream *out) const {
      *out << m_url << "(" << m_boot_time << ")";
      *out << ", [" << m_scopes << "]";
    }

    friend ostream& operator<<(ostream &out, const DirectoryAgent &entry) {
      entry.ToStream(&out);
      return out;
    }

  protected:
    mutable uint32_t m_boot_time;

  private:
    ScopeSet m_scopes;
    string m_url;
    IPV4Address m_address;
    uint32_t m_min_refresh_internal;
};


/**
 * Holds information about the DAs on the network. Clients can register
 * callbacks to be informed of DA events.
 */
class DATracker {
  public:
    typedef ola::Callback1<void, const DirectoryAgent&> NewDACallback;

    DATracker() {}
    ~DATracker();

    void AddNewDACallback(NewDACallback *callback);
    void RemoveNewDACallback(NewDACallback *callback);

    void NewDAAdvert(const DAAdvertPacket &da_advert,
                     const IPV4SocketAddress &source);

    unsigned int DACount() const { return m_agents.size(); }

    void GetDirectoryAgents(vector<DirectoryAgent> *output);
    void GetDAsForScopes(const ScopeSet &scopes,
                         vector<DirectoryAgent> *output);
    void GetMinimalCoveringList(const ScopeSet &scopes,
                                vector<DirectoryAgent> *output);

    bool LookupDA(const string &da_url, DirectoryAgent *da);
    void MarkAsBad(const string &da_url);

  private:
    class InternalDirectoryAgent : public DirectoryAgent {
      public:
        InternalDirectoryAgent(): DirectoryAgent() {}

        InternalDirectoryAgent(const ScopeSet &scopes,
                               const string &url,
                               const IPV4Address &address,
                               uint32_t boot_timestamp)
            : DirectoryAgent(scopes, url, address, boot_timestamp) {
        }

        void SetBootTimestamp(uint32_t boot_timestamp) const {
          m_boot_time = boot_timestamp;
        }
    };

    typedef map<string, InternalDirectoryAgent> DAMap;  // keyed by url
    typedef set<NewDACallback*> DACallbacks;

    DAMap m_agents;
    DACallbacks m_new_da_callbacks;

    void RunCallbacks(const DirectoryAgent &agent);
    bool AddressFromURL(const string &url, IPV4Address *address);

    static const string DA_SERVICE_PREFIX;
};
}  // namespace slp
}  // namespace ola
#endif  // SLP_DATRACKER_H_
