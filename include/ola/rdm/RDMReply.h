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
 * RDMReply.h
 * The RDMReply object.
 * Copyright (C) 2015 Simon Newton
 */

#ifndef INCLUDE_OLA_RDM_RDMREPLY_H_
#define INCLUDE_OLA_RDM_RDMREPLY_H_

#include <ola/base/Macro.h>
#include <ola/rdm/RDMCommand.h>
#include <ola/rdm/RDMFrame.h>
#include <ola/rdm/RDMResponseCodes.h>

#include <string>
#include <memory>

namespace ola {
namespace rdm {

/**
 * @brief Holds the final state of an RDM request.
 *
 * When a RDM request completes, the following information is returned:
 *  - The RDMStatusCode.
 *  - An RDMResponse, if the response data was a valid RDM response.
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
  RDMReply(RDMStatusCode status_code, RDMResponse *response);

  /**
   * @brief Create a RDMReply with a response code and response object.
   * @param status_code The RDMStatusCode.
   * @param response The RDMResponse object, ownership is transferred.
   * @param frames The raw RDM frames that make up the response.
   */
  RDMReply(RDMStatusCode status_code,
           RDMResponse *response,
           const RDMFrames &frames);

  /**
   * @brief Return the RDMStatusCode for the request.
   */
  RDMStatusCode StatusCode() const { return m_status_code; }

  /**
   * @brief Returns the RDMResponse if there is one.
   * @returns A RDMResponse object or NULL if the response data was not a valid
   *   RDM message.
   *
   * The returned pointer is valid for the lifetime of the RDMReply object.
   */
  const RDMResponse* Response() const { return m_response.get(); }

  /**
   * @brief Returns a pointer to a mutable RDMResponse.
   * @returns A RDMResponse object or NULL if the response data was not a valid
   *   RDM message.
   *
   * The returned pointer is valid for the lifetime of the RDMReply object.
   */
  RDMResponse* MutableResponse() { return m_response.get(); }

  /**
   * @brief The frames that make up this RDM reply.
   *
   * This may be empty, if the raw frame data was not available.
   */
  const RDMFrames& Frames() const { return m_frames; }

  /**
   * @brief Test for equality.
   * @param other The RDMReply to test against.
   * @returns True if two RDMReplies are equal.
   */
  bool operator==(const RDMReply &other) const;

  /**
   * @name String Methods
   * @{
   */

  /**
   * @brief Create a human readable string from the RDMReply object.
   * @returns A string representation of the RDMReply.
   */
  std::string ToString() const;

  /**
   * @brief Output an RDMReply object to an ostream.
   * @param out ostream to output to
   * @param reply is the RDMReply to print
   * @sa ToString()
   */
  friend std::ostream& operator<<(std::ostream &out,
                                  const RDMReply &reply) {
    return out << reply.ToString();
  }

  /** @} */

  /**
   * @brief A helper method to create a RDMReply from raw frame data.
   * @param frame A RDMFrame.
   * @param request An optional RDMRequest that triggered this response.
   * @returns A new RDMReply object.
   */
  static RDMReply* FromFrame(const RDMFrame &frame,
                             const RDMRequest *request = NULL);

  /**
   * @brief A helper method to create a RDMReply for a DUB response.
   * @param frame A RDMFrame of DUB data.
   * @returns A new RDMReply object, with a StatusCode() of RDM_DUB_RESPONSE
   *   and a NULL RDMResponse.
   */
  static RDMReply* DUBReply(const RDMFrame &frame);

 private:
  RDMStatusCode m_status_code;
  std::unique_ptr<RDMResponse> m_response;
  RDMFrames m_frames;

  DISALLOW_COPY_AND_ASSIGN(RDMReply);
};
}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_RDMREPLY_H_
