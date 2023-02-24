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
 * SubDeviceDispatcher.h
 * Handles dispatching RDM requests to the correct sub device.
 * Copyright (C) 2013 Simon Newton
 */

/**
 * @addtogroup rdm_controller
 * @{
 * @file SubDeviceDispatcher.h
 * @brief Handles the dispatching of RDM requests to the correct sub device.
 * @}
 */

#ifndef INCLUDE_OLA_RDM_SUBDEVICEDISPATCHER_H_
#define INCLUDE_OLA_RDM_SUBDEVICEDISPATCHER_H_

#include <ola/rdm/RDMControllerInterface.h>

#include <map>
#include <string>
#include <vector>

namespace ola {
namespace rdm {

class SubDeviceDispatcher: public ola::rdm::RDMControllerInterface {
 public:
  SubDeviceDispatcher() {}
  ~SubDeviceDispatcher() {}

  void AddSubDevice(uint16_t sub_device_number,
                    ola::rdm::RDMControllerInterface *device);

  void SendRDMRequest(ola::rdm::RDMRequest *request,
                      ola::rdm::RDMCallback *callback);

 private:
  struct FanOutTracker {
   public:
    FanOutTracker(uint16_t number_of_subdevices,
                  ola::rdm::RDMCallback *callback);

    bool IncrementAndCheckIfComplete() {
      return ++m_responses_so_far == m_number_of_subdevices;
    }

    void SetResponse(ola::rdm::RDMStatusCode code,
                     ola::rdm::RDMResponse *response);

    void RunCallback();

    uint16_t NumResponses() const {
      return m_responses_so_far;
    }

   private:
    uint16_t m_number_of_subdevices;
    uint16_t m_responses_so_far;
    ola::rdm::RDMCallback *m_callback;

    ola::rdm::RDMStatusCode m_status_code;
    ola::rdm::RDMResponse *m_response;
  };

  typedef std::map<uint16_t, ola::rdm::RDMControllerInterface*> SubDeviceMap;

  SubDeviceMap m_subdevices;

  void FanOutToSubDevices(const ola::rdm::RDMRequest *request,
                          ola::rdm::RDMCallback *callback);

  void NackIfNotBroadcast(const ola::rdm::RDMRequest *request,
                          ola::rdm::RDMCallback *callback,
                          ola::rdm::rdm_nack_reason nack_reason);

  void HandleSubDeviceResponse(FanOutTracker *tracker,
                               ola::rdm::RDMReply *reply);
};
}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_SUBDEVICEDISPATCHER_H_
