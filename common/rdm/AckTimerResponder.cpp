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
 * AckTimerResponder.cpp
 * Copyright (C) 2013 Simon Newton
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H

#include <iostream>
#include <string>
#include <vector>
#include "ola/Constants.h"
#include "ola/Clock.h"
#include "ola/Logging.h"
#include "ola/base/Array.h"
#include "ola/network/NetworkUtils.h"
#include "ola/rdm/AckTimerResponder.h"
#include "ola/rdm/OpenLightingEnums.h"
#include "ola/rdm/RDMEnums.h"
#include "ola/rdm/ResponderHelper.h"

namespace ola {
namespace rdm {

using ola::network::HostToNetwork;
using ola::network::NetworkToHost;
using std::string;
using std::vector;

AckTimerResponder::RDMOps *AckTimerResponder::RDMOps::instance = NULL;

const AckTimerResponder::Personalities *
    AckTimerResponder::Personalities::Instance() {
  if (!instance) {
    PersonalityList personalities;
    personalities.push_back(Personality(0, "Personality 1"));
    personalities.push_back(Personality(5, "Personality 2"));
    personalities.push_back(Personality(10, "Personality 3"));
    personalities.push_back(Personality(20, "Personality 4"));
    instance = new Personalities(personalities);
  }
  return instance;
}

AckTimerResponder::Personalities *
  AckTimerResponder::Personalities::instance = NULL;

const ResponderOps<AckTimerResponder>::ParamHandler
    AckTimerResponder::PARAM_HANDLERS[] = {
  { PID_QUEUED_MESSAGE,
    &AckTimerResponder::GetQueuedMessage,
    NULL},
  { PID_DEVICE_INFO,
    &AckTimerResponder::GetDeviceInfo,
    NULL},
  { PID_DEVICE_MODEL_DESCRIPTION,
    &AckTimerResponder::GetDeviceModelDescription,
    NULL},
  { PID_MANUFACTURER_LABEL,
    &AckTimerResponder::GetManufacturerLabel,
    NULL},
  { PID_DEVICE_LABEL,
    &AckTimerResponder::GetDeviceLabel,
    NULL},
  { PID_SOFTWARE_VERSION_LABEL,
    &AckTimerResponder::GetSoftwareVersionLabel,
    NULL},
  { PID_DMX_PERSONALITY,
    &AckTimerResponder::GetPersonality,
    &AckTimerResponder::SetPersonality},
  { PID_DMX_PERSONALITY_DESCRIPTION,
    &AckTimerResponder::GetPersonalityDescription,
    NULL},
  { PID_DMX_START_ADDRESS,
    &AckTimerResponder::GetDmxStartAddress,
    &AckTimerResponder::SetDmxStartAddress},
  { PID_IDENTIFY_DEVICE,
    &AckTimerResponder::GetIdentify,
    &AckTimerResponder::SetIdentify},
  { 0, NULL, NULL},
};


/*
 * This class contains the information required to return a response to Get
 * QUEUED_MESSAGE.
 */
class QueuedResponse {
 public:
    // Takes ownership of the param data
    QueuedResponse(
        const ola::TimeStamp &valid_after,
        rdm_pid pid,
        RDMCommand::RDMCommandClass command_class,
        const uint8_t *param_data,
        unsigned int param_data_size)
        : m_valid_after(valid_after),
          m_pid(pid),
          m_command_class(command_class),
          m_param_data(param_data),
          m_param_data_size(param_data_size) {
    }

    ~QueuedResponse() {
      if (m_param_data) {
        delete[] m_param_data;
      }
    }

    bool IsValid(const TimeStamp &now) const {
      return now >= m_valid_after;
    }

    rdm_pid Pid() const { return m_pid; }
    RDMCommand::RDMCommandClass CommandClass() const { return m_command_class; }
    const uint8_t* ParamData() const { return m_param_data; }
    unsigned int ParamDataSize() const { return m_param_data_size; }

 private:
    ola::TimeStamp m_valid_after;
    rdm_pid m_pid;
    RDMCommand::RDMCommandClass m_command_class;
    const uint8_t *m_param_data;
    unsigned int m_param_data_size;
};

// Use 400ms for the ack timers.
const uint16_t AckTimerResponder::ACK_TIMER_MS = 400;

/**
 * New AckTimerResponder
 */
AckTimerResponder::AckTimerResponder(const UID &uid)
    : m_uid(uid),
      m_start_address(1),
      m_identify_mode(false),
      m_personality_manager(Personalities::Instance()) {
}

/**
 * Clean up
 */
AckTimerResponder::~AckTimerResponder() {
  STLDeleteElements(&m_upcoming_queued_messages);
  while (!m_queued_messages.empty()) {
    delete m_queued_messages.front();
    m_queued_messages.pop();
  }
}

/*
 * Handle an RDM Request
 */
void AckTimerResponder::SendRDMRequest(RDMRequest *request,
                                       RDMCallback *callback) {
  // Queue any messages here
  QueueAnyNewMessages();
  OLA_DEBUG << " Queued message count is now " << m_queued_messages.size();
  RDMOps::Instance()->HandleRDMRequest(this, m_uid, ROOT_RDM_DEVICE, request,
                                       callback);
}

/**
 * Get the number of queued messages, capping it at 255
 */
uint8_t AckTimerResponder::QueuedMessageCount() const {
  unsigned int size = m_queued_messages.size();
  return size > MAX_QUEUED_MESSAGE_COUNT ? MAX_QUEUED_MESSAGE_COUNT : size;
}

/**
 * Move any 'new' queued messages in to the queue.
 */
void AckTimerResponder::QueueAnyNewMessages() {
  TimeStamp now;
  m_clock.CurrentMonotonicTime(&now);
  PendingResponses::iterator iter = m_upcoming_queued_messages.begin();
  while (iter != m_upcoming_queued_messages.end()) {
    if ((*iter)->IsValid(now)) {
      m_queued_messages.push(*iter);
      iter = m_upcoming_queued_messages.erase(iter);
    } else {
      ++iter;
    }
  }
}

/**
 * Build a RDM response from a QueuedResponse
 */
RDMResponse *AckTimerResponder::ResponseFromQueuedMessage(
    const RDMRequest *request,
    const class QueuedResponse *queued_response) {
  switch (queued_response->CommandClass()) {
    case RDMCommand::GET_COMMAND_RESPONSE:
      return new RDMGetResponse(
        // coverity[SWAPPED_ARGUMENTS]
        request->DestinationUID(),
        request->SourceUID(),
        request->TransactionNumber(),
        RDM_ACK,
        QueuedMessageCount(),
        ROOT_RDM_DEVICE,
        queued_response->Pid(),
        queued_response->ParamData(),
        queued_response->ParamDataSize());
    case RDMCommand::SET_COMMAND_RESPONSE:
      return new RDMSetResponse(
        // coverity[SWAPPED_ARGUMENTS]
        request->DestinationUID(),
        request->SourceUID(),
        request->TransactionNumber(),
        RDM_ACK,
        QueuedMessageCount(),
        ROOT_RDM_DEVICE,
        queued_response->Pid(),
        queued_response->ParamData(),
        queued_response->ParamDataSize());
    default:
      OLA_WARN << "Queued message returning NULL, CC was "
               << static_cast<int>(queued_response->CommandClass());
      return NULL;
  }
}

/**
 * Return an empty STATUS_MESSAGES response.
 */
RDMResponse *AckTimerResponder::EmptyStatusMessage(
    const RDMRequest *request) {
  return GetResponseWithPid(request, PID_STATUS_MESSAGES, NULL, 0, RDM_ACK,
                            QueuedMessageCount());
}

/**
 * PID_QUEUED_MESSAGE
 */
RDMResponse *AckTimerResponder::GetQueuedMessage(const RDMRequest *request) {
  uint8_t status_type;
  if (!ResponderHelper::ExtractUInt8(request, &status_type)) {
    return NackWithReason(request, NR_FORMAT_ERROR, QueuedMessageCount());
  }

  if (m_queued_messages.empty()) {
    // respond with empty status message
    return EmptyStatusMessage(request);
  }

  if (status_type == STATUS_GET_LAST_MESSAGE) {
    if (m_last_queued_message.get()) {
      return ResponseFromQueuedMessage(request, m_last_queued_message.get());
    } else {
      return EmptyStatusMessage(request);
    }
  }

  m_last_queued_message.reset(m_queued_messages.front());
  m_queued_messages.pop();
  RDMResponse *response = ResponseFromQueuedMessage(
      request, m_last_queued_message.get());
  OLA_DEBUG << *response;
  return response;
}

/**
 * PID_DEVICE_INFO
 */
RDMResponse *AckTimerResponder::GetDeviceInfo(const RDMRequest *request) {
  return ResponderHelper::GetDeviceInfo(
      request, OLA_ACK_TIMER_MODEL,
      PRODUCT_CATEGORY_TEST, 1,
      &m_personality_manager,
      m_start_address,
      0, 0, QueuedMessageCount());
}

/**
 * PID_DMX_PERSONALITY
 */
RDMResponse *AckTimerResponder::GetPersonality(const RDMRequest *request) {
  return ResponderHelper::GetPersonality(request, &m_personality_manager,
                                         QueuedMessageCount());
}

RDMResponse *AckTimerResponder::SetPersonality(const RDMRequest *request) {
  return ResponderHelper::SetPersonality(request, &m_personality_manager,
                                         m_start_address, QueuedMessageCount());
}

/**
 * PID_DMX_PERSONALITY_DESCRIPTION
 */
RDMResponse *AckTimerResponder::GetPersonalityDescription(
    const RDMRequest *request) {
  return ResponderHelper::GetPersonalityDescription(
      request, &m_personality_manager, QueuedMessageCount());
}

/**
 * PID_DMX_START_ADDRESS
 */
RDMResponse *AckTimerResponder::GetDmxStartAddress(const RDMRequest *request) {
  return ResponderHelper::GetDmxAddress(request, &m_personality_manager,
                                        m_start_address, QueuedMessageCount());
}

RDMResponse *AckTimerResponder::SetDmxStartAddress(const RDMRequest *request) {
  uint16_t address;
  if (!ResponderHelper::ExtractUInt16(request, &address)) {
    return NackWithReason(request, NR_FORMAT_ERROR, QueuedMessageCount());
  }

  uint16_t end_address = (1 + DMX_UNIVERSE_SIZE -
                          m_personality_manager.ActivePersonalityFootprint());
  if (address == 0 || address > end_address) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE, QueuedMessageCount());
  } else if (Footprint() == 0) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE, QueuedMessageCount());
  }

  m_start_address = address;

  TimeStamp valid_after;
  m_clock.CurrentMonotonicTime(&valid_after);
  valid_after += TimeInterval(0, ACK_TIMER_MS * 1000);

  QueuedResponse *our_response = new QueuedResponse(
      valid_after, PID_DMX_START_ADDRESS, RDMCommand::SET_COMMAND_RESPONSE,
      NULL, 0);
  m_upcoming_queued_messages.push_back(our_response);

  uint16_t ack_time = 1 + ACK_TIMER_MS / 100;
  ack_time = HostToNetwork(ack_time);
  return GetResponseFromData(request,
                             reinterpret_cast<uint8_t*>(&ack_time),
                             sizeof(ack_time),
                             RDM_ACK_TIMER,
                             QueuedMessageCount());
}

/**
 * PID_IDENTIFY_DEVICE
 */
RDMResponse *AckTimerResponder::GetIdentify(const RDMRequest *request) {
  return ResponderHelper::GetBoolValue(request, m_identify_mode,
                                       QueuedMessageCount());
}

RDMResponse *AckTimerResponder::SetIdentify(const RDMRequest *request) {
  uint8_t arg;
  if (!ResponderHelper::ExtractUInt8(request, &arg)) {
    return NackWithReason(request, NR_FORMAT_ERROR, QueuedMessageCount());
  }

  if (arg != 0 && arg != 1) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE, QueuedMessageCount());
  }

  bool old_value = m_identify_mode;
  m_identify_mode = arg;
  if (m_identify_mode != old_value) {
    OLA_INFO << "Ack Timer Responder" << m_uid << ", identify mode "
             << (m_identify_mode ? "on" : "off");
  }

  TimeStamp valid_after;
  m_clock.CurrentMonotonicTime(&valid_after);
  valid_after += TimeInterval(0, ACK_TIMER_MS * 1000);

  QueuedResponse *our_response = new QueuedResponse(
      valid_after, PID_IDENTIFY_DEVICE, RDMCommand::SET_COMMAND_RESPONSE,
      NULL, 0);
  m_upcoming_queued_messages.push_back(our_response);

  uint16_t ack_time = 1 + ACK_TIMER_MS / 100;
  ack_time = HostToNetwork(ack_time);
  return GetResponseFromData(request,
                             reinterpret_cast<uint8_t*>(&ack_time),
                             sizeof(ack_time),
                             RDM_ACK_TIMER,
                             QueuedMessageCount());
}

RDMResponse *AckTimerResponder::GetDeviceModelDescription(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, "OLA Ack Timer Responder",
                                    QueuedMessageCount());
}

RDMResponse *AckTimerResponder::GetManufacturerLabel(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, OLA_MANUFACTURER_LABEL,
                                    QueuedMessageCount());
}

RDMResponse *AckTimerResponder::GetDeviceLabel(const RDMRequest *request) {
  return ResponderHelper::GetString(request, "Ack Timer Responder",
                                    QueuedMessageCount());
}

RDMResponse *AckTimerResponder::GetSoftwareVersionLabel(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, string("OLA Version ") + VERSION,
                                    QueuedMessageCount());
}
}  // namespace rdm
}  // namespace ola
