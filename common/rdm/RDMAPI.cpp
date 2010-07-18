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
#include <algorithm>
#include <map>
#include <string>
#include <vector>
#include "ola/Callback.h"
#include "ola/Logging.h"
#include "ola/network/NetworkUtils.h"
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
using ola::network::HostToNetwork;
using ola::network::NetworkToHost;


ResponseStatus::ResponseStatus(const RDMAPIImplResponseStatus &status,
                               const string &data):
    m_response_type(VALID_RESPONSE),
    m_nack_reason(0),
    m_error(status.error) {
  if (!m_error.empty()) {
    m_response_type = TRANSPORT_ERROR;
  } else {
    if (status.was_broadcast) {
      m_response_type = BROADCAST_REQUEST;
    } else if (status.response_type == ola::rdm::NACK_REASON) {
      if (data.size() < sizeof(m_nack_reason)) {
        m_response_type = MALFORMED_RESPONSE;
        m_error = "NACK_REASON data too small";
      } else {
        m_response_type = REQUEST_NACKED;
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
 * Fetch a count of the proxied devices
 * @param uid the UID of the device to address this message to
 * @param callback the Callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return false if an error occurred, true otherwise
 */
bool RDMAPI::GetProxiedDeviceCount(
    const UID &uid,
    SingleUseCallback3<void,
                       const ResponseStatus&,
                       uint16_t,
                       bool> *callback,
    string *error) {
  if (CheckNotBroadcast(uid, error))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleGetProxiedDeviceCount,
    callback);
  return CheckReturnStatus(
      m_impl->RDMGet(cb,
                     m_universe,
                     uid,
                     ROOT_RDM_DEVICE,
                     PID_PROXIED_DEVICE_COUNT),
      error);
}


/*
 * Fetch a list of the proxied devices
 * @param uid the UID of the device to address this message to
 * @param callback the Callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return false if an error occurred, true otherwise
 */
bool RDMAPI::GetProxiedDevices(
    const UID &uid,
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       const vector<UID>&> *callback,
    string *error) {
  if (CheckNotBroadcast(uid, error))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleGetProxiedDevices,
    callback);
  return CheckReturnStatus(
      m_impl->RDMGet(cb,
                     m_universe,
                     uid,
                     ROOT_RDM_DEVICE,
                     PID_PROXIED_DEVICES),
      error);
}


/*
 * Get the communication status report
 * @param uid the UID of the device to address this message to
 * @param callback the Callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return false if an error occurred, true otherwise
 */
bool RDMAPI::GetCommStatus(
    const UID &uid,
    SingleUseCallback4<void,
                       const ResponseStatus&,
                       uint16_t,
                       uint16_t,
                       uint16_t> *callback,
    string *error) {
  if (CheckNotBroadcast(uid, error))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleGetCommStatus,
    callback);
  return CheckReturnStatus(
      m_impl->RDMGet(cb,
                     m_universe,
                     uid,
                     ROOT_RDM_DEVICE,
                     PID_COMMS_STATUS),
      error);
}


/*
 * Clear the Communication status
 * @param uid the UID of the device to address this message to
 * @param callback the Callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return false if an error occurred, true otherwise
 */
bool RDMAPI::ClearCommStatus(
    const UID &uid,
    SingleUseCallback1<void, const ResponseStatus&> *callback,
    string *error) {
  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleEmptyResponse,
    callback);
  return CheckReturnStatus(
      m_impl->RDMSet(cb,
                     m_universe,
                     uid,
                     ROOT_RDM_DEVICE,
                     PID_COMMS_STATUS),
      error);
}


/*
 * Get the status information from a device
 * @param uid the UID of the device to address this message to
 * @param status_type the Status Type requested
 * @param callback the Callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return false if an error occurred, true otherwise
 */
bool RDMAPI::GetStatusMessage(
  const UID &uid,
  rdm_status_type status_type,
  SingleUseCallback2<void,
                     const ResponseStatus&,
                     const vector<StatusMessage> > *callback,
  string *error) {
  if (CheckNotBroadcast(uid, error))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleGetStatusMessage,
    callback);
  uint8_t type = static_cast<uint8_t>(status_type);
  return CheckReturnStatus(
      m_impl->RDMGet(cb,
                     m_universe,
                     uid,
                     ROOT_RDM_DEVICE,
                     PID_STATUS_MESSAGES,
                     &type,
                     sizeof(type)),
      error);
}


/*
 * Fetch the description for a status id
 * @param uid the UID of the device to address this message to
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
 */
bool RDMAPI::GetStatusIdDescription(
    const UID &uid,
    uint16_t status_id,
    SingleUseCallback2<void, const ResponseStatus&, const string&> *callback,
    string *error) {
  if (CheckNotBroadcast(uid, error))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleGetStatusIdDescription,
    callback);
  status_id = HostToNetwork(status_id);
  return CheckReturnStatus(
      m_impl->RDMGet(cb,
                     m_universe,
                     uid,
                     ROOT_RDM_DEVICE,
                     PID_STATUS_ID_DESCRIPTION,
                     reinterpret_cast<const uint8_t*>(&status_id),
                     sizeof(status_id)),
      error);
}


/*
 * Clear the status message queue
 * @param uid the UID of the device to address this message to
 * @param sub_device the sub device to use
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
 */
bool RDMAPI::ClearStatusId(
    const UID &uid,
    uint16_t sub_device,
    SingleUseCallback1<void, const ResponseStatus&> *callback,
    string *error) {
  if (CheckNotBroadcast(uid, error))
    return false;

  if (CheckValidSubDevice(sub_device, true, error))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleEmptyResponse,
    callback);
  return CheckReturnStatus(
      m_impl->RDMSet(cb,
                     m_universe,
                     uid,
                     sub_device,
                     PID_CLEAR_STATUS_ID),
      error);
}


/*
 * Get the reporting threshold for a device
 * @param uid the UID of the device to address this message to
 * @param sub_device the sub device to use
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
 */
bool RDMAPI::GetSubDeviceReporting(
    const UID &uid,
    uint16_t sub_device,
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       uint8_t> *callback,
    string *error) {
  if (CheckNotBroadcast(uid, error))
    return false;

  if (CheckValidSubDevice(sub_device, false, error))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleGetSubDeviceReporting,
    callback);
  return CheckReturnStatus(
      m_impl->RDMGet(cb,
                     m_universe,
                     uid,
                     sub_device,
                     PID_SUB_DEVICE_STATUS_REPORT_THRESHOLD),
      error);
}


/*
 * Set the reporting threshold for a device
 * @param uid the UID of the device to address this message to
 * @param sub_device the sub device to use
 * @param status_type the Status Type to set the threshold as
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
 */
bool RDMAPI::SetSubDeviceReporting(
    const UID &uid,
    uint16_t sub_device,
    rdm_status_type status_type,
    SingleUseCallback1<void, const ResponseStatus&> *callback,
    string *error) {
  if (CheckNotBroadcast(uid, error))
    return false;

  if (CheckValidSubDevice(sub_device, true, error))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleEmptyResponse,
    callback);
  uint8_t type = static_cast<uint8_t>(status_type);
  return CheckReturnStatus(
      m_impl->RDMSet(cb,
                     m_universe,
                     uid,
                     ROOT_RDM_DEVICE,
                     PID_SUB_DEVICE_STATUS_REPORT_THRESHOLD,
                     &type,
                     sizeof(type)),
      error);
}


/*
 * Fetch the supported parameters list
 * @param uid the UID of the device to address this message to
 * @param sub_device the sub device to use
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
 */
bool RDMAPI::GetSupportedParameters(
    const UID &uid,
    uint16_t sub_device,
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       vector<uint16_t> > *callback,
    string *error) {
  if (CheckNotBroadcast(uid, error))
    return false;

  if (CheckValidSubDevice(sub_device, false, error))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleGetSupportedParameters,
    callback);
  return m_impl->RDMGet(cb,
                        m_universe,
                        uid,
                        sub_device,
                        PID_SUPPORTED_PARAMETERS);
}


/*
 * Fetch the description of a param ID
 * @param uid the UID of the device to address this message to
 * @param pid the parameter id to fetch the description for
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
 */
bool RDMAPI::GetParameterDescription(
    const UID &uid,
    uint16_t pid,
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       const ParameterDescription&> *callback,
    string *error) {
  if (CheckNotBroadcast(uid, error))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleGetParameterDescription,
    callback);
  pid = HostToNetwork(pid);
  return CheckReturnStatus(
      m_impl->RDMGet(cb,
                     m_universe,
                     uid,
                     ROOT_RDM_DEVICE,
                     PID_STATUS_ID_DESCRIPTION,
                     reinterpret_cast<const uint8_t*>(&pid),
                     sizeof(pid)),
      error);
}


/*
 * Fetch the device information
 * @param uid the UID to fetch the outstanding message count for
 * @param sub_device the sub device to use
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
 */
bool RDMAPI::GetDeviceInfo(
    const UID &uid,
    uint16_t sub_device,
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       const DeviceInfo&> *callback,
  string *error) {
  if (CheckNotBroadcast(uid, error))
    return false;

  if (CheckValidSubDevice(sub_device, false, error))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleGetDeviceInfo,
    callback);
  return m_impl->RDMGet(cb,
                        m_universe,
                        uid,
                        sub_device,
                        PID_DEVICE_INFO);
}


/*
 * Fetch the product detail IDs.
 * @param uid the UID to fetch the outstanding message count for
 * @param sub_device the sub device to use
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
 */
bool RDMAPI::GetProductDetailIdList(
    const UID &uid,
    uint16_t sub_device,
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       vector<uint16_t> > *callback,
    string *error) {
  if (CheckNotBroadcast(uid, error))
    return false;

  if (CheckValidSubDevice(sub_device, false, error))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleGetProductDetailIdList,
    callback);
  return m_impl->RDMGet(cb,
                        m_universe,
                        uid,
                        sub_device,
                        PID_PRODUCT_DETAIL_ID_LIST);
}


/*
 * Fetch the description for a device model.
 * @param uid the UID to fetch the outstanding message count for
 * @param sub_device the sub device to use
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
 */
bool RDMAPI::GetDeviceModelDescription(
    const UID &uid,
    uint16_t sub_device,
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       const string&> *callback,
    string *error) {
  if (CheckNotBroadcast(uid, error))
    return false;
  if (CheckValidSubDevice(sub_device, false, error))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleLabelResponse,
    callback);
  return m_impl->RDMGet(cb,
                        m_universe,
                        uid,
                        sub_device,
                        PID_DEVICE_MODEL_DESCRIPTION);
}


/*
 * Fetch the manufacturer label for a device
 * @param uid the UID to fetch the outstanding message count for
 * @param sub_device the sub device to use
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
 */
bool RDMAPI::GetManufacturerLabel(
    const UID &uid,
    uint16_t sub_device,
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       const string&> *callback,
    string *error) {
  if (CheckNotBroadcast(uid, error))
    return false;
  if (CheckValidSubDevice(sub_device, false, error))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleLabelResponse,
    callback);
  return m_impl->RDMGet(cb,
                        m_universe,
                        uid,
                        sub_device,
                        PID_MANUFACTURER_LABEL);
}


/*
 * Fetch the device label
 * @param uid the UID to fetch the outstanding message count for
 * @param sub_device the sub device to use
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
 */
bool RDMAPI::GetDeviceLabel(
    const UID &uid,
    uint16_t sub_device,
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       const string&> *callback,
    string *error) {
  if (CheckNotBroadcast(uid, error))
    return false;
  if (CheckValidSubDevice(sub_device, false, error))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleLabelResponse,
    callback);
  return m_impl->RDMGet(cb,
                        m_universe,
                        uid,
                        sub_device,
                        PID_DEVICE_LABEL);
}


/*
 * Set the device label
 * @param uid the UID to fetch the outstanding message count for
 * @param sub_device the sub device to use
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
 */
bool RDMAPI::SetDeviceLabel(
    const UID &uid,
    uint16_t sub_device,
    const string &label,
    SingleUseCallback1<void, const ResponseStatus&> *callback,
    string *error) {
  // It doesn't really make sense to broadcast this but allow it anyway
  if (CheckValidSubDevice(sub_device, true, error))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleEmptyResponse,
    callback);
  return m_impl->RDMSet(cb,
                        m_universe,
                        uid,
                        sub_device,
                        PID_DEVICE_LABEL,
                        reinterpret_cast<const uint8_t*>(label.data()),
                        label.size());
}


/*
 * Check if a device is using the factory defaults
 * @param uid the UID to fetch the outstanding message count for
 * @param sub_device the sub device to use
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
 */
bool RDMAPI::GetFactoryDefaults(
    const UID &uid,
    uint16_t sub_device,
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       bool> *callback,
    string *error) {
  if (CheckNotBroadcast(uid, error))
    return false;
  if (CheckValidSubDevice(sub_device, false, error))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleGetFactoryDefaults,
    callback);
  return m_impl->RDMGet(cb,
                        m_universe,
                        uid,
                        sub_device,
                        PID_FACTORY_DEFAULTS);
}


/*
 * Reset a device to factory defaults
 * @param uid the UID to fetch the outstanding message count for
 * @param sub_device the sub device to use
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
 */
bool RDMAPI::ResetToFactoryDefaults(
    const UID &uid,
    uint16_t sub_device,
    SingleUseCallback1<void, const ResponseStatus&> *callback,
    string *error) {
  if (CheckNotBroadcast(uid, error))
    return false;
  if (CheckValidSubDevice(sub_device, true, error))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleEmptyResponse,
    callback);
  return m_impl->RDMGet(cb,
                        m_universe,
                        uid,
                        sub_device,
                        PID_FACTORY_DEFAULTS);
}


/*
 * Get the list of languages this device supports
 * @param uid the UID to fetch the outstanding message count for
 * @param sub_device the sub device to use
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
 */
bool RDMAPI::GetLanguageCapabilities(
    const UID &uid,
    uint16_t sub_device,
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       vector<string>&> *callback,
    string *error) {
  if (CheckNotBroadcast(uid, error))
    return false;
  if (CheckValidSubDevice(sub_device, false, error))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleGetLanguageCapabilities,
    callback);
  return m_impl->RDMGet(cb,
                        m_universe,
                        uid,
                        sub_device,
                        PID_LANGUAGE_CAPABILITIES);
}


/*
 * Get the language for this device
 * @param uid the UID to fetch the outstanding message count for
 * @param sub_device the sub device to use
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
 */
bool RDMAPI::GetLanguage(
    const UID &uid,
    uint16_t sub_device,
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       const string&> *callback,
    string *error) {
  if (CheckNotBroadcast(uid, error))
    return false;
  if (CheckValidSubDevice(sub_device, false, error))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleGetLanguage,
    callback);
  return m_impl->RDMGet(cb,
                        m_universe,
                        uid,
                        sub_device,
                        PID_LANGUAGE);
}

/*
 * Set the language for this device
 * @param uid the UID to fetch the outstanding message count for
 * @param sub_device the sub device to use
 * @param language the language code, only the first two characters are used
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
 */
bool RDMAPI::SetLanguage(
    const UID &uid,
    uint16_t sub_device,
    const string &language,
    SingleUseCallback1<void, const ResponseStatus&> *callback,
    string *error) {
  static const unsigned int DATA_SIZE = 2;
  if (CheckValidSubDevice(sub_device, true, error))
    return false;

  if (language.size() != DATA_SIZE) {
    if (error)
      *error = "Language must be a two letter code";
    return false;
  }

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleEmptyResponse,
    callback);
  return m_impl->RDMSet(cb,
                        m_universe,
                        uid,
                        sub_device,
                        PID_LANGUAGE,
                        reinterpret_cast<const uint8_t*>(language.data()),
                        DATA_SIZE);
}


/*
 * Get the software version label
 * @param uid the UID to fetch the outstanding message count for
 * @param sub_device the sub device to use
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
 */
bool RDMAPI::GetSoftwareVersionLabel(
    const UID &uid,
    uint16_t sub_device,
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       const string&> *callback,
    string *error) {
  if (CheckNotBroadcast(uid, error))
    return false;
  if (CheckValidSubDevice(sub_device, false, error))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleLabelResponse,
    callback);
  return m_impl->RDMGet(cb,
                        m_universe,
                        uid,
                        sub_device,
                        PID_SOFTWARE_VERSION_LABEL);
}


/*
 * Get the boot software version.
 * @param uid the UID to fetch the outstanding message count for
 * @param sub_device the sub device to use
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
 */
bool RDMAPI::GetBootSoftwareVersion(
    const UID &uid,
    uint16_t sub_device,
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       uint32_t> *callback,
    string *error) {
  if (CheckNotBroadcast(uid, error))
    return false;
  if (CheckValidSubDevice(sub_device, false, error))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleGetBootSoftwareVersion,
    callback);
  return m_impl->RDMGet(cb,
                        m_universe,
                        uid,
                        sub_device,
                        PID_BOOT_SOFTWARE_VERSION_ID);
}


/*
 * Get the boot software version label
 * @param uid the UID to fetch the outstanding message count for
 * @param sub_device the sub device to use
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
 */
bool RDMAPI::GetBootSoftwareVersionLabel(
    const UID &uid,
    uint16_t sub_device,
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       const string&> *callback,
    string *error) {
  if (CheckNotBroadcast(uid, error))
    return false;
  if (CheckValidSubDevice(sub_device, false, error))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleLabelResponse,
    callback);
  return m_impl->RDMGet(cb,
                        m_universe,
                        uid,
                        sub_device,
                        PID_BOOT_SOFTWARE_VERSION_LABEL);
}

// Handlers follow. These are invoked by the RDMAPIImpl when responses arrive
// ----------------------------------------------------------------------------

/*
 * Handle a response that contains a 32 byte ascii string
 */
void RDMAPI::_HandleLabelResponse(
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       const string&> *callback,
    const RDMAPIImplResponseStatus &status,
    const string &data) {
  static const unsigned int MAX_DATA_SIZE = 32;
  ResponseStatus response_status(status, data);
  if (response_status.ResponseType() == ResponseStatus::VALID_RESPONSE &&
      data.size() > MAX_DATA_SIZE) {
    std::stringstream str;
    str << "PDL needs to be <= " << MAX_DATA_SIZE << ", was " << data.size();
    response_status.MalformedResponse(str.str());
  }
  callback->Run(response_status, data);
}

/*
 * HAndle a response that doesn't contain any data
 */
void RDMAPI::_HandleEmptyResponse(
    SingleUseCallback1<void, const ResponseStatus&> *callback,
    const RDMAPIImplResponseStatus &status,
    const string &data) {
  ResponseStatus response_status(status, data);
  if (response_status.ResponseType() == ResponseStatus::VALID_RESPONSE &&
      data.size())
    SetIncorrectPDL(&response_status, data.size(), 0);
  callback->Run(response_status);
}


/*
 * Handle a PROXIED_DEVICE_COUNT get response
 */
void RDMAPI::_HandleGetProxiedDeviceCount(
    SingleUseCallback3<void,
                       const ResponseStatus&,
                       uint16_t,
                       bool> *callback,
    const RDMAPIImplResponseStatus &status,
    const string &data) {
  static const unsigned int DATA_SIZE = 3;
  ResponseStatus response_status(status, data);

  uint16_t device_count = 0;
  bool list_change = false;

  if (response_status.ResponseType() == ResponseStatus::VALID_RESPONSE) {
    if (data.size() >= DATA_SIZE) {
      struct {
        uint16_t device_count;
        uint8_t list_change;
      } unpacked_data;
      memcpy(&unpacked_data, data.data(), DATA_SIZE);
      device_count = unpacked_data.device_count;
      list_change = unpacked_data.list_change;
    } else {
      SetIncorrectPDL(&response_status, data.size(), DATA_SIZE);
    }
  }
  callback->Run(response_status, device_count, list_change);
}


/*
 * Handle a PROXIED_DEVICES get response
 */
void RDMAPI::_HandleGetProxiedDevices(
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       const vector<UID>&> *callback,
    const RDMAPIImplResponseStatus &status,
    const string &data) {
  ResponseStatus response_status(status, data);
  vector<UID> uids;

  unsigned int data_size = data.size();
  if (response_status.ResponseType() == ResponseStatus::VALID_RESPONSE) {
    if (data_size % UID::UID_SIZE == 0) {
      const uint8_t *start = reinterpret_cast<const uint8_t*>(data.data());
      for (const uint8_t *ptr = start; ptr < start + data_size;
           ptr += UID::UID_SIZE) {
        UID uid(ptr);
        uids.push_back(uid);
      }
    } else {
      response_status.MalformedResponse("PDL size not a multiple of " +
          IntToString(static_cast<int>(UID::UID_SIZE)) + " : " +
          IntToString(static_cast<int>(data_size)));
    }
  }
  callback->Run(response_status, uids);
}


/*
 * Handle a get COMMS_STATUS response
 */
void RDMAPI::_HandleGetCommStatus(
    SingleUseCallback4<void,
                       const ResponseStatus&,
                       uint16_t,
                       uint16_t,
                       uint16_t> *callback,
    const RDMAPIImplResponseStatus &status,
    const string &data) {
  static const unsigned int DATA_SIZE = 6;
  ResponseStatus response_status(status, data);

  uint16_t short_message = 0, length_mismatch = 0, checksum_fail = 0;

  if (response_status.ResponseType() == ResponseStatus::VALID_RESPONSE) {
    if (data.size() >= DATA_SIZE) {
      struct {
        uint16_t short_message;
        uint16_t length_mismatch;
        uint16_t checksum_fail;
      } unpacked_data;
      memcpy(&unpacked_data, data.data(), DATA_SIZE);
      short_message = unpacked_data.short_message;
      length_mismatch = unpacked_data.length_mismatch;
      checksum_fail = unpacked_data.checksum_fail;
    } else {
      SetIncorrectPDL(&response_status, data.size(), DATA_SIZE);
    }
  }
  callback->Run(response_status, short_message, length_mismatch,
                checksum_fail);
}


/*
 * Handle a STATUS_MESSAGES response
 */
void RDMAPI::_HandleGetStatusMessage(
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       const vector<StatusMessage> > *callback,
    const RDMAPIImplResponseStatus &status,
    const string &data) {

  // Seriously WTF, learn how to align data structures
  struct status_message {
    uint8_t sub_device_hi;
    uint8_t sub_device_lo;
    uint8_t status_type;
    uint8_t message_id_hi;
    uint8_t message_id_lo;
    uint8_t value_1_hi;
    uint8_t value_1_lo;
    uint8_t value_2_hi;
    uint8_t value_2_lo;
  } message;

  ResponseStatus response_status(status, data);
  vector<StatusMessage> messages;
  unsigned int data_size = data.size();

  if (response_status.ResponseType() == ResponseStatus::VALID_RESPONSE) {
    if (data_size % sizeof(message) == 0) {
      const uint8_t *start = reinterpret_cast<const uint8_t*>(data.data());
      for (const uint8_t *ptr = start; ptr < start + data_size;
           ptr += sizeof(message)) {
        memcpy(&message, ptr, sizeof(message));
        StatusMessage msg_object(
           message.sub_device_hi << 8 + message.sub_device_lo,
           message.status_type,
           message.message_id_hi << 8 + message.message_id_lo,
           message.value_1_hi << 8 + message.value_1_lo,
           message.value_2_hi << 8 + message.value_2_lo);
        messages.push_back(msg_object);
      }
    } else {
      response_status.MalformedResponse("PDL size not a multiple of " +
          IntToString(static_cast<int>(sizeof(message))) + " : " +
          IntToString(static_cast<int>(data_size)));
    }
  }
  callback->Run(response_status, messages);
}


/*
 * Handle a STATUS_ID_DESCRIPTION message
 */
void RDMAPI::_HandleGetStatusIdDescription(
    SingleUseCallback2<void, const ResponseStatus&, const string&> *callback,
    const RDMAPIImplResponseStatus &status,
    const string &data) {
  ResponseStatus response_status(status, data);
  callback->Run(response_status, data);
}


/*
 * Handle a get SUB_DEVICE_STATUS_REPORT_THRESHOLD message
 */
void RDMAPI::_HandleGetSubDeviceReporting(
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       uint8_t> *callback,
    const RDMAPIImplResponseStatus &status,
    const string &data) {
  ResponseStatus response_status(status, data);
  uint8_t status_type = 0;

  if (response_status.ResponseType() == ResponseStatus::VALID_RESPONSE) {
    if (data.size() == sizeof(status_type)) {
      status_type = data.data()[0];
    } else {
      SetIncorrectPDL(&response_status, data.size(), sizeof(status_type));
    }
  }
  callback->Run(response_status, status_type);
}


/*
 * Handle a SUPPORTED_PARAMETERS Get command
 */
void RDMAPI::_HandleGetSupportedParameters(
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       vector<uint16_t> > *callback,
    const RDMAPIImplResponseStatus &status,
    const string &data) {
  ResponseStatus response_status(status, data);
  vector<uint16_t> pids;

  unsigned int data_size = data.size();
  if (response_status.ResponseType() == ResponseStatus::VALID_RESPONSE) {
    if (data_size % 2 == 0) {
      const uint16_t *start = reinterpret_cast<const uint16_t*>(data.data());
      for (const uint16_t *ptr = start; ptr < start + data_size; ptr++) {
        pids.push_back(NetworkToHost(*ptr));
      }
    } else {
      response_status.MalformedResponse("PDL size not a multiple of 2 : " +
          IntToString(static_cast<int>(data_size)));
    }
  }
  callback->Run(response_status, pids);
}


/*
 * Handle a PARAMETER_DESCRIPTION message
 */
void RDMAPI::_HandleGetParameterDescription(
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       const ParameterDescription&> *callback,
    const RDMAPIImplResponseStatus &status,
    const string &data) {
  enum {DESCRIPTION_SIZE = 32};
  ResponseStatus response_status(status, data);
  ParameterDescription description;

  if (response_status.ResponseType() == ResponseStatus::VALID_RESPONSE) {
    struct param_description {
      uint16_t pid;
      uint8_t pdl_size;
      uint8_t data_type;
      uint8_t command_class;
      uint8_t type;
      uint8_t unit;
      uint8_t prefix;
      uint32_t min_value;
      uint32_t default_value;
      uint32_t max_value;
      // +1 for a null since it's not clear in the spec if this is null
      // terminated
      char description[DESCRIPTION_SIZE + 1];
    } __attribute__((packed));
    struct param_description raw_description;

    unsigned int max = sizeof(raw_description) - 1;
    unsigned int min = max - DESCRIPTION_SIZE;
    unsigned int data_size = data.size();
    if (data_size >= min && data_size <= max) {
      memcpy(&raw_description, data.data(),
             std::min(static_cast<unsigned int>(data.size()), max));
      description.description[DESCRIPTION_SIZE] = 0;
      description.pid = NetworkToHost(raw_description.pid);
      description.pdl_size = raw_description.pdl_size;
      description.data_type = raw_description.data_type;
      description.command_class = raw_description.command_class;
      description.unit = raw_description.unit;
      description.prefix = raw_description.prefix;
      description.min_value = HostToNetwork(raw_description.min_value);
      description.default_value = HostToNetwork(raw_description.default_value);
      description.max_value = HostToNetwork(raw_description.max_value);
      description.description = std::string(raw_description.description,
                                            DESCRIPTION_SIZE);
    } else {
      std::stringstream str;
      str << data_size << " needs to be between " << min << " and " << max;
      response_status.MalformedResponse(str.str());
    }
  }
  callback->Run(response_status, description);
}


/*
 * Handle a DEVICE_INFO Get command
 */
void RDMAPI::_HandleGetDeviceInfo(
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       const DeviceInfo&> *callback,
    const RDMAPIImplResponseStatus &status,
    const string &data) {
  ResponseStatus response_status(status, data);
  DeviceInfo device_info;

  if (response_status.ResponseType() == ResponseStatus::VALID_RESPONSE) {
    unsigned int data_size = data.size();
    if (data_size == sizeof(device_info)) {
      memcpy(&device_info, data.data(), sizeof(device_info));
      device_info.protocol_version =
        NetworkToHost(device_info.protocol_version);
      device_info.device_model = NetworkToHost(device_info.device_model);
      device_info.product_category =
        NetworkToHost(device_info.product_category);
      device_info.software_version =
        NetworkToHost(device_info.software_version);
      device_info.dmx_footprint =
        NetworkToHost(device_info.dmx_footprint);
      device_info.dmx_personality =
        NetworkToHost(device_info.dmx_personality);
      device_info.dmx_start_address =
        NetworkToHost(device_info.dmx_start_address);
      device_info.sub_device_count =
        NetworkToHost(device_info.sub_device_count);
    } else {
      SetIncorrectPDL(&response_status, data.size(), sizeof(device_info));
    }
  }
  callback->Run(response_status, device_info);
}


/*
 * Handle a PRODUCT_DETAIL_ID_LIST response
 */
void RDMAPI::_HandleGetProductDetailIdList(
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       vector<uint16_t> > *callback,
    const RDMAPIImplResponseStatus &status,
    const string &data) {
  static const unsigned int MAX_DETAIL_IDS = 6;
  ResponseStatus response_status(status, data);
  vector<uint16_t> product_detail_ids;

  if (response_status.ResponseType() == ResponseStatus::VALID_RESPONSE) {
    unsigned int data_size = data.size();
    if (data_size > MAX_DETAIL_IDS * sizeof(uint16_t)) {
      std::stringstream str;
      str << "PDL needs to be <= " << (MAX_DETAIL_IDS * sizeof(uint16_t)) <<
        ", was " << data_size;
      response_status.MalformedResponse(str.str());
    } else if (data_size % 2) {
      std::stringstream str;
      str << "PDL needs to be a multiple of 2, was " << data_size;
      response_status.MalformedResponse(str.str());
    } else {
      const uint16_t *start = reinterpret_cast<const uint16_t*>(data.data());
      const uint16_t *ptr = start;
      while (ptr < start + (data_size / sizeof(*ptr))) {
        product_detail_ids.push_back(*ptr);
        ptr++;
      }
    }
  }
  callback->Run(response_status, product_detail_ids);
}


/*
 * Handle a get FACTORY_DEFAULTS response
 */
void RDMAPI::_HandleGetFactoryDefaults(
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       bool> *callback,
    const RDMAPIImplResponseStatus &status,
    const string &data) {
  static const unsigned int DATA_SIZE = 1;
  ResponseStatus response_status(status, data);
  bool defaults_enabled = false;

  if (response_status.ResponseType() == ResponseStatus::VALID_RESPONSE) {
    if (data.size() == DATA_SIZE) {
      defaults_enabled = data.data()[0];
    } else {
      SetIncorrectPDL(&response_status, data.size(), DATA_SIZE);
    }
  }
  callback->Run(response_status, defaults_enabled);
}


/*
 * Handle a LANGUAGE_CAPABILITIES response
 */
void RDMAPI::_HandleGetLanguageCapabilities(
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       vector<string>&> *callback,
    const RDMAPIImplResponseStatus &status,
    const string &data) {
  ResponseStatus response_status(status, data);
  vector<string> languages;

  if (response_status.ResponseType() == ResponseStatus::VALID_RESPONSE) {
    unsigned int data_size = data.size();
    if (data_size % 2) {
      std::stringstream str;
      str << "PDL needs to be a multiple of 2, was " << data_size;
      response_status.MalformedResponse(str.str());
    } else {
      const char *ptr = data.data();
      const char *end = data.data() + data.size();
      while (ptr < end) {
        languages.push_back(string(ptr, 2));
        ptr+=2;
      }
    }
  }
  callback->Run(response_status, languages);
}


/*
 * Handle a LANGUAGE response
 */
void RDMAPI::_HandleGetLanguage(
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       const string&> *callback,
    const RDMAPIImplResponseStatus &status,
    const string &data) {
  ResponseStatus response_status(status, data);
  static const unsigned int DATA_SIZE = 2;
  if (response_status.ResponseType() == ResponseStatus::VALID_RESPONSE) {
    if (data.size() != DATA_SIZE) {
      SetIncorrectPDL(&response_status, data.size(), DATA_SIZE);
    }
  }
  callback->Run(response_status, data);
}


/*
 * Handle a BOOT_SOFTWARE_VERSION_ID response
 */
void RDMAPI::_HandleGetBootSoftwareVersion(
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       uint32_t> *callback,
    const RDMAPIImplResponseStatus &status,
    const string &data) {
  ResponseStatus response_status(status, data);
  static const unsigned int DATA_SIZE = 4;
  uint32_t boot_version = 0;
  if (response_status.ResponseType() == ResponseStatus::VALID_RESPONSE) {
    if (data.size() != DATA_SIZE) {
      SetIncorrectPDL(&response_status, data.size(), DATA_SIZE);
    } else {
      boot_version = *(reinterpret_cast<const uint32_t*>(data.data()));
      boot_version = NetworkToHost(boot_version);
    }
  }
  callback->Run(response_status, boot_version);
}


//-----------------------------------------------------------------------------
// Private methods follow

// Check that a UID is not a broadcast address
bool RDMAPI::CheckNotBroadcast(const UID &uid, string *error) {
  if (uid.IsBroadcast()) {
    if (error)
      *error = "Cannot send to broadcast address";
    return false;
  }
  return true;
}


// Check the subdevice value is valid
bool RDMAPI::CheckValidSubDevice(uint16_t sub_device,
                                 bool broadcast_allowed,
                                 string *error) {
  if (sub_device <= 0x0200)
    return true;

  if (broadcast_allowed && sub_device == 0xffff)
    return true;

  if (error) {
    *error = "Sub device must be <= 0x0200";
    if (broadcast_allowed)
      *error += " or 0xffff";
  }
  return false;
}


// Checks the status of a rdm command and sets error appropriately
bool RDMAPI::CheckReturnStatus(bool status, string *error) {
  if (!status && error)
    *error = "Unable to send RDM command";
  return status;
}

// Mark a ResponseStatus as malformed due to a length mismatch
void RDMAPI::SetIncorrectPDL(ResponseStatus *status,
                             unsigned int actual,
                             unsigned int expected) {
  status->MalformedResponse("PDL mismatch, " +
    IntToString(actual) + " != " +
    IntToString(expected) + " (expected)");
}
}  // rdm
}  // ola
