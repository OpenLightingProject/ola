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
 * SLPPendingOperations.h
 * Contains classes that store state for network requests.
 * Copyright (C) 2012 Simon Newton
 */

#ifndef TOOLS_SLP_SLPPENDINGOPERATIONS_H_
#define TOOLS_SLP_SLPPENDINGOPERATIONS_H_

#include <ola/network/IPV4Address.h>
#include <ola/thread/SchedulerInterface.h>
#include <map>
#include <set>
#include <string>
#include "tools/slp/SLPPacketConstants.h"
#include "tools/slp/ServiceEntry.h"

using ola::network::IPV4Address;
using std::map;
using std::pair;
using std::set;
using std::string;

namespace ola {
namespace slp {

typedef set<IPV4Address> IPV4AddressSet;

class PendingOperation {
  public:
    PendingOperation(xid_t xid, unsigned int retry_time)
      : xid(xid),
        timer_id(ola::thread::INVALID_TIMEOUT),
        retry_time(retry_time) {
    }

    virtual ~PendingOperation() {}

    xid_t xid;
    ola::thread::timeout_id timer_id;
    // seconds since the first attempt, doubles up to m_config_retry_max
    unsigned int retry_time;
};


/**
 * An operation with a DA.
 */
class PendingDAOperation: public PendingOperation {
  public:
    PendingDAOperation(xid_t xid, unsigned int retry_time,
                       const string &da_url)
      : PendingOperation(xid, retry_time),
        da_url(da_url) {
    }

    const string da_url;
};


/**
 * This represents a pending registation / deregistation operation.
 */
class PendingRegistationOperation: public PendingDAOperation {
  public:
    PendingRegistationOperation(xid_t xid, unsigned int retry_time,
                                const string &da_url,
                                const ServiceEntry &service)
      : PendingDAOperation(xid, retry_time, da_url),
        service(service) {
    }

    ServiceEntry service;
};


/**
 * This represents a pending DA find operation
 */
class PendingDAFindOperation: public PendingDAOperation {
  public:
    PendingDAFindOperation(xid_t xid, unsigned int retry_time,
                           const string &da_url, const ScopeSet &scopes,
                           class PendingSrvRqst *parent)
      : PendingDAOperation(xid, retry_time, da_url),
        parent(parent),
        scopes(scopes),
        da_busy(false) {
    }

    class PendingSrvRqst *parent;
    ScopeSet scopes;  // the set of scopes this DA is responsible for
    bool da_busy;
};


/**
 * This represents a pending multicast operation.
 */
class PendingMulticastFindOperation: public PendingOperation {
  public:
    PendingMulticastFindOperation(xid_t xid, unsigned int retry_time,
                                  const ScopeSet &scopes,
                                  class PendingSrvRqst *parent)
        : PendingOperation(xid, retry_time),
          scopes(scopes),
          parent(parent),
          m_pr_list_changed(false) {
    }

    // Insert an element, and update pr_list_changed if required
    void AddPR(const IPV4Address &address) {
      pair<IPV4AddressSet::iterator, bool> p = pr_list.insert(address);
      m_pr_list_changed = p.second;
    }

    bool PRListChanged() const { return m_pr_list_changed; }
    void ResetPRListChanged() { m_pr_list_changed = false; }
    unsigned int PRListSize() const { return pr_list.size(); }

    ScopeSet scopes;  // the set of scopes in this request
    class PendingSrvRqst *parent;
    IPV4AddressSet pr_list;

  private:
    bool m_pr_list_changed;
};


/**
 * Represents a find operation.
 */
class PendingSrvRqst {
  public:
    typedef enum {
      PENDING,
      COMPLETE,
    } ScopeStatus;

    PendingSrvRqst(const string &service_type,
                   const ScopeSet &scopes,
                   SingleUseCallback1<void, const URLEntries&> *callback);

    void MarkScopeAsDone(const string &scope) {
      scope_status_map[scope] = COMPLETE;
    }

    // True if all scopes have completed.
    bool Complete() const;

    string service_type;  // the service we're trying to find
    SingleUseCallback1<void, const URLEntries&> *callback;
    URLEntries urls;

  private:
    typedef map<string, ScopeStatus> ScopeStatusMap;
    ScopeStatusMap scope_status_map;
};
}  // slp
}  // ola
#endif  // TOOLS_SLP_SLPPENDINGOPERATIONS_H_
