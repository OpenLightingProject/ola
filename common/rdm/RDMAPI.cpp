/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * RDMAPI.cpp
 * Provides a generic RDM API that can use different implementations.
 * Copyright (C) 2010 Simon Newton
 */

#include <stdint.h>
#include <map>
#include <string>
#include <vector>
#include "ola/Callback.h"
#include "ola/Logging.h"
#include "ola/rdm/RDMAPI.h"
#include "ola/rdm/RDMAPIImplInterface.h"
#include "ola/rdm/RDMEnums.h"
#include "ola/rdm/UID.h"

namespace ola {
namespace rdm {

using std::map;
using std::string;
using std::vector;
using ola::SingleUseCallback1;
using ola::SingleUseCallback2;
using ola::SingleUseCallback4;


ResponseStatus::ResponseStatus(const RDMAPIImplResponseStatus &status,
                               const string &data):
    m_was_broadcast(false),
    m_was_nacked(false),
    m_nack_reason(0),
    m_error(status.error) {
  if (m_error.empty()) {
    if (status.was_broadcast) {
      m_was_broadcast = true;
    } else if (status.response_type == ola::rdm::NACK_REASON) {
      if (data.size() < sizeof(m_nack_reason)) {
        m_error = "NACK_REASON data too small";
        OLA_WARN << m_error;
      } else {
        m_was_nacked = true;
        const uint8_t *ptr = reinterpret_cast<const uint8_t*>(
          data.c_str());
        m_nack_reason = (ptr[0] << 8) + ptr[1];
      }
    }
  }
}


/*
 * Return the number of queues messages for a UID. Note that this is cached on
 * the client side so this number may not be correct.
 * @param uid the UID to fetch the outstanding message count for
 * @return the number of outstanding messages
 */
uint8_t RDMAPI::OutstandingMessagesCount(const UID &uid) {
  map<UID, uint8_t>::const_iterator iter = m_outstanding_messages.find(uid);

  if (iter == m_outstanding_messages.end())
    return 0;
  return iter->second;
}


/*
 * Fetch the supported parameters list
 * @param uid the UID to fetch the outstanding message count for
 * @param sub_device the sub device to use
 * @param callback the callback to invoke when this request completes
 * @return true if the request is sent correctly, false otherwise
 */
bool RDMAPI::GetSupportedParameters(
    const UID &uid,
    uint16_t sub_device,
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       vector<uint16_t> > *callback) {
  return m_impl->RDMGet(NULL,
                        m_universe,
                        uid,
                        sub_device,
                        PID_SUPPORTED_PARAMETERS);
}


/*
 * Send a DEVICE_INFO Get command
 * @param uid the UID to fetch the outstanding message count for
 * @param sub_device the sub device to use
 * @param callback the callback to invoke when this request completes
 * @return true if the request is sent correctly, false otherwise
 */
bool RDMAPI::GetDeviceInfo(
  const UID &uid,
  uint16_t sub_device,
  SingleUseCallback2<void,
                     const ResponseStatus&,
                     const DeviceInfo&> *callback) {
  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::HandleGetDeviceInfo,
    callback);
  return m_impl->RDMGet(cb,
                        m_universe,
                        uid,
                        sub_device,
                        PID_DEVICE_INFO);
}


// Handlers follow. These are invoked by the RDMAPIImpl when responses arrive
// ----------------------------------------------------------------------------

/*
 * Handle a SUPPORTED_PARAMETERS Get command
 */
void RDMAPI::HandleGetSupportedParameters(
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       vector<uint16_t> > *callback,
    const RDMAPIImplResponseStatus &status,
    const string &data) {
  OLA_INFO << "get support pids response size was " << data.size();
  ResponseStatus response_status(status, data);

  // TODO(simon): we need to update the outstanding message counter here

  vector<uint16_t> pids;
  if (response_status.Error().empty() && !response_status.WasNacked()) {
    // unpack pids
  }

  callback->Run(response_status, pids);
}


/*
 * Handle a DEVICE_INFO Get command
 */
void RDMAPI::HandleGetDeviceInfo(
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       const DeviceInfo&> *callback,
    const RDMAPIImplResponseStatus &status,
    const string &data) {

  OLA_INFO << "device info response size was " << data.size();
  ResponseStatus response_status(status, data);
  DeviceInfo device_info;

  // TODO(simon): we need to update the outstanding message counter here

  if (response_status.Error().empty() && !response_status.WasNacked()) {
    // unpack Device info here
  }
  callback->Run(response_status, device_info);
}
}  // rdm
}  // ola
