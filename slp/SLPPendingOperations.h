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
 * SLPPendingOperations.h
 * The hierarchy of Operations classes that are used to keep track of
 * outstanding SLP requests.
 * Copyright (C) 2012 Simon Newton
 */

#ifndef SLP_SLPPENDINGOPERATIONS_H_
#define SLP_SLPPENDINGOPERATIONS_H_

#include <ola/network/IPV4Address.h>
#include <ola/thread/SchedulerInterface.h>
#include <map>
#include <set>
#include <string>
#include <utility>
#include "slp/SLPPacketConstants.h"
#include "slp/ServiceEntry.h"

using ola::network::IPV4Address;
using std::map;
using std::pair;
using std::set;
using std::string;

namespace ola {
namespace slp {

typedef set<IPV4Address> IPV4AddressSet;

/**
 * The base class for outstanding operations. This holds the xid allocated to
 * the operation and the associated timer.
 */
class PendingOperation {
  public:
    PendingOperation(xid_t xid, unsigned int retry_time)
      : xid(xid),
        timer_id(ola::thread::INVALID_TIMEOUT),
        m_retry_time(retry_time),
        m_cumulative_time(0),
        m_attempt_number(1) {
    }

    virtual ~PendingOperation() {}

    unsigned int retry_time() const { return m_retry_time; }
    unsigned int total_time() const { return m_cumulative_time; }

    void UpdateRetryTime() {
      m_attempt_number++;
      m_cumulative_time += m_retry_time;
      m_retry_time += m_retry_time;
    }

    // The number of times we've tried this operation, starts from 1
    uint8_t AttemptNumber() const { return m_attempt_number; }

    xid_t xid;
    ola::thread::timeout_id timer_id;

  private:
    // The time to wait before the next timeout, doubles each time.
    unsigned int m_retry_time;
    // milli-seconds since the first attempt
    unsigned int m_cumulative_time;
    // the number of attempts
    uint8_t m_attempt_number;
};


/**
 * A multicast operation. This keeps track of the previous responders.
 */
class PendingMulticastOperation: public PendingOperation {
  public:
    PendingMulticastOperation(xid_t xid, unsigned int retry_time)
      : PendingOperation(xid, retry_time),
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
 * A unicast SrvReg / SrvDeReg operation.
 */
class UnicastSrvRegOperation: public PendingOperation {
  public:
    UnicastSrvRegOperation(xid_t xid, unsigned int retry_time,
                           const string &da_url,
                           const ServiceEntry &service)
      : PendingOperation(xid, retry_time),
        da_url(da_url),
        service(service) {
    }

    const string da_url;
    ServiceEntry service;
};


/**
 * A unicast SrvRqst operation to a DA.
 */
class UnicastSrvRqstOperation: public PendingOperation {
  public:
    UnicastSrvRqstOperation(xid_t xid, unsigned int retry_time,
                            const string &da_url, const ScopeSet &scopes,
                            class PendingSrvRqst *parent)
      : PendingOperation(xid, retry_time),
        da_url(da_url),
        parent(parent),
        scopes(scopes),
        da_busy(false) {
    }

    const string da_url;
    class PendingSrvRqst *parent;
    ScopeSet scopes;  // the set of scopes this DA is responsible for
    bool da_busy;
};


/**
 * A multicast SrvRqst operation. This is used for all multicast SrvRqsts
 * expect those for directory-agents.
 */
class MulicastSrvRqstOperation: public PendingMulticastOperation {
  public:
    MulicastSrvRqstOperation(xid_t xid, unsigned int retry_time,
                             const ScopeSet &scopes,
                             class PendingSrvRqst *parent)
        : PendingMulticastOperation(xid, retry_time),
          scopes(scopes),
          parent(parent) {
    }

    ScopeSet scopes;  // the set of scopes in this request
    class PendingSrvRqst *parent;
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
                   BaseCallback1<void, const URLEntries&> *callback);

    void MarkScopeAsDone(const string &scope) {
      scope_status_map[scope] = COMPLETE;
    }

    // True if all scopes have completed.
    bool Complete() const;

    string service_type;  // the service we're trying to find
    BaseCallback1<void, const URLEntries&> *callback;
    URLEntries urls;

  private:
    typedef map<string, ScopeStatus> ScopeStatusMap;
    ScopeStatusMap scope_status_map;
};
}  // namespace slp
}  // namespace ola
#endif  // SLP_SLPPENDINGOPERATIONS_H_
