/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * DiscoveryAgent.h
 * Implements the RDM Discovery algorithm.
 * Copyright (C) 2011 Simon Newton
 */

#ifndef INCLUDE_OLA_RDM_DISCOVERYAGENT_H_
#define INCLUDE_OLA_RDM_DISCOVERYAGENT_H_

#include <ola/Callback.h>
#include <ola/rdm/UID.h>
#include <ola/rdm/UIDSet.h>
#include <queue>
#include <stack>
#include <utility>


namespace ola {
namespace rdm {


using std::queue;

/**
 * This is the interface for classes which want to act as a discovery
 * controller. Such clases must implement the Mute, UnMute & Discovery Unique
 * Branch (DUB) methods.
 */
class DiscoveryTargetInterface {
  public:
    // Argument is true if the device responded
    typedef ola::BaseCallback1<void, bool> MuteDeviceCallback;
    typedef ola::BaseCallback0<void> UnMuteDeviceCallback;
    // Arguments are a pointer to the data & length of the DUB response,
    // excluding the RDM start code.
    typedef ola::BaseCallback2<void, const uint8_t*, unsigned int>
      BranchCallback;

    virtual ~DiscoveryTargetInterface() {}

    // Mute a device
    virtual void MuteDevice(const UID &target,
                            MuteDeviceCallback *mute_complete) = 0;
    // Un Mute all devices
    virtual void UnMuteAll(UnMuteDeviceCallback *unmute_complete) = 0;

    // Send a branch request
    virtual void Branch(const UID &lower,
                        const UID &upper,
                        BranchCallback *callback) = 0;
};


/**
 * This class controls the discovery algorithm.
 */
class DiscoveryAgent {
  public:
    explicit DiscoveryAgent(DiscoveryTargetInterface *target);
    ~DiscoveryAgent();

    typedef ola::SingleUseCallback2<void, bool, const UIDSet&>
      DiscoveryCompleteCallback;

    void Abort();
    void StartFullDiscovery(DiscoveryCompleteCallback *on_complete);
    void StartIncrementalDiscovery(DiscoveryCompleteCallback *on_complete);

  private:
    /**
     * Represents a range of UIDs (a branch of the UID tree)
     */
    struct UIDRange {
      UIDRange(const UID &lower, const UID &upper, UIDRange *parent)
          : lower(lower),
            upper(upper),
            parent(parent),
            attempt(0),
            failures(0),
            uids_discovered(0),
            branch_corrupt(false) {
      }
      UID lower;
      UID upper;
      UIDRange *parent;  // the parent Range
      unsigned int attempt;  // the # of attempts for this branch
      unsigned int failures;
      unsigned int uids_discovered;
      bool branch_corrupt;  // true if this branch contains a bad device
    };

    typedef std::stack<UIDRange*> UIDRanges;

    DiscoveryTargetInterface *m_target;
    UIDSet m_uids;
    // uids that are misbehaved in some way
    UIDSet m_bad_uids;
    DiscoveryCompleteCallback *m_on_complete;
    // uids to mute during incremental discovery
    std::queue<UID> m_uids_to_mute;
    // Callbacks used by the DiscoveryTarget
    DiscoveryTargetInterface::UnMuteDeviceCallback *m_unmute_callback;
    DiscoveryTargetInterface::MuteDeviceCallback *m_incremental_mute_callback;
    DiscoveryTargetInterface::MuteDeviceCallback *m_branch_mute_callback;
    DiscoveryTargetInterface::BranchCallback *m_branch_callback;
    // The stack of UIDRanges
    UIDRanges m_uid_ranges;
    UID m_muting_uid;  // the uid we're currently trying to mute
    unsigned int m_mute_attempts;
    bool m_tree_corrupt;  // true if there was a problem with discovery

    void InitDiscovery(DiscoveryCompleteCallback *on_complete,
                       bool incremental);

    void UnMuteComplete();
    void MaybeMuteNextDevice();
    void IncrementalMuteComplete(bool status);
    void SendDiscovery();

    void BranchComplete(const uint8_t *data, unsigned int length);
    void BranchMuteComplete(bool status);
    void HandleCollision();
    void FreeCurrentRange();

    static const unsigned int MAX_DUB_RESPONSE_SIZE = 24;
    static const unsigned int MIN_DUB_RESPONSE_SIZE = 17;
    /*
     * The maximum numbers of times we'll retry discovery if we get a
     * collision, but after splitting the range in two no nodes can be found.
     */
    static const unsigned int MAX_EMPTY_BRANCH_ATTEMPTS = 5;
    /*
     * The maximum number of times we'll perform discovery on a branch when we
     * get an inconsistent result (responder not muting, etc.)
     */
    static const unsigned int MAX_BRANCH_FAILURES = 5;
    // The number of times we'll attempt to mute a UID
    static const unsigned int MAX_MUTE_ATTEMPTS = 5;
};
}  // rdm
}  // ola
#endif  // INCLUDE_OLA_RDM_DISCOVERYAGENT_H_
