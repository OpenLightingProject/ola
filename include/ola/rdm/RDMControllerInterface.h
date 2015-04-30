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
#include <ola/rdm/RDMResponseCodes.h>
#include <ola/rdm/UIDSet.h>
#include <string>
#include <vector>

namespace ola {
namespace rdm {

/**
 * @brief The raw data for a RDM message and its timing information.
 *
 * A RDMFrame holds the raw data and timing metadata for a RDM message. If no
 * timing data was provided, all timing values will be 0.
 */
class RDMFrame {
 public:
  typedef std::basic_string<uint8_t> ByteString;

  ByteString data;  //!< The raw RDM response data.

  /**
   * @brief The timing measurements for an RDM Frame.
   *
   * All times are measured in nanoseconds.
   *
   * For DUB responses, the break and mark values will be 0.
   */
  struct {
    uint32_t response_delay;  //!< The delay before the first response.
    uint32_t break_time;  //!< The duration of the break.
    uint32_t mark_time;  //!< The duration of the mark-after-break.
    uint32_t data_time;  //!< The duration of the data.
  } timing_info;
};

/**
 * @brief Holds the final state of an RDM request.
 *
 * When a RDM request completes, the following information is returned:
 *  - The RDMStatusCode.
 *  - An RDMResponse, if the response data was valid.
 *  - Raw data, including response timing information if provided.
 */
struct RDMReply {
 public:
  /**
   * @brief Create a new RDMReply from a RDM Response Code.
   */
  explicit RDMReply(RDMStatusCode status_code);

  /**
   * @brief Create a RDMReply with a response code and response object.
   * @param status_code The RDMStatusCode.
   * @param response The RDMResponse object, ownership is transferred.
   */
  RDMReply(RDMStatusCode status_code,
           RDMResponse *response);

  /**
   * @brief Return the RDMStatusCode for the request.
   */
  RDMStatusCode StatusCode() const;

  /**
   * @brief Returns the RDMResponse if there is one.
   * @returns A RDMResponse object or NULL if the response data was not a valid
   *   RDM message.
   *
   * The returned pointer is valid for the lifetime of the RDMReply object.
   */
  const RDMResponse* Response() const;

  /**
   * @brief Returns a pointer to a mutable RDMResponse.
   * @returns A RDMResponse object or NULL if the response data was not a valid
   *   RDM message.
   *
   * The returned pointer is valid for the lifetime of the RDMReply object.
   */
  RDMResponse* MutableResponse();

  /**
   * @brief The frames that make up this RDM reply.
   *
   * This may be empty, if the raw frame data was not available.
   */
  std::vector<RDMFrame> frames;

 private:
  RDMStatusCode m_status_code;
  std::auto_ptr<RDMResponse> m_rdm_response;

  DISALLOW_COPY_AND_ASSIGN(RDMReply);
};

/**
 * @brief The callback run when a RDM request completes.
 * @tparam reply The RDMReply object.
 *
 * For performance reasons this can be either a single use callback or a
 * permanent callback.
 */
// typedef ola::BaseCallback1<void, const RDMReply&> RDMCallback;

typedef ola::BaseCallback3<void,
                           RDMStatusCode,
                           const RDMResponse*,
                           const std::vector<std::string>&> RDMCallback;

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
   * Implementors much ensure that the callback is always run at some point.
   * In other words, there must be no way that a request can be dropped in such
   * a way that the callback is never run. Doing so will either block all
   * subsequent requests, or leak memory depending on the implementation.
   *
   * Also the implementor of this class may want to  re-write the transaction #,
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
