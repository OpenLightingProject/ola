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
#include <string.h>
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
    unsigned int universe,
    const UID &uid,
    SingleUseCallback3<void,
                       const ResponseStatus&,
                       uint16_t,
                       bool> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  if (CheckNotBroadcast(uid, error, callback))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleGetProxiedDeviceCount,
    callback);
  return CheckReturnStatus(
      m_impl->RDMGet(cb,
                     universe,
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
    unsigned int universe,
    const UID &uid,
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       const vector<UID>&> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  if (CheckNotBroadcast(uid, error, callback))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleGetProxiedDevices,
    callback);
  return CheckReturnStatus(
      m_impl->RDMGet(cb,
                     universe,
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
    unsigned int universe,
    const UID &uid,
    SingleUseCallback4<void,
                       const ResponseStatus&,
                       uint16_t,
                       uint16_t,
                       uint16_t> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  if (CheckNotBroadcast(uid, error, callback))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleGetCommStatus,
    callback);
  return CheckReturnStatus(
      m_impl->RDMGet(cb,
                     universe,
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
    unsigned int universe,
    const UID &uid,
    SingleUseCallback1<void, const ResponseStatus&> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleEmptyResponse,
    callback);
  return CheckReturnStatus(
      m_impl->RDMSet(cb,
                     universe,
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
    unsigned int universe,
    const UID &uid,
    rdm_status_type status_type,
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       const vector<StatusMessage>&> *callback,
  string *error) {
  if (CheckCallback(error, callback))
    return false;
  if (CheckNotBroadcast(uid, error, callback))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleGetStatusMessage,
    callback);
  uint8_t type = static_cast<uint8_t>(status_type);
  return CheckReturnStatus(
      m_impl->RDMGet(cb,
                     universe,
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
    unsigned int universe,
    const UID &uid,
    uint16_t status_id,
    SingleUseCallback2<void, const ResponseStatus&, const string&> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  if (CheckNotBroadcast(uid, error, callback))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleLabelResponse,
    callback);
  status_id = HostToNetwork(status_id);
  return CheckReturnStatus(
      m_impl->RDMGet(cb,
                     universe,
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
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    SingleUseCallback1<void, const ResponseStatus&> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  if (CheckValidSubDevice(sub_device, true, error, callback))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleEmptyResponse,
    callback);
  return CheckReturnStatus(
      m_impl->RDMSet(cb,
                     universe,
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
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       uint8_t> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  if (CheckNotBroadcast(uid, error, callback))
    return false;
  if (CheckValidSubDevice(sub_device, false, error, callback))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleGetSubDeviceReporting,
    callback);
  return CheckReturnStatus(
      m_impl->RDMGet(cb,
                     universe,
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
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    rdm_status_type status_type,
    SingleUseCallback1<void, const ResponseStatus&> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  if (CheckNotBroadcast(uid, error, callback))
    return false;
  if (CheckValidSubDevice(sub_device, true, error, callback))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleEmptyResponse,
    callback);
  uint8_t type = static_cast<uint8_t>(status_type);
  return CheckReturnStatus(
      m_impl->RDMSet(cb,
                     universe,
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
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       const vector<uint16_t>&> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  if (CheckNotBroadcast(uid, error, callback))
    return false;
  if (CheckValidSubDevice(sub_device, false, error, callback))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleGetSupportedParameters,
    callback);
  return m_impl->RDMGet(cb,
                        universe,
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
    unsigned int universe,
    const UID &uid,
    uint16_t pid,
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       const ParameterDescriptor&> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  if (CheckNotBroadcast(uid, error, callback))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleGetParameterDescriptor,
    callback);
  pid = HostToNetwork(pid);
  return CheckReturnStatus(
      m_impl->RDMGet(cb,
                     universe,
                     uid,
                     ROOT_RDM_DEVICE,
                     PID_PARAMETER_DESCRIPTION,
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
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       const DeviceDescriptor&> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  if (CheckNotBroadcast(uid, error, callback))
    return false;
  if (CheckValidSubDevice(sub_device, false, error, callback))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleGetDeviceDescriptor,
    callback);
  return CheckReturnStatus(
    m_impl->RDMGet(cb,
                   universe,
                   uid,
                   sub_device,
                   PID_DEVICE_INFO),
    error);
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
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       const vector<uint16_t>&> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  if (CheckNotBroadcast(uid, error, callback))
    return false;
  if (CheckValidSubDevice(sub_device, false, error, callback))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleGetProductDetailIdList,
    callback);
  return CheckReturnStatus(
    m_impl->RDMGet(cb,
                   universe,
                   uid,
                   sub_device,
                   PID_PRODUCT_DETAIL_ID_LIST),
    error);
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
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       const string&> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  if (CheckNotBroadcast(uid, error, callback))
    return false;
  if (CheckValidSubDevice(sub_device, false, error, callback))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleLabelResponse,
    callback);
  return CheckReturnStatus(
    m_impl->RDMGet(cb,
                   universe,
                   uid,
                   sub_device,
                   PID_DEVICE_MODEL_DESCRIPTION),
    error);
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
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       const string&> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  if (CheckNotBroadcast(uid, error, callback))
    return false;
  if (CheckValidSubDevice(sub_device, false, error, callback))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleLabelResponse,
    callback);
  return CheckReturnStatus(
    m_impl->RDMGet(cb,
                   universe,
                   uid,
                   sub_device,
                   PID_MANUFACTURER_LABEL),
    error);
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
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       const string&> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  if (CheckNotBroadcast(uid, error, callback))
    return false;
  if (CheckValidSubDevice(sub_device, false, error, callback))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleLabelResponse,
    callback);
  return CheckReturnStatus(
    m_impl->RDMGet(cb,
                   universe,
                   uid,
                   sub_device,
                   PID_DEVICE_LABEL),
    error);
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
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    const string &label,
    SingleUseCallback1<void, const ResponseStatus&> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  // It doesn't really make sense to broadcast this but allow it anyway
  if (CheckValidSubDevice(sub_device, true, error, callback))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleEmptyResponse,
    callback);
  return CheckReturnStatus(
    m_impl->RDMSet(cb,
                   universe,
                   uid,
                   sub_device,
                   PID_DEVICE_LABEL,
                   reinterpret_cast<const uint8_t*>(label.data()),
                   label.size()),
    error);
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
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       bool> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  if (CheckNotBroadcast(uid, error, callback))
    return false;
  if (CheckValidSubDevice(sub_device, false, error, callback))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleBoolResponse,
    callback);
  return CheckReturnStatus(
    m_impl->RDMGet(cb,
                   universe,
                   uid,
                   sub_device,
                   PID_FACTORY_DEFAULTS),
    error);
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
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    SingleUseCallback1<void, const ResponseStatus&> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  if (CheckNotBroadcast(uid, error, callback))
    return false;
  if (CheckValidSubDevice(sub_device, true, error, callback))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleEmptyResponse,
    callback);
  return CheckReturnStatus(
    m_impl->RDMSet(cb,
                   universe,
                   uid,
                   sub_device,
                   PID_FACTORY_DEFAULTS),
    error);
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
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       const vector<string>&> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  if (CheckNotBroadcast(uid, error, callback))
    return false;
  if (CheckValidSubDevice(sub_device, false, error, callback))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleGetLanguageCapabilities,
    callback);
  return CheckReturnStatus(
    m_impl->RDMGet(cb,
                   universe,
                   uid,
                   sub_device,
                   PID_LANGUAGE_CAPABILITIES),
    error);
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
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       const string&> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  if (CheckNotBroadcast(uid, error, callback))
    return false;
  if (CheckValidSubDevice(sub_device, false, error, callback))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleGetLanguage,
    callback);
  return CheckReturnStatus(
    m_impl->RDMGet(cb,
                   universe,
                   uid,
                   sub_device,
                   PID_LANGUAGE),
    error);
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
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    const string &language,
    SingleUseCallback1<void, const ResponseStatus&> *callback,
    string *error) {
  static const unsigned int DATA_SIZE = 2;
  if (CheckCallback(error, callback))
    return false;
  if (CheckValidSubDevice(sub_device, true, error, callback))
    return false;

  if (language.size() != DATA_SIZE) {
    if (error)
      *error = "Language must be a two letter code";
    delete callback;
    return false;
  }

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleEmptyResponse,
    callback);
  return CheckReturnStatus(
    m_impl->RDMSet(cb,
                   universe,
                   uid,
                   sub_device,
                   PID_LANGUAGE,
                   reinterpret_cast<const uint8_t*>(language.data()),
                   DATA_SIZE),
    error);
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
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       const string&> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  if (CheckNotBroadcast(uid, error, callback))
    return false;
  if (CheckValidSubDevice(sub_device, false, error, callback))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleLabelResponse,
    callback);
  return CheckReturnStatus(
    m_impl->RDMGet(cb,
                   universe,
                   uid,
                   sub_device,
                   PID_SOFTWARE_VERSION_LABEL),
    error);
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
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       uint32_t> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  if (CheckNotBroadcast(uid, error, callback))
    return false;
  if (CheckValidSubDevice(sub_device, false, error, callback))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleGetBootSoftwareVersion,
    callback);
  return CheckReturnStatus(
    m_impl->RDMGet(cb,
                   universe,
                   uid,
                   sub_device,
                   PID_BOOT_SOFTWARE_VERSION_ID),
    error);
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
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       const string&> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  if (CheckNotBroadcast(uid, error, callback))
    return false;
  if (CheckValidSubDevice(sub_device, false, error, callback))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleLabelResponse,
    callback);
  return CheckReturnStatus(
    m_impl->RDMGet(cb,
                   universe,
                   uid,
                   sub_device,
                   PID_BOOT_SOFTWARE_VERSION_LABEL),
    error);
}


/*
 * Get the current DMX personality
 * @param uid the UID to fetch the outstanding message count for
 * @param sub_device the sub device to use
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
 */
bool RDMAPI::GetDMXPersonality(
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    SingleUseCallback3<void,
                       const ResponseStatus&,
                       uint8_t,
                       uint8_t> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  if (CheckNotBroadcast(uid, error, callback))
    return false;
  if (CheckValidSubDevice(sub_device, false, error, callback))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleGetDMXPersonality,
    callback);
  return CheckReturnStatus(
    m_impl->RDMGet(cb,
                   universe,
                   uid,
                   sub_device,
                   PID_DMX_PERSONALITY),
    error);
}


/*
 * Set the DMX personality
 * @param uid the UID to fetch the outstanding message count for
 * @param sub_device the sub device to use
 * @param personality the value of the personality to choose
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
 */
bool RDMAPI::SetDMXPersonality(
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    uint8_t personality,
    SingleUseCallback1<void, const ResponseStatus&> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  if (CheckNotBroadcast(uid, error, callback))
    return false;
  if (CheckValidSubDevice(sub_device, true, error, callback))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleEmptyResponse,
    callback);
  return CheckReturnStatus(
    m_impl->RDMSet(cb,
                   universe,
                   uid,
                   sub_device,
                   PID_DMX_PERSONALITY,
                   &personality,
                   sizeof(personality)),
    error);
}


/*
 * Get the description for a DMX personality
 * @param uid the UID to fetch the outstanding message count for
 * @param sub_device the sub device to use
 * @param personality the value of the personality to get the description of
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
 */
bool RDMAPI::GetDMXPersonalityDescription(
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    uint8_t personality,
    SingleUseCallback4<void,
                       const ResponseStatus&,
                       uint8_t,
                       uint16_t,
                       const string&> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  if (CheckNotBroadcast(uid, error, callback))
    return false;
  if (CheckValidSubDevice(sub_device, false, error, callback))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleGetDMXPersonalityDescription,
    callback);
  return CheckReturnStatus(
    m_impl->RDMGet(cb,
                   universe,
                   uid,
                   sub_device,
                   PID_DMX_PERSONALITY_DESCRIPTION,
                   &personality,
                   sizeof(personality)),
    error);
}


/*
 * Get the DMX start address
 * @param uid the UID to fetch the outstanding message count for
 * @param sub_device the sub device to use
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
 */
bool RDMAPI::GetDMXAddress(
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       uint16_t> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  if (CheckNotBroadcast(uid, error, callback))
    return false;
  if (CheckValidSubDevice(sub_device, false, error, callback))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleGetDMXAddress,
    callback);
  return CheckReturnStatus(
    m_impl->RDMGet(cb,
                   universe,
                   uid,
                   sub_device,
                   PID_DMX_START_ADDRESS),
    error);
}


/*
 * Set the DMX start address
 * @param uid the UID to fetch the outstanding message count for
 * @param sub_device the sub device to use
 * @param start_address the new start address
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
 */
bool RDMAPI::SetDMXAddress(
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    uint16_t start_address,
    SingleUseCallback1<void, const ResponseStatus&> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  if (CheckValidSubDevice(sub_device, true, error, callback))
    return false;

  start_address = HostToNetwork(start_address);
  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleEmptyResponse,
    callback);
  return CheckReturnStatus(
    m_impl->RDMSet(cb,
                   universe,
                   uid,
                   sub_device,
                   PID_DMX_START_ADDRESS,
                   reinterpret_cast<const uint8_t*>(&start_address),
                   sizeof(start_address)),
    error);
}


/*
 * Fetch the DMX slot info
 * @param uid the UID to fetch the outstanding message count for
 * @param sub_device the sub device to use
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
 */
bool RDMAPI::GetSlotInfo(
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       const vector<SlotDescriptor>&> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  if (CheckNotBroadcast(uid, error, callback))
    return false;
  if (CheckValidSubDevice(sub_device, false, error, callback))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleGetSlotInfo,
    callback);
  return CheckReturnStatus(
    m_impl->RDMGet(cb,
                   universe,
                   uid,
                   sub_device,
                   PID_SLOT_INFO),
    error);
}


/*
 * Fetch a DMX slot description
 * @param uid the UID to fetch the outstanding message count for
 * @param sub_device the sub device to use
 * @param slot_offset the offset of the slot to get the description of
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
 */
bool RDMAPI::GetSlotDescription(
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    uint16_t slot_offset,
    SingleUseCallback3<void,
                       const ResponseStatus&,
                       uint16_t,
                       const string&> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  if (CheckNotBroadcast(uid, error, callback))
    return false;
  if (CheckValidSubDevice(sub_device, false, error, callback))
    return false;

  slot_offset = HostToNetwork(slot_offset);
  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleGetSlotDescription,
    callback);
  return CheckReturnStatus(
    m_impl->RDMGet(cb,
                   universe,
                   uid,
                   sub_device,
                   PID_SLOT_DESCRIPTION,
                   reinterpret_cast<const uint8_t*>(&slot_offset),
                   sizeof(slot_offset)),
    error);
}


/*
 * Get the default value for a slot
 * @param uid the UID to fetch the outstanding message count for
 * @param sub_device the sub device to use
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
 */
bool RDMAPI::GetSlotDefaultValues(
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       const vector<SlotDefault>&> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  if (CheckNotBroadcast(uid, error, callback))
    return false;
  if (CheckValidSubDevice(sub_device, false, error, callback))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleGetSlotDefaultValues,
    callback);
  return CheckReturnStatus(
    m_impl->RDMGet(cb,
                   universe,
                   uid,
                   sub_device,
                   PID_DEFAULT_SLOT_VALUE),
    error);
}


/*
 * Get the definition for a sensor
 * @param uid the UID to fetch the outstanding message count for
 * @param sub_device the sub device to use
 * @param sensor_number the sensor index to get the descriptor for
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
 */
bool RDMAPI::GetSensorDefinition(
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    uint8_t sensor_number,
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       const SensorDescriptor&> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  if (CheckNotBroadcast(uid, error, callback))
    return false;
  if (CheckValidSubDevice(sub_device, false, error, callback))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleGetSensorDefinition,
    callback);
  return CheckReturnStatus(
    m_impl->RDMGet(cb,
                   universe,
                   uid,
                   sub_device,
                   PID_SENSOR_DEFINITION,
                   &sensor_number,
                   sizeof(sensor_number)),
    error);
}


/*
 * Get the value of a sensor
 * @param uid the UID to fetch the outstanding message count for
 * @param sub_device the sub device to use
 * @param sensor_number the sensor index to get the value of
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
 */
bool RDMAPI::GetSensorValue(
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    uint8_t sensor_number,
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       const SensorValueDescriptor&> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  if (CheckNotBroadcast(uid, error, callback))
    return false;
  if (CheckValidSubDevice(sub_device, false, error, callback))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleSensorValue,
    callback);
  return CheckReturnStatus(
    m_impl->RDMGet(cb,
                   universe,
                   uid,
                   sub_device,
                   PID_SENSOR_VALUE,
                   &sensor_number,
                   sizeof(sensor_number)),
    error);
}


/*
 * Reset a sensor
 * @param uid the UID to fetch the outstanding message count for
 * @param sub_device the sub device to use
 * @param sensor_number the sensor index to reset
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
 */
bool RDMAPI::SetSensorValue(
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    uint8_t sensor_number,
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       const SensorValueDescriptor&> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  if (CheckValidSubDevice(sub_device, true, error, callback))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleSensorValue,
    callback);
  return CheckReturnStatus(
    m_impl->RDMSet(cb,
                   universe,
                   uid,
                   sub_device,
                   PID_SENSOR_VALUE,
                   &sensor_number,
                   sizeof(sensor_number)),
    error);
}


/*
 * Put a sensor into record mode
 * @param uid the UID to fetch the outstanding message count for
 * @param sub_device the sub device to use
 * @param sensor_number the sensor index to record
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
 */
bool RDMAPI::RecordSensors(
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    uint8_t sensor_number,
    SingleUseCallback1<void, const ResponseStatus&> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  if (CheckValidSubDevice(sub_device, true, error, callback))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleEmptyResponse,
    callback);
  return CheckReturnStatus(
    m_impl->RDMSet(cb,
                   universe,
                   uid,
                   sub_device,
                   PID_RECORD_SENSORS,
                   &sensor_number,
                   sizeof(sensor_number)),
    error);
}


/*
 * Get the device hours
 * @param uid the UID to fetch the outstanding message count for
 * @param sub_device the sub device to use
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
*/
bool RDMAPI::GetDeviceHours(
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    SingleUseCallback2<void, const ResponseStatus&, uint32_t> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  return GenericGetU32(
      universe,
      uid,
      sub_device,
      callback,
      PID_DEVICE_HOURS,
      error);
}


/*
 * Set the device hours
 * @param uid the UID to fetch the outstanding message count for
 * @param sub_device the sub device to use
 * @param device_hours the number of device hours
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
*/
bool RDMAPI::SetDeviceHours(
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    uint32_t device_hours,
    SingleUseCallback1<void, const ResponseStatus&> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  return GenericSetU32(
      universe,
      uid,
      sub_device,
      device_hours,
      callback,
      PID_DEVICE_HOURS,
      error);
}


/*
* Get the lamp hours
* @param uid the UID to fetch the outstanding message count for
* @param sub_device the sub device to use
* @param callback the callback to invoke when this request completes
* @param error a pointer to a string which it set if an error occurs
* @return true if the request is sent correctly, false otherwise
*/
bool RDMAPI::GetLampHours(
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    SingleUseCallback2<void, const ResponseStatus&, uint32_t> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  return GenericGetU32(
      universe,
      uid,
      sub_device,
      callback,
      PID_LAMP_HOURS,
      error);
}


/*
 * Set the lamp hours
 * @param uid the UID to fetch the outstanding message count for
 * @param sub_device the sub device to use
 * @param lamp_hours the number of lamp hours
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
*/
bool RDMAPI::SetLampHours(
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    uint32_t lamp_hours,
    SingleUseCallback1<void, const ResponseStatus&> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  return GenericSetU32(
      universe,
      uid,
      sub_device,
      lamp_hours,
      callback,
      PID_LAMP_STRIKES,
      error);
}


/*
* Get the number of lamp strikes
* @param uid the UID to fetch the outstanding message count for
* @param sub_device the sub device to use
* @param callback the callback to invoke when this request completes
* @param error a pointer to a string which it set if an error occurs
* @return true if the request is sent correctly, false otherwise
*/
bool RDMAPI::GetLampStrikes(
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    SingleUseCallback2<void, const ResponseStatus&, uint32_t> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  return GenericGetU32(
      universe,
      uid,
      sub_device,
      callback,
      PID_LAMP_STRIKES,
      error);
}


/*
 * Set the lamp strikes
 * @param uid the UID to fetch the outstanding message count for
 * @param sub_device the sub device to use
 * @param lamp_strikes the number of lamp strikes
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
*/
bool RDMAPI::SetLampStrikes(
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    uint32_t lamp_strikes,
    SingleUseCallback1<void, const ResponseStatus&> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  return GenericSetU32(
      universe,
      uid,
      sub_device,
      lamp_strikes,
      callback,
      PID_LAMP_STRIKES,
      error);
}


/*
* Get the state of the lamp
* @param uid the UID to fetch the outstanding message count for
* @param sub_device the sub device to use
* @param callback the callback to invoke when this request completes
* @param error a pointer to a string which it set if an error occurs
* @return true if the request is sent correctly, false otherwise
*/
bool RDMAPI::GetLampState(
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    SingleUseCallback2<void, const ResponseStatus&, uint8_t> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  return GenericGetU8(
      universe,
      uid,
      sub_device,
      callback,
      PID_LAMP_STATE,
      error);
}


/*
 * Set the lamp state
 * @param uid the UID to fetch the outstanding message count for
 * @param sub_device the sub device to use
 * @param lamp_state the new lamp state
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
*/
bool RDMAPI::SetLampState(
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    uint8_t lamp_state,
    SingleUseCallback1<void, const ResponseStatus&> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  return GenericSetU8(
      universe,
      uid,
      sub_device,
      lamp_state,
      callback,
      PID_LAMP_STATE,
      error);
}


/*
* Get the mode of the lamp
* @param uid the UID to fetch the outstanding message count for
* @param sub_device the sub device to use
* @param callback the callback to invoke when this request completes
* @param error a pointer to a string which it set if an error occurs
* @return true if the request is sent correctly, false otherwise
*/
bool RDMAPI::GetLampMode(
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    SingleUseCallback2<void, const ResponseStatus&, uint8_t> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  return GenericGetU8(
      universe,
      uid,
      sub_device,
      callback,
      PID_LAMP_ON_MODE,
      error);
}


/*
 * Set the lamp mode
 * @param uid the UID to fetch the outstanding message count for
 * @param sub_device the sub device to use
 * @param lamp_mode the new lamp mode
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
*/
bool RDMAPI::SetLampMode(
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    uint8_t lamp_mode,
    SingleUseCallback1<void, const ResponseStatus&> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  return GenericSetU8(
      universe,
      uid,
      sub_device,
      lamp_mode,
      callback,
      PID_LAMP_ON_MODE,
      error);
}


/*
* Get the number of device power cycles
* @param uid the UID to fetch the outstanding message count for
* @param sub_device the sub device to use
* @param callback the callback to invoke when this request completes
* @param error a pointer to a string which it set if an error occurs
* @return true if the request is sent correctly, false otherwise
*/
bool RDMAPI::GetDevicePowerCycles(
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    SingleUseCallback2<void, const ResponseStatus&, uint32_t> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  return GenericGetU32(
      universe,
      uid,
      sub_device,
      callback,
      PID_DEVICE_POWER_CYCLES,
      error);
}


/*
 * Set the number of power cycles
 * @param uid the UID to fetch the outstanding message count for
 * @param sub_device the sub device to use
 * @param power_cycles the number of power cycles
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
*/
bool RDMAPI::SetDevicePowerCycles(
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    uint32_t power_cycles,
    SingleUseCallback1<void, const ResponseStatus&> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  return GenericSetU32(
      universe,
      uid,
      sub_device,
      power_cycles,
      callback,
      PID_DEVICE_POWER_CYCLES,
      error);
}


/*
* Get the display invert setting
* @param uid the UID to fetch the outstanding message count for
* @param sub_device the sub device to use
* @param callback the callback to invoke when this request completes
* @param error a pointer to a string which it set if an error occurs
* @return true if the request is sent correctly, false otherwise
*/
bool RDMAPI::GetDisplayInvert(
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    SingleUseCallback2<void, const ResponseStatus&, uint8_t> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  return GenericGetU8(
      universe,
      uid,
      sub_device,
      callback,
      PID_DISPLAY_INVERT,
      error);
}


/*
 * Set the display invert setting
 * @param uid the UID to fetch the outstanding message count for
 * @param sub_device the sub device to use
 * @param display_invert the new invert setting
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
*/
bool RDMAPI::SetDisplayInvert(
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    uint8_t display_invert,
    SingleUseCallback1<void, const ResponseStatus&> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  return GenericSetU8(
      universe,
      uid,
      sub_device,
      display_invert,
      callback,
      PID_DISPLAY_INVERT,
      error);
}


/*
* Get the display level
* @param uid the UID to fetch the outstanding message count for
* @param sub_device the sub device to use
* @param callback the callback to invoke when this request completes
* @param error a pointer to a string which it set if an error occurs
* @return true if the request is sent correctly, false otherwise
*/
bool RDMAPI::GetDisplayLevel(
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    SingleUseCallback2<void, const ResponseStatus&, uint8_t> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  return GenericGetU8(
      universe,
      uid,
      sub_device,
      callback,
      PID_DISPLAY_LEVEL,
      error);
}


/*
 * Set the display level
 * @param uid the UID to fetch the outstanding message count for
 * @param sub_device the sub device to use
 * @param display_level the new setting
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
*/
bool RDMAPI::SetDisplayLevel(
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    uint8_t display_level,
    SingleUseCallback1<void, const ResponseStatus&> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  return GenericSetU8(
      universe,
      uid,
      sub_device,
      display_level,
      callback,
      PID_DISPLAY_LEVEL,
      error);
}


/*
* Get the pan invert parameter
* @param uid the UID to fetch the outstanding message count for
* @param sub_device the sub device to use
* @param callback the callback to invoke when this request completes
* @param error a pointer to a string which it set if an error occurs
* @return true if the request is sent correctly, false otherwise
*/
bool RDMAPI::GetPanInvert(
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    SingleUseCallback2<void, const ResponseStatus&, uint8_t> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  return GenericGetU8(
      universe,
      uid,
      sub_device,
      callback,
      PID_PAN_INVERT,
      error);
}


/*
 * Invert the pan parameter
 * @param uid the UID to fetch the outstanding message count for
 * @param sub_device the sub device to use
 * @param invert set to true to invert
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
*/
bool RDMAPI::SetPanInvert(
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    uint8_t invert,
    SingleUseCallback1<void, const ResponseStatus&> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  return GenericSetU8(
      universe,
      uid,
      sub_device,
      invert,
      callback,
      PID_PAN_INVERT,
      error);
}


/*
* Get the tilt invert parameter
* @param uid the UID to fetch the outstanding message count for
* @param sub_device the sub device to use
* @param callback the callback to invoke when this request completes
* @param error a pointer to a string which it set if an error occurs
* @return true if the request is sent correctly, false otherwise
*/
bool RDMAPI::GetTiltInvert(
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    SingleUseCallback2<void, const ResponseStatus&, uint8_t> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  return GenericGetU8(
      universe,
      uid,
      sub_device,
      callback,
      PID_TILT_INVERT,
      error);
}


/*
 * Invert the tilt parameter
 * @param uid the UID to fetch the outstanding message count for
 * @param sub_device the sub device to use
 * @param invert set to true to invert
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
*/
bool RDMAPI::SetTiltInvert(
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    uint8_t invert,
    SingleUseCallback1<void, const ResponseStatus&> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  return GenericSetU8(
      universe,
      uid,
      sub_device,
      invert,
      callback,
      PID_TILT_INVERT,
      error);
}


/*
* Get the pan/tilt swap parameter
* @param uid the UID to fetch the outstanding message count for
* @param sub_device the sub device to use
* @param callback the callback to invoke when this request completes
* @param error a pointer to a string which it set if an error occurs
* @return true if the request is sent correctly, false otherwise
*/
bool RDMAPI::GetPanTiltSwap(
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    SingleUseCallback2<void, const ResponseStatus&, uint8_t> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  return GenericGetU8(
      universe,
      uid,
      sub_device,
      callback,
      PID_PAN_TILT_SWAP,
      error);
}


/*
 * Swap the pan and tilt actions
 * @param uid the UID to fetch the outstanding message count for
 * @param sub_device the sub device to use
 * @param swap, true to swap, false otherwise
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
*/
bool RDMAPI::SetPanTiltSwap(
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    uint8_t swap,
    SingleUseCallback1<void, const ResponseStatus&> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  return GenericSetU8(
      universe,
      uid,
      sub_device,
      swap,
      callback,
      PID_PAN_TILT_SWAP,
      error);
}


/*
 * Get the clock value
 * @param uid the UID to fetch the outstanding message count for
 * @param sub_device the sub device to use
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
*/
bool RDMAPI::GetClock(
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       const ClockValue&> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleClock,
    callback);
  return CheckReturnStatus(
      m_impl->RDMGet(cb,
                     universe,
                     uid,
                     sub_device,
                     PID_REAL_TIME_CLOCK),
      error);
}


/*
 * Set the clock value
 * @param uid the UID to fetch the outstanding message count for
 * @param sub_device the sub device to use
 * @param clock, the new clock settings
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
*/
bool RDMAPI::SetClock(
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    const ClockValue &clock,
    SingleUseCallback1<void, const ResponseStatus&> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  ClockValue clock_data;
  memcpy(&clock_data, &clock, sizeof(clock_data));
  clock_data.year = HostToNetwork(clock_data.year);

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleEmptyResponse,
    callback);
  return CheckReturnStatus(
      m_impl->RDMSet(cb,
                     universe,
                     uid,
                     sub_device,
                     PID_REAL_TIME_CLOCK,
                     reinterpret_cast<const uint8_t*>(&clock_data),
                     sizeof(struct clock_value_s)),
      error);
}


/*
 * Check the identify mode for a device
 * @param uid the UID to fetch the outstanding message count for
 * @param sub_device the sub device to use
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
 */
bool RDMAPI::GetIdentifyMode(
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    SingleUseCallback2<void, const ResponseStatus&, bool> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  if (CheckNotBroadcast(uid, error, callback))
    return false;
  if (CheckValidSubDevice(sub_device, false, error, callback))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleBoolResponse,
    callback);
  return CheckReturnStatus(
    m_impl->RDMGet(cb,
                   universe,
                   uid,
                   sub_device,
                   PID_IDENTIFY_DEVICE),
    error);
}


/*
 * Change the identify mode for a device
 * @param uid the UID to fetch the outstanding message count for
 * @param sub_device the sub device to use
 * @param mode the identify mode to set
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
*/
bool RDMAPI::IdentifyDevice(
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    bool mode,
    SingleUseCallback1<void, const ResponseStatus&> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  if (CheckValidSubDevice(sub_device, true, error, callback))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleEmptyResponse,
    callback);
  uint8_t option = mode;
  return CheckReturnStatus(
    m_impl->RDMSet(cb,
                   universe,
                   uid,
                   sub_device,
                   PID_IDENTIFY_DEVICE,
                   &option,
                   sizeof(option)),
    error);
}


/*
 * Reset a device
 * @param uid the UID to fetch the outstanding message count for
 * @param sub_device the sub device to use
 * @param warm_reset true for a warm reset, false for a cold reset
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
*/
bool RDMAPI::ResetDevice(
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    bool warm_reset,
    SingleUseCallback1<void, const ResponseStatus&> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  if (CheckValidSubDevice(sub_device, true, error, callback))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleEmptyResponse,
    callback);
  uint8_t option = warm_reset ? 0x01 : 0xff;
  return CheckReturnStatus(
    m_impl->RDMSet(cb,
                   universe,
                   uid,
                   sub_device,
                   PID_IDENTIFY_DEVICE,
                   &option,
                   sizeof(option)),
    error);
}


/*
 * Get the power state for a device
 * @param uid the UID to fetch the outstanding message count for
 * @param sub_device the sub device to use
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
*/
bool RDMAPI::GetPowerState(
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    SingleUseCallback2<void, const ResponseStatus&, uint8_t> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  if (CheckNotBroadcast(uid, error, callback))
    return false;
  if (CheckValidSubDevice(sub_device, false, error, callback))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleU8Response,
    callback);
  return CheckReturnStatus(
    m_impl->RDMGet(cb,
                   universe,
                   uid,
                   sub_device,
                   PID_POWER_STATE),
    error);
}


/*
 * Set the power state for a device.
 * @param uid the UID to fetch the outstanding message count for
 * @param sub_device the sub device to use
 * @param power_state the new power state
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
*/
bool RDMAPI::SetPowerState(
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    rdm_power_state power_state,
    SingleUseCallback1<void, const ResponseStatus&> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  if (CheckValidSubDevice(sub_device, true, error, callback))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleEmptyResponse,
    callback);
  uint8_t option = static_cast<uint8_t>(power_state);
  return CheckReturnStatus(
    m_impl->RDMSet(cb,
                   universe,
                   uid,
                   sub_device,
                   PID_POWER_STATE,
                   &option,
                   sizeof(option)),
    error);
}


/*
 * Check if a device is in self test mode.
 * @param uid the UID to fetch the outstanding message count for
 * @param sub_device the sub device to use
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
*/
bool RDMAPI::SelfTestEnabled(
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    SingleUseCallback2<void, const ResponseStatus&, bool> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  if (CheckNotBroadcast(uid, error, callback))
    return false;
  if (CheckValidSubDevice(sub_device, false, error, callback))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleBoolResponse,
    callback);
  return CheckReturnStatus(
    m_impl->RDMGet(cb,
                   universe,
                   uid,
                   sub_device,
                   PID_PERFORM_SELFTEST),
    error);
}


/*
 * Perform a self test on a device.
 * @param uid the UID to fetch the outstanding message count for
 * @param sub_device the sub device to use
 * @param self_test_number the number of the self test to perform.
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
*/
bool RDMAPI::PerformSelfTest(
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    uint8_t self_test_number,
    SingleUseCallback1<void, const ResponseStatus&> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  if (CheckValidSubDevice(sub_device, true, error, callback))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleEmptyResponse,
    callback);
  return CheckReturnStatus(
    m_impl->RDMSet(cb,
                   universe,
                   uid,
                   sub_device,
                   PID_PERFORM_SELFTEST,
                   &self_test_number,
                   sizeof(self_test_number)),
    error);
}


/*
 * Fetch the description of a self test.
 * @param uid the UID to fetch the outstanding message count for
 * @param sub_device the sub device to use
 * @param self_test_number the number of the self test to fetch the description
 *   of.
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
*/
bool RDMAPI::SelfTestDescription(
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    uint8_t self_test_number,
    SingleUseCallback3<void, const ResponseStatus&, uint8_t,
                       const string&> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  if (CheckNotBroadcast(uid, error, callback))
    return false;
  if (CheckValidSubDevice(sub_device, false, error, callback))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleSelfTestDescription,
    callback);
  return CheckReturnStatus(
    m_impl->RDMGet(cb,
                   universe,
                   uid,
                   sub_device,
                   PID_SELF_TEST_DESCRIPTION,
                   &self_test_number,
                   sizeof(self_test_number)),
    error);
}


/*
 * Capture the current state into a preset.
 * @param uid the UID to fetch the outstanding message count for
 * @param sub_device the sub device to use
 * @param scene the number of the preset scene to store
 * @param fade_up_time the time in 10s of a second to fade up this scene.
 * @param fade_down_time the time in 10s of a second to fade down the previous
 *   scene.
 * @param wait_time the time in 10s of a second to hold this scene when the
 *   playback mode is PRESET_PLAYBACK_ALL
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
*/
bool RDMAPI::CapturePreset(
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    uint16_t scene,
    uint16_t fade_up_time,
    uint16_t fade_down_time,
    uint16_t wait_time,
    SingleUseCallback1<void, const ResponseStatus&> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  if (CheckValidSubDevice(sub_device, true, error, callback))
    return false;

  struct preset_config {
    uint16_t scene;
    uint16_t fade_up_time;
    uint16_t fade_down_time;
    uint16_t wait_time;
  } __attribute__((packed));
  struct preset_config raw_config;

  raw_config.scene = HostToNetwork(scene);
  raw_config.fade_up_time = HostToNetwork(fade_up_time);
  raw_config.fade_down_time = HostToNetwork(fade_down_time);
  raw_config.wait_time = HostToNetwork(wait_time);

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleEmptyResponse,
    callback);
  return CheckReturnStatus(
    m_impl->RDMSet(cb,
                   universe,
                   uid,
                   sub_device,
                   PID_CAPTURE_PRESET,
                   reinterpret_cast<const uint8_t*>(&raw_config),
                   sizeof(raw_config)),
    error);
}


/*
 * Fetch the current playback mode.
 * @param uid the UID to fetch the outstanding message count for
 * @param sub_device the sub device to use
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
*/
bool RDMAPI::PresetPlaybackMode(
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    SingleUseCallback3<void,
                       const ResponseStatus&,
                       uint16_t,
                       uint8_t> *callback,
    string *error) {
  if (CheckCallback(error, callback))
    return false;
  if (CheckNotBroadcast(uid, error, callback))
    return false;
  if (CheckValidSubDevice(sub_device, false, error, callback))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandlePlaybackMode,
    callback);
  return CheckReturnStatus(
    m_impl->RDMGet(cb,
                   universe,
                   uid,
                   sub_device,
                   PID_PRESET_PLAYBACK,
                   NULL,
                   0),
    error);
}


/*
 * Set the current playback mode.
 * @param uid the UID to fetch the outstanding message count for
 * @param sub_device the sub device to use
 * @param playback_mode the playback scene to use, PRESET_PLAYBACK_OFF or
 *   PRESET_PLAYBACK_ALL.
 * @param level the level to use for the scene.
 * @param callback the callback to invoke when this request completes
 * @param error a pointer to a string which it set if an error occurs
 * @return true if the request is sent correctly, false otherwise
*/
bool RDMAPI::SetPresetPlaybackMode(
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    uint16_t playback_mode,
    uint8_t level,
    SingleUseCallback1<void, const ResponseStatus&> *callback,
    string *error) {

  if (CheckCallback(error, callback))
    return false;
  if (CheckValidSubDevice(sub_device, true, error, callback))
    return false;

  struct preset_config {
    uint16_t mode;
    uint8_t level;
  } __attribute__((packed));
  struct preset_config raw_config;

  raw_config.mode = HostToNetwork(playback_mode);
  raw_config.level = level;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleEmptyResponse,
    callback);
  return CheckReturnStatus(
    m_impl->RDMSet(cb,
                   universe,
                   uid,
                   sub_device,
                   PID_PRESET_PLAYBACK,
                   reinterpret_cast<const uint8_t*>(&raw_config),
                   sizeof(raw_config)),
    error);
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
  ResponseStatus response_status(status, data);
  if (response_status.ResponseType() == ResponseStatus::VALID_RESPONSE &&
      data.size() > LABEL_SIZE) {
    std::stringstream str;
    str << "PDL needs to be <= " << LABEL_SIZE << ", was " << data.size();
    response_status.MalformedResponse(str.str());
  }

  string label = data;
  ShortenString(&label);
  callback->Run(response_status, label);
}


/*
 * Handle a response that contains a bool
 */
void RDMAPI::_HandleBoolResponse(
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       bool> *callback,
    const RDMAPIImplResponseStatus &status,
    const string &data) {
  static const unsigned int DATA_SIZE = 1;
  ResponseStatus response_status(status, data);
  bool option = false;

  if (response_status.ResponseType() == ResponseStatus::VALID_RESPONSE) {
    if (data.size() == DATA_SIZE) {
      option = data.data()[0];
    } else {
      SetIncorrectPDL(&response_status, data.size(), DATA_SIZE);
    }
  }
  callback->Run(response_status, option);
}


/*
 * Handle a response that contains a uint8_t
 */
void RDMAPI::_HandleU8Response(
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       uint8_t> *callback,
    const RDMAPIImplResponseStatus &status,
    const string &data) {
  ResponseStatus response_status(status, data);
  uint8_t value = 0;

  if (response_status.ResponseType() == ResponseStatus::VALID_RESPONSE) {
    if (data.size() == sizeof(value)) {
      value = data.data()[0];
    } else {
      SetIncorrectPDL(&response_status, data.size(), sizeof(value));
    }
  }
  callback->Run(response_status, value);
}


/*
 * Handle a response that contains a uint32_t
 */
void RDMAPI::_HandleU32Response(
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       uint32_t> *callback,
    const RDMAPIImplResponseStatus &status,
    const string &data) {
  ResponseStatus response_status(status, data);
  uint32_t value = 0;

  if (response_status.ResponseType() == ResponseStatus::VALID_RESPONSE) {
    if (data.size() == sizeof(value)) {
      const uint32_t *ptr = reinterpret_cast<const uint32_t*>(data.data());
      value = NetworkToHost(*ptr);
    } else {
      SetIncorrectPDL(&response_status, data.size(), sizeof(value));
    }
  }
  callback->Run(response_status, value);
}


/*
 * Handle a response that doesn't contain any data
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
      device_count = NetworkToHost(unpacked_data.device_count);
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
      short_message = NetworkToHost(unpacked_data.short_message);
      length_mismatch = NetworkToHost(unpacked_data.length_mismatch);
      checksum_fail = NetworkToHost(unpacked_data.checksum_fail);
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
                       const vector<StatusMessage>&> *callback,
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
        StatusMessage msg_object;
        msg_object.sub_device = (message.sub_device_hi << 8) +
            message.sub_device_lo;
        msg_object.status_message_id = (message.message_id_hi << 8) +
            message.message_id_lo;
        msg_object.value1 = (message.value_1_hi << 8) + message.value_1_lo;
        msg_object.value2 = (message.value_2_hi << 8) + message.value_2_lo;
        msg_object.status_type = message.status_type;
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
                       const vector<uint16_t>&> *callback,
    const RDMAPIImplResponseStatus &status,
    const string &data) {
  ResponseStatus response_status(status, data);
  vector<uint16_t> pids;

  unsigned int data_size = data.size();
  if (response_status.ResponseType() == ResponseStatus::VALID_RESPONSE) {
    if (data_size % 2 == 0) {
      const uint16_t *start = reinterpret_cast<const uint16_t*>(data.data());
      const uint16_t *end = start + (data_size / sizeof(*start));
      for (const uint16_t *ptr = start; ptr < end; ptr++) {
        pids.push_back(NetworkToHost(*ptr));
      }
    } else {
      response_status.MalformedResponse("PDL size not a multiple of 2 : " +
          IntToString(static_cast<int>(data_size)));
    }
    sort(pids.begin(), pids.end());
  }
  callback->Run(response_status, pids);
}


/*
 * Handle a PARAMETER_DESCRIPTION message
 */
void RDMAPI::_HandleGetParameterDescriptor(
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       const ParameterDescriptor&> *callback,
    const RDMAPIImplResponseStatus &status,
    const string &data) {
  ResponseStatus response_status(status, data);
  ParameterDescriptor description;

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
      uint32_t max_value;
      uint32_t default_value;
      // +1 for a null since it's not clear in the spec if this is null
      // terminated
      char description[LABEL_SIZE + 1];
    } __attribute__((packed));
    struct param_description raw_description;

    unsigned int max = sizeof(raw_description) - 1;
    unsigned int min = max - LABEL_SIZE;
    unsigned int data_size = data.size();
    if (data_size >= min && data_size <= max) {
      memcpy(&raw_description, data.data(),
             std::min(static_cast<unsigned int>(data.size()), max));
      description.description[LABEL_SIZE] = 0;
      description.pid = NetworkToHost(raw_description.pid);
      description.pdl_size = raw_description.pdl_size;
      description.data_type = raw_description.data_type;
      description.command_class = raw_description.command_class;
      description.unit = raw_description.unit;
      description.prefix = raw_description.prefix;
      description.min_value = NetworkToHost(raw_description.min_value);
      description.default_value = NetworkToHost(raw_description.default_value);
      description.max_value = NetworkToHost(raw_description.max_value);
      unsigned int label_size = data_size - (
          sizeof(raw_description) - LABEL_SIZE - 1);
      description.description = std::string(raw_description.description,
                                            label_size);
      ShortenString(&description.description);
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
void RDMAPI::_HandleGetDeviceDescriptor(
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       const DeviceDescriptor&> *callback,
    const RDMAPIImplResponseStatus &status,
    const string &data) {
  ResponseStatus response_status(status, data);
  DeviceDescriptor device_info;

  if (response_status.ResponseType() == ResponseStatus::VALID_RESPONSE) {
    unsigned int data_size = data.size();
    if (data_size == sizeof(device_info)) {
      memcpy(&device_info, data.data(), sizeof(device_info));
      device_info.device_model = NetworkToHost(device_info.device_model);
      device_info.product_category =
        NetworkToHost(device_info.product_category);
      device_info.software_version =
        NetworkToHost(device_info.software_version);
      device_info.dmx_footprint =
        NetworkToHost(device_info.dmx_footprint);
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
                       const vector<uint16_t>&> *callback,
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
      const uint16_t *end = start + (data_size / sizeof(*start));
      for (const uint16_t *ptr = start; ptr < end; ptr++) {
        product_detail_ids.push_back(NetworkToHost(*ptr));
      }
    }
  }
  callback->Run(response_status, product_detail_ids);
}


/*
 * Handle a LANGUAGE_CAPABILITIES response
 */
void RDMAPI::_HandleGetLanguageCapabilities(
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       const vector<string>&> *callback,
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


/*
 * Handle a get DMX_PERSONALITY response
 */
void RDMAPI::_HandleGetDMXPersonality(
    SingleUseCallback3<void,
                       const ResponseStatus&,
                       uint8_t,
                       uint8_t> *callback,
    const RDMAPIImplResponseStatus &status,
    const string &data) {
  ResponseStatus response_status(status, data);
  static const unsigned int DATA_SIZE = 2;
  uint8_t current_personality = 0;
  uint8_t personality_count = 0;
  if (response_status.ResponseType() == ResponseStatus::VALID_RESPONSE) {
    if (data.size() != DATA_SIZE) {
      SetIncorrectPDL(&response_status, data.size(), DATA_SIZE);
    } else {
      current_personality = data[0];
      personality_count = data[1];
    }
  }
  callback->Run(response_status, current_personality, personality_count);
}


/*
 * Handle a get DMX_PERSONALITY_DESCRIPTION response
 */
void RDMAPI::_HandleGetDMXPersonalityDescription(
    SingleUseCallback4<void,
                       const ResponseStatus&,
                       uint8_t,
                       uint16_t,
                       const string&> *callback,
    const RDMAPIImplResponseStatus &status,
    const string &data) {
  ResponseStatus response_status(status, data);

  uint8_t personality = 0;
  uint16_t dmx_slots = 0;
  string description;

  if (response_status.ResponseType() == ResponseStatus::VALID_RESPONSE) {
    struct personality_description {
      uint8_t personality;
      uint16_t dmx_slots;
      // +1 for a null since it's not clear in the spec if this is null
      // terminated
      char description[LABEL_SIZE + 1];
    } __attribute__((packed));
    struct personality_description raw_description;

    unsigned int max = sizeof(personality_description) - 1;
    unsigned int min = max - LABEL_SIZE;
    unsigned int data_size = data.size();
    if (data_size >= min && data_size <= max) {
      memcpy(&raw_description, data.data(),
             std::min(static_cast<unsigned int>(data.size()), max));
      personality = raw_description.personality;
      dmx_slots = NetworkToHost(raw_description.dmx_slots);
      description = std::string(raw_description.description, data_size - min);
      ShortenString(&description);
    } else {
      std::stringstream str;
      str << data_size << " needs to be between " << min << " and " << max;
      response_status.MalformedResponse(str.str());
    }
  }
  callback->Run(response_status, personality, dmx_slots, description);
}


/*
 * Handle a get DMX_START_ADDRESS response
 */
void RDMAPI::_HandleGetDMXAddress(
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       uint16_t> *callback,
    const RDMAPIImplResponseStatus &status,
    const string &data) {
  ResponseStatus response_status(status, data);
  static const unsigned int DATA_SIZE = 2;
  uint16_t start_address = 0;
  if (response_status.ResponseType() == ResponseStatus::VALID_RESPONSE) {
    if (data.size() != DATA_SIZE) {
      SetIncorrectPDL(&response_status, data.size(), DATA_SIZE);
    } else {
      start_address = *(reinterpret_cast<const uint16_t*>(data.data()));
      start_address = NetworkToHost(start_address);
    }
  }
  callback->Run(response_status, start_address);
}


/*
 * Handle a get SLOT_INFO response
 */
void RDMAPI::_HandleGetSlotInfo(
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       const vector<SlotDescriptor>&> *callback,
    const RDMAPIImplResponseStatus &status,
    const string &data) {
  ResponseStatus response_status(status, data);
  vector<SlotDescriptor> slots;
  SlotDescriptor slot_info;
  unsigned int slot_info_size = sizeof(slot_info);

  unsigned int data_size = data.size();
  if (response_status.ResponseType() == ResponseStatus::VALID_RESPONSE) {
    if (data_size % slot_info_size) {
      response_status.MalformedResponse("PDL size not a multiple of " +
          IntToString(slot_info_size) + ", was " +
          IntToString(static_cast<int>(data_size)));
    } else {
      const uint8_t *ptr = reinterpret_cast<const uint8_t*>(data.data());
      const uint8_t *end = ptr + data.size();
      while (ptr < end) {
        memcpy(&slot_info, ptr, slot_info_size);
        slot_info.slot_offset = NetworkToHost(slot_info.slot_offset);
        slot_info.slot_label = NetworkToHost(slot_info.slot_label);
        slots.push_back(slot_info);
      }
    }
  }
  callback->Run(response_status, slots);
}


/*
 * Handle a get SLOT_DESCRIPTION response
 */
void RDMAPI::_HandleGetSlotDescription(
    SingleUseCallback3<void,
                       const ResponseStatus&,
                       uint16_t,
                       const string&> *callback,
    const RDMAPIImplResponseStatus &status,
    const string &data) {
  ResponseStatus response_status(status, data);

  uint16_t slot_index = 0;
  string description;

  if (response_status.ResponseType() == ResponseStatus::VALID_RESPONSE) {
    struct slot_description {
      uint16_t slot_index;
      // +1 for a null since it's not clear in the spec if this is null
      // terminated
      char description[LABEL_SIZE + 1];
    } __attribute__((packed));
    struct slot_description raw_description;

    unsigned int max = sizeof(raw_description) - 1;
    unsigned int min = max - LABEL_SIZE;
    unsigned int data_size = data.size();
    if (data_size >= min && data_size <= max) {
      raw_description.description[LABEL_SIZE] = 0;
      memcpy(&raw_description, data.data(), data.size());
      slot_index = NetworkToHost(raw_description.slot_index);
      description = std::string(raw_description.description,
                                data.size() - min);
      ShortenString(&description);
    } else {
      std::stringstream str;
      str << data_size << " needs to be between " << min << " and " << max;
      response_status.MalformedResponse(str.str());
    }
  }
  callback->Run(response_status, slot_index, description);
}


/*
 * Handle a get DEFAULT_SLOT_VALUE response
 */
void RDMAPI::_HandleGetSlotDefaultValues(
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       const vector<SlotDefault>&> *callback,
    const RDMAPIImplResponseStatus &status,
    const string &data) {
  ResponseStatus response_status(status, data);
  vector<SlotDefault> slots;
  SlotDefault slot_default;
  unsigned int slot_default_size = sizeof(slot_default);

  unsigned int data_size = data.size();
  if (response_status.ResponseType() == ResponseStatus::VALID_RESPONSE) {
    if (data_size % slot_default_size) {
      response_status.MalformedResponse("PDL size not a multiple of " +
          IntToString(slot_default_size) + ", was " +
          IntToString(static_cast<int>(data_size)));
    } else {
      const uint8_t *ptr = reinterpret_cast<const uint8_t*>(data.data());
      const uint8_t *end = ptr + data.size();
      while (ptr < end) {
        memcpy(&slot_default, ptr, slot_default_size);
        slot_default.slot_offset = NetworkToHost(slot_default.slot_offset);
        slots.push_back(slot_default);
      }
    }
  }
  callback->Run(response_status, slots);
}


/*
 * Handle a SENSOR_DEFINITION response
 */
void RDMAPI::_HandleGetSensorDefinition(
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       const SensorDescriptor&> *callback,
    const RDMAPIImplResponseStatus &status,
    const string &data) {
  ResponseStatus response_status(status, data);
  SensorDescriptor sensor;

  if (response_status.ResponseType() == ResponseStatus::VALID_RESPONSE) {
    struct sensor_definition_s {
      uint8_t sensor_number;
      uint8_t type;
      uint8_t unit;
      uint8_t prefix;
      int16_t range_min;
      int16_t range_max;
      int16_t normal_min;
      int16_t normal_max;
      uint8_t recorded_value_support;
      char description[LABEL_SIZE + 1];
    } __attribute__((packed));
    struct sensor_definition_s raw_description;

    unsigned int max = sizeof(raw_description) - 1;
    unsigned int min = max - LABEL_SIZE;
    unsigned int data_size = data.size();
    if (data_size >= min && data_size <= max) {
      memcpy(&raw_description, data.data(),
             std::min(static_cast<unsigned int>(data.size()), max));

      sensor.sensor_number = raw_description.sensor_number;
      sensor.type = raw_description.type;
      sensor.unit = raw_description.unit;
      sensor.prefix = raw_description.prefix;
      sensor.range_min = NetworkToHost(raw_description.range_min);
      sensor.range_max = NetworkToHost(raw_description.range_max);
      sensor.normal_min = NetworkToHost(raw_description.normal_min);
      sensor.normal_max = NetworkToHost(raw_description.normal_max);
      sensor.recorded_value_support = raw_description.recorded_value_support;
      sensor.description = std::string(raw_description.description,
                                       data_size - min);
      ShortenString(&sensor.description);
    } else {
      std::stringstream str;
      str << data_size << " needs to be between " << min << " and " << max;
      response_status.MalformedResponse(str.str());
    }
  }
  callback->Run(response_status, sensor);
}


/*
 * Handle a SENSOR_VALUE response
 */
void RDMAPI::_HandleSensorValue(
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       const SensorValueDescriptor&> *callback,
    const RDMAPIImplResponseStatus &status,
    const string &data) {
  ResponseStatus response_status(status, data);
  SensorValueDescriptor sensor;

  if (response_status.ResponseType() == ResponseStatus::VALID_RESPONSE) {
    unsigned int data_size = data.size();
    if (data_size == sizeof(sensor)) {
      memcpy(&sensor, data.data(), sizeof(sensor));
      sensor.present_value = NetworkToHost(sensor.present_value);
      sensor.lowest = NetworkToHost(sensor.lowest);
      sensor.highest = NetworkToHost(sensor.highest);
      sensor.recorded = NetworkToHost(sensor.recorded);
    } else {
      SetIncorrectPDL(&response_status, data.size(), sizeof(sensor));
    }
  }
  callback->Run(response_status, sensor);
}


/*
 * Handle a REAL_TIME_CLOCK response
 */
void RDMAPI::_HandleClock(
    SingleUseCallback2<void,
                       const ResponseStatus&,
                       const ClockValue&> *callback,
    const RDMAPIImplResponseStatus &status,
    const string &data) {
  ResponseStatus response_status(status, data);
  ClockValue clock;

  if (response_status.ResponseType() == ResponseStatus::VALID_RESPONSE) {
    unsigned int data_size = data.size();
    if (data_size == sizeof(clock)) {
      memcpy(&clock, data.data(), sizeof(clock));
      clock.year = NetworkToHost(clock.year);
    } else {
      SetIncorrectPDL(&response_status, data.size(), sizeof(clock));
    }
  }
  callback->Run(response_status, clock);
}


/**
 * Handle a PID_SELF_TEST_DESCRIPTION response.
 */
void RDMAPI::_HandleSelfTestDescription(
    SingleUseCallback3<void,
                       const ResponseStatus&,
                       uint8_t,
                       const string&> *callback,
    const RDMAPIImplResponseStatus &status,
    const string &data) {
  ResponseStatus response_status(status, data);

  uint8_t self_test_number = 0;
  string description;

  if (response_status.ResponseType() == ResponseStatus::VALID_RESPONSE) {
    struct self_test_description {
      uint8_t self_test_number;
      // +1 for a null since it's not clear in the spec if this is null
      // terminated
      char description[LABEL_SIZE + 1];
    } __attribute__((packed));
    struct self_test_description raw_description;

    unsigned int max = sizeof(raw_description) - 1;
    unsigned int min = max - LABEL_SIZE;
    unsigned int data_size = data.size();
    if (data_size >= min && data_size <= max) {
      raw_description.description[LABEL_SIZE] = 0;
      memcpy(&raw_description, data.data(), data.size());
      self_test_number = raw_description.self_test_number;
      description = std::string(raw_description.description,
                                data.size() - min);
      ShortenString(&description);
    } else {
      std::stringstream str;
      str << data_size << " needs to be between " << min << " and " << max;
      response_status.MalformedResponse(str.str());
    }
  }
  callback->Run(response_status, self_test_number, description);
}


/**
 * Handle a PID_PRESET_PLAYBACK response
 */
void RDMAPI::_HandlePlaybackMode(
    SingleUseCallback3<void,
                       const ResponseStatus&,
                       uint16_t,
                       uint8_t> *callback,
    const RDMAPIImplResponseStatus &status,
    const string &data) {
  ResponseStatus response_status(status, data);

  uint16_t mode = 0;
  uint8_t level = 0;

  if (response_status.ResponseType() == ResponseStatus::VALID_RESPONSE) {
    struct preset_mode {
      uint16_t mode;
      uint8_t level;
    } __attribute__((packed));
    struct preset_mode raw_config;

    if (data.size() >= sizeof(raw_config)) {
      memcpy(&raw_config, data.data(), data.size());
      mode = NetworkToHost(raw_config.mode);
      level = raw_config.level;
    } else {
      std::stringstream str;
      str << data.size() << " needs to be more than " << sizeof(raw_config);
      response_status.MalformedResponse(str.str());
    }
  }
  callback->Run(response_status, mode, level);
}


//-----------------------------------------------------------------------------
// Private methods follow

// get a 8 bit value
bool RDMAPI::GenericGetU8(
    unsigned int universe,
    const UID &uid,
    uint8_t sub_device,
    SingleUseCallback2<void, const ResponseStatus&, uint8_t> *callback,
    uint16_t pid,
    string *error) {
  if (CheckNotBroadcast(uid, error, callback))
    return false;
  if (CheckValidSubDevice(sub_device, false, error, callback))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleU8Response,
    callback);
  return CheckReturnStatus(
    m_impl->RDMGet(cb,
                   universe,
                   uid,
                   sub_device,
                   pid),
    error);
}


// set an 8 bit value
bool RDMAPI::GenericSetU8(
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    uint8_t value,
    SingleUseCallback1<void, const ResponseStatus&> *callback,
    uint16_t pid,
    string *error) {
  if (CheckValidSubDevice(sub_device, true, error, callback))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleEmptyResponse,
    callback);
  return CheckReturnStatus(
    m_impl->RDMSet(cb,
                   universe,
                   uid,
                   sub_device,
                   pid,
                   &value,
                   sizeof(value)),
    error);
}


// get a 32 bit value
bool RDMAPI::GenericGetU32(
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    SingleUseCallback2<void, const ResponseStatus&, uint32_t> *callback,
    uint16_t pid,
    string *error) {
  if (CheckNotBroadcast(uid, error, callback))
    return false;
  if (CheckValidSubDevice(sub_device, false, error, callback))
    return false;

  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleU32Response,
    callback);
  return CheckReturnStatus(
    m_impl->RDMGet(cb,
                   universe,
                   uid,
                   sub_device,
                   pid),
    error);
}


// set a 32 bit value
bool RDMAPI::GenericSetU32(
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    uint32_t value,
    SingleUseCallback1<void, const ResponseStatus&> *callback,
    uint16_t pid,
    string *error) {
  if (CheckValidSubDevice(sub_device, true, error, callback))
    return false;

  value = HostToNetwork(value);
  RDMAPIImplInterface::rdm_callback *cb = NewSingleCallback(
    this,
    &RDMAPI::_HandleEmptyResponse,
    callback);
  return CheckReturnStatus(
    m_impl->RDMSet(cb,
                   universe,
                   uid,
                   sub_device,
                   pid,
                   reinterpret_cast<const uint8_t*>(&value),
                   sizeof(value)),
    error);
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
