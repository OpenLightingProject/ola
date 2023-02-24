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
 * RDMReply.cpp
 * The RDMReply object.
 * Copyright (C) 2015 Simon Newton
 */

#include "ola/rdm/RDMReply.h"

#include <ola/rdm/RDMHelper.h>
#include <ola/rdm/RDMResponseCodes.h>
#include <string>
#include <vector>

using ola::rdm::RDMResponse;

namespace {
bool ResponsesAreEqual(const RDMResponse *response1,
                       const RDMResponse *response2) {
  if (response1 == NULL && response2 == NULL) {
    return true;
  }

  if (response1 == NULL || response2 == NULL) {
    return false;
  }
  return *response1 == *response2;
}
}  // namespace

namespace ola {
namespace rdm {

RDMReply::RDMReply(RDMStatusCode status_code)
    : m_status_code(status_code) {
}

RDMReply::RDMReply(RDMStatusCode status_code, RDMResponse *response)
    : m_status_code(status_code),
      m_response(response) {
}

RDMReply::RDMReply(RDMStatusCode status_code,
                   RDMResponse *response,
                   const RDMFrames &frames)
    : m_status_code(status_code),
      m_response(response),
      m_frames(frames) {
}

bool RDMReply::operator==(const RDMReply &other) const {
  return (m_status_code == other.m_status_code &&
          ResponsesAreEqual(m_response.get(), other.m_response.get()) &&
          m_frames == other.m_frames);
}

std::string RDMReply::ToString() const {
  std::ostringstream str;
  str << StatusCodeToString(m_status_code);
  if (m_response.get()) {
    str << ": " << *m_response.get();
  }
  return str.str();
}

RDMReply* RDMReply::FromFrame(const RDMFrame &frame,
                              const RDMRequest *request) {
  RDMFrames frames;
  frames.push_back(frame);

  ola::rdm::RDMStatusCode status_code = RDM_INVALID_RESPONSE;
  RDMResponse *response = NULL;
  if (frame.data.size() > 1) {
    // Skip over the start code.
    response = ola::rdm::RDMResponse::InflateFromData(
        frame.data.data() + 1, frame.data.size() - 1, &status_code,
        request);
  }
  return new RDMReply(status_code, response, frames);
}

RDMReply* RDMReply::DUBReply(const RDMFrame &frame) {
  RDMFrames frames;
  frames.push_back(frame);
  return new RDMReply(ola::rdm::RDM_DUB_RESPONSE, NULL, frames);
}
}  // namespace rdm
}  // namespace ola
