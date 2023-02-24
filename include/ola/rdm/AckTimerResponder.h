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
 * AckTimerResponder.h
 * Copyright (C) 2013 Simon Newton
 */

/**
 * @addtogroup rdm_resp
 * @{
 * @file AckTimerResponder.h
 * @brief This responder implements the code needed to deal with AckTimers
 * @}
 */

#ifndef INCLUDE_OLA_RDM_ACKTIMERRESPONDER_H_
#define INCLUDE_OLA_RDM_ACKTIMERRESPONDER_H_

#include <ola/Clock.h>
#include <ola/rdm/RDMControllerInterface.h>
#include <ola/rdm/RDMEnums.h>
#include <ola/rdm/ResponderOps.h>
#include <ola/rdm/ResponderPersonality.h>
#include <ola/rdm/UID.h>

#include <queue>
#include <string>
#include <vector>

namespace ola {
namespace rdm {

/**
 * A responder that ACK_TIMERs certain GETs / SETs.
 */
class AckTimerResponder: public RDMControllerInterface {
 public:
  explicit AckTimerResponder(const UID &uid);
  ~AckTimerResponder();

  void SendRDMRequest(RDMRequest *request, RDMCallback *callback);

 private:
  /**
   * The RDM Operations for the AckTimerResponder.
   */
  class RDMOps : public ResponderOps<AckTimerResponder> {
   public:
    static RDMOps *Instance() {
      if (!instance)
        instance = new RDMOps();
      return instance;
    }

   private:
    RDMOps() : ResponderOps<AckTimerResponder>(PARAM_HANDLERS) {}

    static RDMOps *instance;
  };

  /**
   * The personalities
   */
  class Personalities : public PersonalityCollection {
   public:
    static const Personalities *Instance();

   private:
    explicit Personalities(const PersonalityList &personalities) :
      PersonalityCollection(personalities) {
    }

    static Personalities *instance;
  };

  // The actual queue of messages to be collected.
  typedef std::queue<class QueuedResponse*> ResponseQueue;

  // The list of responses which aren't available yet. When they become
  // valid they are moved to the ResponseQueue,
  typedef std::vector<class QueuedResponse*> PendingResponses;

  const UID m_uid;
  uint16_t m_start_address;
  bool m_identify_mode;
  PersonalityManager m_personality_manager;

  ResponseQueue m_queued_messages;
  PendingResponses m_upcoming_queued_messages;
  std::auto_ptr<class QueuedResponse> m_last_queued_message;
  ola::Clock m_clock;

  uint16_t Footprint() const {
    return m_personality_manager.ActivePersonalityFootprint();
  }

  uint8_t QueuedMessageCount() const;
  void QueueAnyNewMessages();

  RDMResponse *ResponseFromQueuedMessage(
      const RDMRequest *request,
      const class QueuedResponse *queued_response);
  RDMResponse *EmptyStatusMessage(const RDMRequest *request);
  RDMResponse *GetQueuedMessage(const RDMRequest *request);
  RDMResponse *GetDeviceInfo(const RDMRequest *request);
  RDMResponse *GetPersonality(const RDMRequest *request);
  RDMResponse *SetPersonality(const RDMRequest *request);
  RDMResponse *GetPersonalityDescription(const RDMRequest *request);
  RDMResponse *GetDmxStartAddress(const RDMRequest *request);
  RDMResponse *SetDmxStartAddress(const RDMRequest *request);
  RDMResponse *GetIdentify(const RDMRequest *request);
  RDMResponse *SetIdentify(const RDMRequest *request);
  RDMResponse *GetManufacturerLabel(const RDMRequest *request);
  RDMResponse *GetDeviceLabel(const RDMRequest *request);
  RDMResponse *GetDeviceModelDescription(const RDMRequest *request);
  RDMResponse *GetSoftwareVersionLabel(const RDMRequest *request);

  static const ResponderOps<AckTimerResponder>::ParamHandler PARAM_HANDLERS[];
  static const uint16_t ACK_TIMER_MS;
};
}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_ACKTIMERRESPONDER_H_
