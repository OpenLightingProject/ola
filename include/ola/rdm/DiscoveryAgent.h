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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * DiscoveryAgent.h
 * Implements the RDM Discovery algorithm.
 * Copyright (C) 2011 Simon Newton
 */

/**
 * @addtogroup rdm_controller
 * @{
 * @file include/ola/rdm/DiscoveryAgent.h
 * @brief Implements the RDM Discovery algorithm
 * @}
 */

#ifndef INCLUDE_OLA_RDM_DISCOVERYAGENT_H_
#define INCLUDE_OLA_RDM_DISCOVERYAGENT_H_

#include <ola/Callback.h>
#include <ola/rdm/UID.h>
#include <ola/rdm/UIDSet.h>
#include <memory>
#include <queue>
#include <stack>
#include <utility>

namespace ola {
namespace rdm {

/**
 * @brief The interface used by the DiscoveryTarget to send RDM commands.
 *
 * This class abstracts away the method of sending RDM commands from the
 * discovery algorithm in DiscoveryAgent.
 *
 * For each of the MuteDevice, UnMuteAll and Branch methods, the implementation
 * should send the appropriate RDM command and then run the provided callback
 * when the command has been sent.
 */
class DiscoveryTargetInterface {
 public:
  /**
   * @brief The callback run when a mute command completes.
   * @tparam ok true if the device muted correctly, false if the device failed
   *   to ack the mute.
   */
  typedef ola::BaseCallback1<void, bool> MuteDeviceCallback;

  /**
   * @brief The callback run when an unmute command completes.
   */
  typedef ola::BaseCallback0<void> UnMuteDeviceCallback;

  /**
   * @brief The callback run when a DUB command completes.
   * @tparam data The DUB response, if any was received. Otherwise pass NULL.
   * @tparam length The length of the DUB response.
   */
  typedef ola::BaseCallback2<void, const uint8_t*, unsigned int>
    BranchCallback;

  virtual ~DiscoveryTargetInterface() {}

  /**
   * @brief Mute a device.
   * @param target the device to mute
   * @param mute_complete the callback to run when the mute operations
   * completes.
   */
  virtual void MuteDevice(const UID &target,
                          MuteDeviceCallback *mute_complete) = 0;

  /**
   * @brief Unmute all devices.
   * @param unmute_complete the callback to run when the unmute operation
   * completes.
   */
  virtual void UnMuteAll(UnMuteDeviceCallback *unmute_complete) = 0;

  /**
   * @brief Send a DUB command
   * @param lower the lower bound UID.
   * @param upper the upper bound UID.
   * @param callback the callback to run when the DUB completes.
   *
   * Any data received in response to the DUB command should be passed back
   * when the callback is run.
   */
  virtual void Branch(const UID &lower,
                      const UID &upper,
                      BranchCallback *callback) = 0;
};


/**
 * @brief An asynchronous RDM Discovery algorithm.
 *
 * This implements the binary search algorithm from the E1.20 standard. The
 * implementation is asynchronous and relies on callbacks to indicate when each
 * step completes.
 *
 * To use the DiscoveryAgent, one should write a class that implements the
 * DiscoveryTargetInterface interface and then pass a pointer to that object to
 * the DiscoveryAgent.
 *
 * The discovery process goes something like this:
 *   - if incremental, copy all previously discovered UIDs to the mute list
 *   - push (0, 0xffffffffffff) onto the resolution stack
 *   - unmute all
 *   - mute all previously discovered UIDs, for any that fail to mute remove
 *     them from the UIDSet.
 *   - Send a discovery unique branch message
 *     - If we get a valid response, mute, and send the same branch again
 *     - If we get a collision, split the UID range, and try each branch
 *       separately.
 *
 * We also track responders that fail to ack a mute request (we attempt to mute
 * MAX_MUTE_ATTEMPTS times) and branches that contain responders which continue
 * to respond once muted. The latter causes a branch to be marked as corrupt,
 * which prevents us from looping forver.
 */
class DiscoveryAgent {
 public:
  /**
   * @brief Create a new DiscoveryAgent
   * @param target the DiscoveryTargetInterface to use for sending commands.
   */
  explicit DiscoveryAgent(DiscoveryTargetInterface *target);

  /**
   * @brief Destructor.
   */
  ~DiscoveryAgent();

  typedef ola::SingleUseCallback2<void, bool, const UIDSet&>
    DiscoveryCompleteCallback;

  /**
   * @brief Cancel any in-progress discovery operation.
   * If a discovery operation is running, this will result in the callback
   * being run.
   */
  void Abort();

  /**
   * @brief Initiate a full discovery operation.
   * @param on_complete the callback to run once discovery completes.
   */
  void StartFullDiscovery(DiscoveryCompleteCallback *on_complete);

  /**
   * @brief Initiate an incremental discovery operation.
   * @param on_complete the callback to run once discovery completes.
   */
  void StartIncrementalDiscovery(DiscoveryCompleteCallback *on_complete);

 private:
  /**
   * @brief Represents a range of UIDs (a branch of the UID tree)
   */
  struct UIDRange {
    UIDRange(const UID &_lower, const UID &_upper, UIDRange *_parent)
        : lower(_lower),
          upper(_upper),
          parent(_parent),
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
  // uids that are misbehaved in some way which we've already split around
  UIDSet m_split_uids;
  DiscoveryCompleteCallback *m_on_complete;
  // uids to mute during incremental discovery
  std::queue<UID> m_uids_to_mute;
  // Callbacks used by the DiscoveryTarget
  std::auto_ptr<DiscoveryTargetInterface::UnMuteDeviceCallback>
      m_unmute_callback;
  std::auto_ptr<DiscoveryTargetInterface::MuteDeviceCallback>
      m_incremental_mute_callback;
  std::auto_ptr<DiscoveryTargetInterface::MuteDeviceCallback>
      m_branch_mute_callback;
  std::auto_ptr<DiscoveryTargetInterface::BranchCallback> m_branch_callback;

  // The stack of UIDRanges
  UIDRanges m_uid_ranges;
  UID m_muting_uid;  // the uid we're currently trying to mute
  unsigned int m_unmute_count;
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
  void SplitAroundBadUID(UID bad_uid);
  void FreeCurrentRange();

  static const unsigned int PREAMBLE_SIZE = 8;
  static const unsigned int EUID_SIZE = 12;
  static const unsigned int CHECKSUM_SIZE = 4;

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
  // The number of times we'll send a broadcast unmute command
  // This should be more than 1 to ensure that all devices are unmuted.
  static const unsigned int BROADCAST_UNMUTE_REPEATS = 3;

  static const uint8_t PREAMBLE = 0xfe;
  static const uint8_t PREAMBLE_SEPARATOR = 0xaa;
};
}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_DISCOVERYAGENT_H_
