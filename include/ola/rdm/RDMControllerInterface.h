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
 * RDMControllerInterface.h
 * A RDM Controller that sends a single message at a time.
 * Copyright (C) 2010 Simon Newton
 */

/**
 * @addtogroup rdm_controller
 * @{
 * @file RDMControllerInterface.h
 * @brief Definitions and Interfaces to implement an RDMController that sends a
 * single message at a time.
 * @}
 */
#ifndef INCLUDE_OLA_RDM_RDMCONTROLLERINTERFACE_H_
#define INCLUDE_OLA_RDM_RDMCONTROLLERINTERFACE_H_

#include <ola/Callback.h>
#include <ola/rdm/RDMCommand.h>
#include <ola/rdm/RDMReply.h>
#include <ola/rdm/RDMResponseCodes.h>
#include <ola/rdm/UIDSet.h>

namespace ola {
namespace rdm {

/**
 * @brief The callback run when a RDM request completes.
 * @tparam reply The RDMReply object. The reply object is valid for the
 * duration of the call.
 *
 * The RDMReply is not const, since some stages of the pipeline may need to
 * rewrite the UID / Transaction Number.
 *
 * For performance reasons this can be either a single use callback or a
 * permanent callback.
 */
typedef ola::BaseCallback1<void, RDMReply*> RDMCallback;

/**
 * @brief A helper message to run a RDMCallback with the given status code.
 * @param callback The RDMCallback to run.
 * @param status_code The status code to use in the RDMReply.
 */
inline void RunRDMCallback(RDMCallback *callback, RDMStatusCode status_code) {
  ola::rdm::RDMReply reply(status_code);
  callback->Run(&reply);
}

/**
 * @brief The callback run when a discovery operation completes.
 * @tparam UIDSet The UIDs that were discovered.
 */
typedef ola::BaseCallback1<void, const ola::rdm::UIDSet&> RDMDiscoveryCallback;

/**
 * @brief The interface that can send RDMRequest.
 */
class RDMControllerInterface {
 public:
  virtual ~RDMControllerInterface() {}

  /**
   * @brief Send a RDM command.
   * @param request the RDMRequest, ownership is transferred.
   * @param on_complete The callback to run when the request completes.
   *
   * Implementers much ensure that the callback is always run at some point.
   * In other words, there must be no way that a request can be dropped in such
   * a way that the callback is never run. Doing so will either block all
   * subsequent requests, or leak memory depending on the implementation.
   *
   * Also the implementer of this class may want to  re-write the transaction #,
   * and possibly the UID (changing src UIDs isn't addressed by the RDM
   * spec).
   *
   * The RDMRequest may be a DISCOVERY_COMMAND, if the implementation does not
   * support DISCOVERY_COMMANDs then the callback should be run with
   * ola::rdm::RDM_PLUGIN_DISCOVERY_NOT_SUPPORTED.
   */
  virtual void SendRDMRequest(RDMRequest *request,
                              RDMCallback *on_complete) = 0;
};


/**
 * @brief The interface that can send RDM commands, as well as perform discovery
 * operations.
 */
class DiscoverableRDMControllerInterface: public RDMControllerInterface {
 public:
  DiscoverableRDMControllerInterface(): RDMControllerInterface() {}

  virtual ~DiscoverableRDMControllerInterface() {}

  /**
   * @brief Start a full discovery operation.
   * @param callback The callback run when discovery completes. This may run
   *   immediately in some implementations.
   */
  virtual void RunFullDiscovery(RDMDiscoveryCallback *callback) = 0;

  /**
   * @brief Start an incremental discovery operation.
   * @param callback The callback run when discovery completes. This may run
   *   immediately in some implementations.
   */
  virtual void RunIncrementalDiscovery(RDMDiscoveryCallback *callback) = 0;
};
}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_RDMCONTROLLERINTERFACE_H_
