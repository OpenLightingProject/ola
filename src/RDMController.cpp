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
 *  RDMController.cpp
 *  A command line based RDM controller
 *  Copyright (C) 2010 Simon Newton
 */

#include <ola/Callback.h>
#include <ola/Logging.h>
#include <ola/StringUtils.h>
#include <ola/rdm/RDMAPI.h>
#include <ola/rdm/UID.h>

#include <algorithm>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "src/RDMController.h"
#include "src/RDMHandler.h"

using std::map;
using std::string;
using std::vector;
using ola::rdm::RDMAPI;
using ola::rdm::UID;


map<uint16_t, const RDMController::pid_descriptor> RDMController::s_pid_map;

/*
 * Make a GET/SET PID request
 * @param uid
 * @param sub_device
 * @param set, true if this is a set request, false for a get request
 * @param pid_name, the name of the pid to get/set
 * @param params, optional data for the request
 * @param error, a pointer to a string which is updated with the error
 * @returns true if ok, false otherwise
 */
bool RDMController::RequestPID(const UID &uid,
                               uint16_t sub_device,
                               bool set,
                               uint16_t pid,
                               const vector<string> &params,
                               string *error) {
  map<uint16_t, const pid_descriptor>::const_iterator iter =
    s_pid_map.find(pid);

  if (iter == s_pid_map.end()) {
    *error = "Unknown PID";
    return false;
  }

  CheckMethod check_method = set ? iter->second.set_verify :
    iter->second.get_verify;
  if (!check_method) {
    *error = set ? "Set" : "Get";
    *error += " not permitted";
    return false;
  }

  if (!(this->*check_method)(uid,
                             sub_device,
                             params,
                             error))
    return false;

  ExecuteMethod exec_method = set ? iter->second.set_execute :
    iter->second.get_execute;

  if (!exec_method) {
    *error = "No get/set method supplied, this indicates a bug!";
    return false;
  }
  return (this->*exec_method)(uid, sub_device, params, error);
}


/*
 * Populate the pid maps
 */
void RDMController::LoadMap() {
  if (s_pid_map.size())
    return;

  // populate the PID descriptor map
  MakeDescriptor(ola::rdm::PID_PROXIED_DEVICES,
    &RDMController::NoArgsRootDeviceCheck,
    NULL,
    &RDMController::GetProxiedDeviceCount,
    NULL);
  MakeDescriptor(ola::rdm::PID_PROXIED_DEVICE_COUNT,
    &RDMController::NoArgsRootDeviceCheck,
    NULL,
    &RDMController::GetProxiedDevices,
    NULL);
  MakeDescriptor(ola::rdm::PID_COMMS_STATUS,
    &RDMController::NoArgsRootDeviceCheck,
    &RDMController::NoArgsRootDeviceCheck,
    &RDMController::GetCommStatus,
    &RDMController::ClearCommStatus);
  MakeDescriptor(ola::rdm::PID_STATUS_MESSAGES,
    &RDMController::StatusTypeCheck,
    NULL,
    &RDMController::GetStatusMessage,
    NULL);
  MakeDescriptor(ola::rdm::PID_STATUS_ID_DESCRIPTION,
    &RDMController::UInt16Check,
    NULL,
    &RDMController::GetStatusIdDescription,
    NULL);
  MakeDescriptor(ola::rdm::PID_CLEAR_STATUS_ID,
    NULL,
    &RDMController::NoArgsValidBroadcastSubDeviceCheck,
    NULL,
    &RDMController::ClearStatusId);
  MakeDescriptor(ola::rdm::PID_SUB_DEVICE_STATUS_REPORT_THRESHOLD,
    &RDMController::NoArgsValidSubDeviceCheck,
    //&RDMController::StatusTypeCheck,
    NULL,
    &RDMController::GetSubDeviceReporting,
    &RDMController::SetSubDeviceReporting);
  MakeDescriptor(ola::rdm::PID_SUPPORTED_PARAMETERS,
    &RDMController::NoArgsValidSubDeviceCheck,
    NULL,
    &RDMController::GetSupportedParameters,
    NULL);
  MakeDescriptor(ola::rdm::PID_PARAMETER_DESCRIPTION,
    NULL,
    NULL,
    &RDMController::GetParameterDescription,
    NULL);
  MakeDescriptor(ola::rdm::PID_DEVICE_INFO,
    &RDMController::NoArgsValidSubDeviceCheck,
    NULL,
    &RDMController::GetDeviceInfo,
    NULL);
  MakeDescriptor(ola::rdm::PID_PRODUCT_DETAIL_ID_LIST,
    &RDMController::NoArgsValidSubDeviceCheck,
    NULL,
    &RDMController::GetProductDetailIdList,
    NULL);
  MakeDescriptor(ola::rdm::PID_DEVICE_MODEL_DESCRIPTION,
    &RDMController::NoArgsValidSubDeviceCheck,
    NULL,
    &RDMController::GetDeviceModelDescription,
    NULL);
  MakeDescriptor(ola::rdm::PID_MANUFACTURER_LABEL,
    &RDMController::NoArgsValidSubDeviceCheck,
    NULL,
    &RDMController::GetManufacturerLabel,
    NULL);
  MakeDescriptor(ola::rdm::PID_DEVICE_LABEL,
    &RDMController::NoArgsValidSubDeviceCheck,
    NULL,
    &RDMController::GetDeviceLabel,
    &RDMController::SetDeviceLabel);
  MakeDescriptor(ola::rdm::PID_FACTORY_DEFAULTS,
    &RDMController::NoArgsValidSubDeviceCheck,
    &RDMController::NoArgsValidBroadcastSubDeviceCheck,
    &RDMController::GetFactoryDefaults,
    &RDMController::ResetToFactoryDefaults);
  MakeDescriptor(ola::rdm::PID_LANGUAGE_CAPABILITIES,
    &RDMController::NoArgsValidSubDeviceCheck,
    NULL,
    &RDMController::GetLanguageCapabilities,
    NULL);
  MakeDescriptor(ola::rdm::PID_LANGUAGE,
    &RDMController::NoArgsValidSubDeviceCheck,
    NULL,
    &RDMController::GetLanguage,
    &RDMController::SetLanguage);
  MakeDescriptor(ola::rdm::PID_SOFTWARE_VERSION_LABEL,
    &RDMController::NoArgsValidSubDeviceCheck,
    NULL,
    &RDMController::GetSoftwareVersionLabel,
    NULL);
  MakeDescriptor(ola::rdm::PID_BOOT_SOFTWARE_VERSION_ID,
    &RDMController::NoArgsValidSubDeviceCheck,
    NULL,
    &RDMController::GetBootSoftwareVersion,
    NULL);
  MakeDescriptor(ola::rdm::PID_BOOT_SOFTWARE_VERSION_LABEL,
    &RDMController::NoArgsValidSubDeviceCheck,
    NULL,
    &RDMController::GetBootSoftwareVersionLabel,
    NULL);
  // DONE UP TO HERE!!!
  MakeDescriptor(ola::rdm::PID_DMX_PERSONALITY,
    NULL,
    NULL,
    &RDMController::GetDMXPersonality,
    &RDMController::SetDMXPersonality);
  MakeDescriptor(ola::rdm::PID_DMX_PERSONALITY_DESCRIPTION,
    NULL,
    NULL,
    &RDMController::GetDMXPersonalityDescription,
    NULL);
  MakeDescriptor(ola::rdm::PID_DMX_START_ADDRESS,
    NULL,
    NULL,
    &RDMController::GetDMXAddress,
    &RDMController::SetDMXAddress);
  MakeDescriptor(ola::rdm::PID_SLOT_INFO,
    NULL,
    NULL,
    &RDMController::GetSlotInfo,
    NULL);
  MakeDescriptor(ola::rdm::PID_SLOT_DESCRIPTION,
    NULL,
    NULL,
    &RDMController::GetSlotDescription,
    NULL);
  MakeDescriptor(ola::rdm::PID_DEFAULT_SLOT_VALUE,
    NULL,
    NULL,
    &RDMController::GetSlotDefaultValues,
    NULL);
  MakeDescriptor(ola::rdm::PID_SENSOR_DEFINITION,
    NULL,
    NULL,
    &RDMController::GetSensorDefinition,
    NULL);
  MakeDescriptor(ola::rdm::PID_SENSOR_VALUE,
    NULL,
    NULL,
    &RDMController::GetSensorValue,
    &RDMController::SetSensorValue);
  MakeDescriptor(ola::rdm::PID_RECORD_SENSORS,
    NULL,
    NULL,
    &RDMController::RecordSensors,
    NULL);
  MakeDescriptor(ola::rdm::PID_DEVICE_HOURS,
    NULL,
    NULL,
    &RDMController::GetDeviceHours,
    &RDMController::SetDeviceHours);
  MakeDescriptor(ola::rdm::PID_LAMP_HOURS,
    NULL,
    NULL,
    &RDMController::GetLampHours,
    &RDMController::SetLampHours);
  MakeDescriptor(ola::rdm::PID_LAMP_STRIKES,
    NULL,
    NULL,
    &RDMController::GetLampStrikes,
    &RDMController::SetLampStrikes);
  MakeDescriptor(ola::rdm::PID_LAMP_STATE,
    NULL,
    NULL,
    &RDMController::GetLampState,
    &RDMController::SetLampState);
  MakeDescriptor(ola::rdm::PID_LAMP_ON_MODE,
    NULL,
    NULL,
    &RDMController::GetLampMode,
    &RDMController::SetLampMode);
  MakeDescriptor(ola::rdm::PID_DEVICE_POWER_CYCLES,
    NULL,
    NULL,
    &RDMController::GetDevicePowerCycles,
    &RDMController::SetDevicePowerCycles);
  MakeDescriptor(ola::rdm::PID_DISPLAY_INVERT,
    NULL,
    NULL,
    &RDMController::GetDisplayInvert,
    &RDMController::SetDisplayInvert);
  MakeDescriptor(ola::rdm::PID_DISPLAY_LEVEL,
    NULL,
    NULL,
    &RDMController::GetDisplayLevel,
    &RDMController::SetDisplayLevel);
  MakeDescriptor(ola::rdm::PID_PAN_INVERT,
    NULL,
    NULL,
    &RDMController::GetPanInvert,
    &RDMController::SetPanInvert);
  MakeDescriptor(ola::rdm::PID_TILT_INVERT,
    NULL,
    NULL,
    &RDMController::GetTiltInvert,
    &RDMController::SetTiltInvert);
  MakeDescriptor(ola::rdm::PID_PAN_TILT_SWAP,
    NULL,
    NULL,
    &RDMController::GetPanTiltSwap,
    &RDMController::SetPanTiltSwap);
  MakeDescriptor(ola::rdm::PID_IDENTIFY_DEVICE,
    NULL,
    NULL,
    &RDMController::GetIdentifyMode,
    &RDMController::IdentifyDevice);
  MakeDescriptor(ola::rdm::PID_RESET_DEVICE,
    NULL,
    NULL,
    &RDMController::ResetDevice,
    NULL);
}


//-----------------------------------------------------------------------------

// First up are generic check methods


/*
 * Check that no args have been supplied and that the sub device is 0
 */
bool RDMController::NoArgsRootDeviceCheck(const UID &uid,
                                          uint16_t sub_device,
                                          const vector<string> &args,
                                          string *error) {
  if (sub_device) {
    *error = "Sub device must be 0 (root device)";
    return false;
  }
  return NoArgsGetCheck(uid, sub_device, args, error);
}


/*
 * Check the sub device is valid
 */
bool RDMController::NoArgsValidSubDeviceCheck(const UID &uid,
                                              uint16_t sub_device,
                                              const vector<string> &args,
                                              string *error) {
  if (sub_device > 0x0200) {
    *error = "Sub device must be <= 512";
    return false;
  }
  return NoArgsGetCheck(uid, sub_device, args, error);
}


/*
 * Check the sub device is valid or broadcast
 */
bool RDMController::NoArgsValidBroadcastSubDeviceCheck(
    const UID &uid,
    uint16_t sub_device,
    const vector<string> &args,
    string *error) {
  if (sub_device > 0x0200 && sub_device != ola::rdm::ALL_RDM_SUBDEVICES) {
    *error = "Sub device must be <= 512 or 65535";
    return false;
  }
  return NoArgsGetCheck(uid, sub_device, args, error);
}



/*
 * Check that no arguments were supplied and this isn't broadcast
 */
bool RDMController::NoArgsGetCheck(const UID &uid,
                                   uint16_t sub_device,
                                   const vector<string> &args,
                                   string *error) {
  if (args.size()) {
    *error = "No args required";
    return false;
  }
  return true;
}


/*
 * Check for no args, and the root device
 */
bool RDMController::NoArgsRootDeviceSetCheck(const UID &uid,
                                             uint16_t sub_device,
                                             const vector<string> &args,
                                             string *error) {
  if (sub_device) {
    *error = "Sub device must be 0 (root device)";
    return false;
  }
  if (args.size()) {
    *error = "No args required";
    return false;
  }
  return true;
}


/*
 * Check the sub device is valid or broadcast
 */
bool RDMController::StatusTypeCheck(const UID &uid,
                                    uint16_t sub_device,
                                    const vector<string> &args,
                                    string *error) {
  if (sub_device) {
    *error = "Sub device must be 0 (root device)";
    return false;
  }
  if (!args.size() || args.size() > 1) {
    *error = "Requires one of {none,error,warning,advisory}";
    return false;
  }
  ola::rdm::rdm_status_type status_type;
  if (!StringToStatusType(args[0], &status_type)) {
    *error = "Invalid arg: ";
    *error += args[0];
    return false;
  }
  return true;
}


/*
 * Check the sub device is valid or broadcast
 */
bool RDMController::UInt16Check(const UID &uid,
                                uint16_t sub_device,
                                const vector<string> &args,
                                string *error) {
  if (sub_device) {
    *error = "Sub device must be 0 (root device)";
    return false;
  }
  if (!args.size() || args.size() > 1) {
    *error = "Requires a unsigned 16 bit int";
    return false;
  }
  uint16_t value;
  if (!ola::StringToUInt16(args[0], &value)) {
    *error = "Invalid arg";
    return false;
  }
  return true;
}


// Custom verify methods


// Now we have the get/ set methods
//-----------------------------------------------------------------------------

bool RDMController::GetProxiedDeviceCount(const UID &uid,
                                          uint16_t sub_device,
                                          const vector<string> &args,
                                          string *error) {
  return m_api->GetProxiedDeviceCount(
      uid,
      ola::NewSingleCallback(m_handler, &ResponseHandler::ProxiedDeviceCount),
      error);
}


bool RDMController::GetProxiedDevices(const UID &uid,
                                      uint16_t sub_device,
                                      const vector<string> &args,
                                      string *error) {
  return m_api->GetProxiedDevices(
      uid,
      ola::NewSingleCallback(m_handler, &ResponseHandler::ProxiedDevices),
      error);
}


bool RDMController::GetCommStatus(const UID &uid,
                                  uint16_t sub_device,
                                  const vector<string> &args,
                                  string *error) {
  return m_api->GetCommStatus(
      uid,
      ola::NewSingleCallback(m_handler, &ResponseHandler::CommStatus),
      error);
}


bool RDMController::ClearCommStatus(const UID &uid,
                                    uint16_t sub_device,
                                    const vector<string> &args,
                                    string *error) {
  return m_api->ClearCommStatus(
      uid,
      ola::NewSingleCallback(m_handler, &ResponseHandler::ClearCommStatus),
      error);
}


bool RDMController::GetStatusMessage(const UID &uid,
                                     uint16_t sub_device,
                                     const vector<string> &args,
                                     string *error) {
  ola::rdm::rdm_status_type status_type;
  if (!StringToStatusType(args[0], &status_type)) {
    *error = "arg must be one of {none,error,warning,advisory}";
    return false;
  }
  return m_api->GetStatusMessage(
      uid,
      status_type,
      ola::NewSingleCallback(m_handler,
                             &ResponseHandler::StatusMessages),
      error);
}


bool RDMController::GetStatusIdDescription(const UID &uid,
                                           uint16_t sub_device,
                                           const vector<string> &args,
                                           string *error) {
  uint16_t status_id;
  if (!ola::StringToUInt16(args[0], &status_id))
    return false;
  return m_api->GetStatusIdDescription(
      uid,
      status_id,
      ola::NewSingleCallback(m_handler, &ResponseHandler::StatusIdDescription),
      error);
}


bool RDMController::ClearStatusId(const UID &uid,
                                  uint16_t sub_device,
                                  const vector<string> &args,
                                  string *error) {
  return m_api->ClearStatusId(
      uid,
      sub_device,
      ola::NewSingleCallback(m_handler, &ResponseHandler::ClearStatusId),
      error);
}


bool RDMController::GetSubDeviceReporting(const UID &uid,
                                          uint16_t sub_device,
                                          const vector<string> &args,
                                          string *error) {
  return m_api->GetSubDeviceReporting(
      uid,
      sub_device,
      ola::NewSingleCallback(m_handler, &ResponseHandler::SubDeviceReporting),
      error);
}


bool RDMController::SetSubDeviceReporting(const UID &uid,
                                          uint16_t sub_device,
                                          const vector<string> &args,
                                          string *error) {
  ola::rdm::rdm_status_type status_type;
  if (!StringToStatusType(args[0], &status_type)) {
    *error = "arg must be one of {none, error, warning, advisory}";
    return false;
  }
  return m_api->SetSubDeviceReporting(
      uid,
      sub_device,
      status_type,
      ola::NewSingleCallback(m_handler,
                             &ResponseHandler::SetSubDeviceReporting),
      error);
}


bool RDMController::GetSupportedParameters(const UID &uid,
                                           uint16_t sub_device,
                                           const vector<string> &args,
                                           string *error) {
  return m_api->GetSupportedParameters(
      uid,
      sub_device,
      ola::NewSingleCallback(m_handler, &ResponseHandler::SupportedParameters),
      error);
}


bool RDMController::GetParameterDescription(const UID &uid,
                                            uint16_t sub_device,
                                            const vector<string> &args,
                                            string *error) {
  uint16_t pid;
  if (!ola::StringToUInt16(args[0], &pid))
    return false;
  return m_api->GetParameterDescription(
      uid,
      pid,
      ola::NewSingleCallback(m_handler,
                             &ResponseHandler::ParameterDescription),
      error);
}


bool RDMController::GetDeviceInfo(const UID &uid,
                                  uint16_t sub_device,
                                  const vector<string> &args,
                                  string *error) {
  return m_api->GetDeviceInfo(
      uid,
      sub_device,
      ola::NewSingleCallback(m_handler, &ResponseHandler::DeviceInfo),
      error);
}


bool RDMController::GetProductDetailIdList(const UID &uid,
                                           uint16_t sub_device,
                                           const vector<string> &args,
                                           string *error) {
  return m_api->GetProductDetailIdList(
      uid,
      sub_device,
      ola::NewSingleCallback(m_handler, &ResponseHandler::ProductDetailIdList),
      error);
}


bool RDMController::GetDeviceModelDescription(const UID &uid,
                                              uint16_t sub_device,
                                              const vector<string> &args,
                                              string *error) {
  return m_api->GetDeviceModelDescription(
      uid,
      sub_device,
      ola::NewSingleCallback(m_handler,
                             &ResponseHandler::DeviceModelDescription),
      error);
}


bool RDMController::GetManufacturerLabel(const UID &uid,
                                         uint16_t sub_device,
                                         const vector<string> &args,
                                         string *error) {
  return m_api->GetManufacturerLabel(
      uid,
      sub_device,
      ola::NewSingleCallback(m_handler, &ResponseHandler::ManufacturerLabel),
      error);
}


bool RDMController::GetDeviceLabel(const UID &uid,
                                   uint16_t sub_device,
                                   const vector<string> &args,
                                   string *error) {
  return m_api->GetDeviceLabel(
      uid,
      sub_device,
      ola::NewSingleCallback(m_handler, &ResponseHandler::DeviceLabel),
      error);
}


bool RDMController::SetDeviceLabel(const UID &uid,
                                   uint16_t sub_device,
                                   const vector<string> &args,
                                   string *error) {
  return m_api->SetDeviceLabel(
      uid,
      sub_device,
      args[0],
      ola::NewSingleCallback(m_handler, &ResponseHandler::SetDeviceLabel),
      error);
}


bool RDMController::GetFactoryDefaults(const UID &uid,
                                       uint16_t sub_device,
                                       const vector<string> &args,
                                       string *error) {
  return m_api->GetFactoryDefaults(
      uid,
      sub_device,
      ola::NewSingleCallback(m_handler, &ResponseHandler::FactoryDefaults),
      error);
}


bool RDMController::ResetToFactoryDefaults(const UID &uid,
                                           uint16_t sub_device,
                                           const vector<string> &args,
                                           string *error) {
  return m_api->ResetToFactoryDefaults(
      uid,
      sub_device,
      ola::NewSingleCallback(m_handler,
                             &ResponseHandler::ResetToFactoryDefaults),
      error);
}


bool RDMController::GetLanguageCapabilities(const UID &uid,
                                            uint16_t sub_device,
                                            const vector<string> &args,
                                            string *error) {
  return m_api->GetLanguageCapabilities(
      uid,
      sub_device,
      ola::NewSingleCallback(m_handler,
                             &ResponseHandler::LanguageCapabilities),
      error);
}


bool RDMController::GetLanguage(const UID &uid,
                                uint16_t sub_device,
                                const vector<string> &args,
                                string *error) {
  return m_api->GetLanguage(
      uid,
      sub_device,
      ola::NewSingleCallback(m_handler, &ResponseHandler::Language),
      error);
}


bool RDMController::SetLanguage(const UID &uid,
                                uint16_t sub_device,
                                const vector<string> &args,
                                string *error) {
  string language = args[0].substr(0, 2);
  return m_api->SetLanguage(
      uid,
      sub_device,
      language,
      ola::NewSingleCallback(m_handler, &ResponseHandler::SetLanguage),
      error);
}


bool RDMController::GetSoftwareVersionLabel(const UID &uid,
                                            uint16_t sub_device,
                                            const vector<string> &args,
                                            string *error) {
  return m_api->GetSoftwareVersionLabel(
      uid,
      sub_device,
      ola::NewSingleCallback(m_handler,
                             &ResponseHandler::SoftwareVersionLabel),
      error);
}


bool RDMController::GetBootSoftwareVersion(const UID &uid,
                                           uint16_t sub_device,
                                           const vector<string> &args,
                                           string *error) {
  return m_api->GetBootSoftwareVersion(
      uid,
      sub_device,
      ola::NewSingleCallback(m_handler, &ResponseHandler::BootSoftwareVersion),
      error);
}


bool RDMController::GetBootSoftwareVersionLabel(const UID &uid,
                                                uint16_t sub_device,
                                                const vector<string> &args,
                                                string *error) {
  return m_api->GetBootSoftwareVersionLabel(
      uid,
      sub_device,
      ola::NewSingleCallback(m_handler,
                             &ResponseHandler::BootSoftwareVersionLabel),
      error);
}


bool RDMController::GetDMXPersonality(const UID &uid,
                                      uint16_t sub_device,
                                      const vector<string> &args,
                                      string *error) {
  return m_api->GetDMXPersonality(
      uid,
      sub_device,
      ola::NewSingleCallback(m_handler, &ResponseHandler::DMXPersonality),
      error);
}


bool RDMController::SetDMXPersonality(const UID &uid,
                                      uint16_t sub_device,
                                      const vector<string> &args,
                                      string *error) {
  uint8_t personality;
  if (!ola::StringToUInt8(args[0], &personality))
    return false;
  return m_api->SetDMXPersonality(
      uid,
      sub_device,
      personality,
      ola::NewSingleCallback(m_handler, &ResponseHandler::SetDMXPersonality),
      error);
}


bool RDMController::GetDMXPersonalityDescription(const UID &uid,
                                                 uint16_t sub_device,
                                                 const vector<string> &args,
                                                 string *error) {
  uint8_t personality;
  if (!ola::StringToUInt8(args[0], &personality))
    return false;
  return m_api->GetDMXPersonalityDescription(
      uid,
      sub_device,
      personality,
      ola::NewSingleCallback(m_handler,
                             &ResponseHandler::DMXPersonalityDescription),
      error);
}


bool RDMController::GetDMXAddress(const UID &uid,
                                  uint16_t sub_device,
                                  const vector<string> &args,
                                  string *error) {
  return m_api->GetDMXAddress(
      uid,
      sub_device,
      ola::NewSingleCallback(m_handler, &ResponseHandler::DMXAddress),
      error);
}


bool RDMController::SetDMXAddress(const UID &uid,
                                  uint16_t sub_device,
                                  const vector<string> &args,
                                  string *error) {
  uint16_t dmx_address = 0;
  if (!ola::StringToUInt16(args[0], &dmx_address))
    return false;
  return m_api->SetDMXAddress(
      uid,
      sub_device,
      dmx_address,
      ola::NewSingleCallback(m_handler, &ResponseHandler::SetDMXAddress),
      error);
}


bool RDMController::GetSlotInfo(const UID &uid,
                                uint16_t sub_device,
                                const vector<string> &args,
                                string *error) {
  return m_api->GetSlotInfo(
      uid,
      sub_device,
      ola::NewSingleCallback(m_handler, &ResponseHandler::SlotInfo),
      error);
}


bool RDMController::GetSlotDescription(const UID &uid,
                                       uint16_t sub_device,
                                       const vector<string> &args,
                                       string *error) {
  uint16_t slot_id = 0;
  if (!ola::StringToUInt16(args[0], &slot_id))
    return false;
  return m_api->GetSlotDescription(
      uid,
      sub_device,
      slot_id,
      ola::NewSingleCallback(m_handler, &ResponseHandler::SlotDescription),
      error);
}


bool RDMController::GetSlotDefaultValues(const UID &uid,
                                         uint16_t sub_device,
                                         const vector<string> &args,
                                         string *error) {
  return m_api->GetSlotDefaultValues(
      uid,
      sub_device,
      ola::NewSingleCallback(m_handler, &ResponseHandler::SlotDefaultValues),
      error);
}


bool RDMController::GetSensorDefinition(const UID &uid,
                                        uint16_t sub_device,
                                        const vector<string> &args,
                                        string *error) {
  uint8_t sensor;
  if (!ola::StringToUInt8(args[0], &sensor))
    return false;
  return m_api->GetSensorDefinition(
      uid,
      sub_device,
      sensor,
      ola::NewSingleCallback(m_handler, &ResponseHandler::SensorDefinition),
      error);
}


bool RDMController::GetSensorValue(const UID &uid,
                                   uint16_t sub_device,
                                   const vector<string> &args,
                                   string *error) {
  uint8_t sensor;
  if (!ola::StringToUInt8(args[0], &sensor))
    return false;
  return m_api->GetSensorValue(
      uid,
      sub_device,
      sensor,
      ola::NewSingleCallback(m_handler, &ResponseHandler::SensorValue),
      error);
}


bool RDMController::SetSensorValue(const UID &uid,
                                   uint16_t sub_device,
                                   const vector<string> &args,
                                   string *error) {
  uint8_t sensor;
  if (!ola::StringToUInt8(args[0], &sensor))
    return false;
  return m_api->SetSensorValue(
      uid,
      sub_device,
      sensor,
      ola::NewSingleCallback(m_handler, &ResponseHandler::SetSensorValue),
      error);
}


bool RDMController::RecordSensors(const UID &uid,
                                  uint16_t sub_device,
                                  const vector<string> &args,
                                  string *error) {
  uint8_t sensor;
  if (!ola::StringToUInt8(args[0], &sensor))
    return false;
  return m_api->RecordSensors(
      uid,
      sub_device,
      sensor,
      ola::NewSingleCallback(m_handler, &ResponseHandler::RecordSensors),
      error);
}


bool RDMController::GetDeviceHours(const UID &uid,
                                   uint16_t sub_device,
                                   const vector<string> &args,
                                   string *error) {
  return m_api->GetDeviceHours(
      uid,
      sub_device,
      ola::NewSingleCallback(m_handler, &ResponseHandler::DeviceHours),
      error);
}


bool RDMController::SetDeviceHours(const UID &uid,
                                   uint16_t sub_device,
                                   const vector<string> &args,
                                   string *error) {
  uint32_t device_hours = 0;
  if (!ola::StringToUInt(args[0], &device_hours))
    return false;
  return m_api->SetDeviceHours(
      uid,
      sub_device,
      device_hours,
      ola::NewSingleCallback(m_handler, &ResponseHandler::SetDeviceHours),
      error);
}


bool RDMController::GetLampHours(const UID &uid,
                                 uint16_t sub_device,
                                 const vector<string> &args,
                                 string *error) {
  return m_api->GetLampHours(
      uid,
      sub_device,
      ola::NewSingleCallback(m_handler, &ResponseHandler::LampHours),
      error);
}


bool RDMController::SetLampHours(const UID &uid,
                                 uint16_t sub_device,
                                 const vector<string> &args,
                                 string *error) {
  uint32_t lamp_hours = 0;
  if (!ola::StringToUInt(args[0], &lamp_hours))
    return false;
  return m_api->SetLampHours(
      uid,
      sub_device,
      lamp_hours,
      ola::NewSingleCallback(m_handler, &ResponseHandler::SetLampHours),
      error);
}


bool RDMController::GetLampStrikes(const UID &uid,
                                   uint16_t sub_device,
                                   const vector<string> &args,
                                   string *error) {
  return m_api->GetLampStrikes(
      uid,
      sub_device,
      ola::NewSingleCallback(m_handler, &ResponseHandler::LampStrikes),
      error);
}


bool RDMController::SetLampStrikes(const UID &uid,
                                   uint16_t sub_device,
                                   const vector<string> &args,
                                   string *error) {
  uint32_t lamp_strikes = 0;
  if (!ola::StringToUInt(args[0], &lamp_strikes))
    return false;
  return m_api->SetLampStrikes(
      uid,
      sub_device,
      lamp_strikes,
      ola::NewSingleCallback(m_handler, &ResponseHandler::SetLampStrikes),
      error);
}


bool RDMController::GetLampState(const UID &uid,
                                 uint16_t sub_device,
                                 const vector<string> &args,
                                 string *error) {
  return m_api->GetLampState(
      uid,
      sub_device,
      ola::NewSingleCallback(m_handler,
                             &ResponseHandler::LampState),
      error);
}


bool RDMController::SetLampState(const UID &uid,
                                 uint16_t sub_device,
                                 const vector<string> &args,
                                 string *error) {
  ola::rdm::rdm_lamp_state state;
  if (!StringToLampState(args[0], &state))
    return false;
  return m_api->SetLampState(
      uid,
      sub_device,
      state,
      ola::NewSingleCallback(m_handler, &ResponseHandler::SetLampState),
      error);
}


bool RDMController::GetLampMode(const UID &uid,
                                uint16_t sub_device,
                                const vector<string> &args,
                                string *error) {
  return m_api->GetLampMode(
      uid,
      sub_device,
      ola::NewSingleCallback(m_handler, &ResponseHandler::LampMode),
      error);
}


bool RDMController::SetLampMode(const UID &uid,
                                uint16_t sub_device,
                                const vector<string> &args,
                                string *error) {
  ola::rdm::rdm_lamp_mode mode;
  if (!StringToLampMode(args[0], &mode))
    return false;
  return m_api->SetLampMode(
      uid,
      sub_device,
      mode,
      ola::NewSingleCallback(m_handler, &ResponseHandler::SetLampMode),
      error);
}


bool RDMController::GetDevicePowerCycles(const UID &uid,
                                         uint16_t sub_device,
                                         const vector<string> &args,
                                         string *error) {
  return m_api->GetDevicePowerCycles(
      uid,
      sub_device,
      ola::NewSingleCallback(m_handler, &ResponseHandler::DevicePowerCycles),
      error);
}


bool RDMController::SetDevicePowerCycles(const UID &uid,
                                         uint16_t sub_device,
                                         const vector<string> &args,
                                         string *error) {
  uint32_t power_cycles = 0;
  if (!ola::StringToUInt(args[0], &power_cycles))
    return false;
  return m_api->SetDevicePowerCycles(
      uid,
      sub_device,
      power_cycles,
      ola::NewSingleCallback(m_handler,
                             &ResponseHandler::SetDevicePowerCycles),
      error);
}


bool RDMController::GetDisplayInvert(const UID &uid,
                                     uint16_t sub_device,
                                     const vector<string> &args,
                                     string *error) {
  return m_api->GetDisplayInvert(
      uid,
      sub_device,
      ola::NewSingleCallback(m_handler, &ResponseHandler::DisplayInvert),
      error);
}


bool RDMController::SetDisplayInvert(const UID &uid,
                                     uint16_t sub_device,
                                     const vector<string> &args,
                                     string *error) {
  uint8_t mode;
  if (!StringToOnOffAuto(args[0], &mode))
    return false;
  return m_api->SetDisplayInvert(
      uid,
      sub_device,
      mode,
      ola::NewSingleCallback(m_handler, &ResponseHandler::SetDisplayInvert),
      error);
}


bool RDMController::GetDisplayLevel(const UID &uid,
                                    uint16_t sub_device,
                                    const vector<string> &args,
                                    string *error) {
  return m_api->GetDisplayLevel(
      uid,
      sub_device,
      ola::NewSingleCallback(m_handler, &ResponseHandler::DisplayLevel),
      error);
}


bool RDMController::SetDisplayLevel(const UID &uid,
                                    uint16_t sub_device,
                                    const vector<string> &args,
                                    string *error) {
  uint8_t level;
  if (!ola::StringToUInt8(args[0], &level))
    return false;
  return m_api->SetDisplayLevel(
      uid,
      sub_device,
      level,
      ola::NewSingleCallback(m_handler, &ResponseHandler::SetDisplayLevel),
      error);
}


bool RDMController::GetPanInvert(const UID &uid,
                                 uint16_t sub_device,
                                 const vector<string> &args,
                                 string *error) {
  return m_api->GetPanInvert(
      uid,
      sub_device,
      ola::NewSingleCallback(m_handler, &ResponseHandler::PanInvert),
      error);
}


bool RDMController::SetPanInvert(const UID &uid,
                                 uint16_t sub_device,
                                 const vector<string> &args,
                                 string *error) {
  uint8_t mode;
  if (!StringToOnOff(args[0], &mode))
    return false;
  return m_api->SetPanInvert(
      uid,
      sub_device,
      mode,
      ola::NewSingleCallback(m_handler, &ResponseHandler::SetPanInvert),
      error);
}


bool RDMController::GetTiltInvert(const UID &uid,
                                  uint16_t sub_device,
                                  const vector<string> &args,
                                  string *error) {
  return m_api->GetTiltInvert(
      uid,
      sub_device,
      ola::NewSingleCallback(m_handler, &ResponseHandler::TiltInvert),
      error);
}


bool RDMController::SetTiltInvert(const UID &uid,
                                  uint16_t sub_device,
                                  const vector<string> &args,
                                  string *error) {
  uint8_t mode;
  if (!StringToOnOff(args[0], &mode))
    return false;
  return m_api->SetTiltInvert(
      uid,
      sub_device,
      mode,
      ola::NewSingleCallback(m_handler, &ResponseHandler::SetTiltInvert),
      error);
}


bool RDMController::GetPanTiltSwap(const UID &uid,
                                   uint16_t sub_device,
                                   const vector<string> &args,
                                   string *error) {
  return m_api->GetPanTiltSwap(
      uid,
      sub_device,
      ola::NewSingleCallback(m_handler, &ResponseHandler::PanTiltSwap),
      error);
}


bool RDMController::SetPanTiltSwap(const UID &uid,
                                   uint16_t sub_device,
                                   const vector<string> &args,
                                   string *error) {
  uint8_t mode;
  if (!StringToOnOff(args[0], &mode))
    return false;
  return m_api->SetPanTiltSwap(
      uid,
      sub_device,
      mode,
      ola::NewSingleCallback(m_handler, &ResponseHandler::SetPanTiltSwap),
      error);
}


bool RDMController::GetIdentifyMode(const UID &uid,
                                    uint16_t sub_device,
                                    const vector<string> &args,
                                    string *error) {
  return m_api->GetIdentifyMode(
      uid,
      sub_device,
      ola::NewSingleCallback(m_handler, &ResponseHandler::IdentifyMode),
      error);
}


bool RDMController::IdentifyDevice(const UID &uid,
                                   uint16_t sub_device,
                                   const vector<string> &args,
                                   string *error) {
  uint8_t mode;
  if (!StringToOnOff(args[0], &mode))
    return false;
  return m_api->IdentifyDevice(
      uid,
      sub_device,
      mode,
      ola::NewSingleCallback(m_handler, &ResponseHandler::IdentifyDevice),
      error);
}


bool RDMController::ResetDevice(const UID &uid,
                                uint16_t sub_device,
                                const vector<string> &args,
                                string *error) {
  uint8_t mode;
  if (!StringToWarmCold(args[0], &mode))
    return false;
  return m_api->ResetDevice(
      uid,
      sub_device,
      mode,
      ola::NewSingleCallback(m_handler, &ResponseHandler::ResetDevice),
      error);
}


//-----------------------------------------------------------------------------
// Util methods


bool RDMController::StringToStatusType(
    const string &arg,
    ola::rdm::rdm_status_type *status_type) {
  string lower_arg = arg;
  ola::ToLower(&lower_arg);
  if (arg == "none") {
    *status_type = ola::rdm::STATUS_NONE;
    return true;
  } else if (lower_arg == "error") {
    *status_type = ola::rdm::STATUS_ERROR;
    return true;
  } else if (lower_arg == "warning") {
    *status_type = ola::rdm::STATUS_WARNING;
    return true;
  } else if (lower_arg == "advisory") {
    *status_type = ola::rdm::STATUS_ADVISORY;
    return true;
  }
  return false;
}

bool RDMController::StringToLampState(const string &arg,
                                      ola::rdm::rdm_lamp_state *state) {
  string lower_arg = arg;
  ola::ToLower(&lower_arg);
  if (arg == "off") {
    *state = ola::rdm::LAMP_OFF;
    return true;
  } else if (lower_arg == "on") {
    *state = ola::rdm::LAMP_ON;
    return true;
  } else if (lower_arg == "strike") {
    *state = ola::rdm::LAMP_STRIKE;
    return true;
  } else if (lower_arg == "standby") {
    *state = ola::rdm::LAMP_STANDBY;
    return true;
  }
  return false;
}


bool RDMController::StringToLampMode(const string &arg,
                                     ola::rdm::rdm_lamp_mode *mode) {
  string lower_arg = arg;
  ola::ToLower(&lower_arg);
  if (arg == "off") {
    *mode = ola::rdm::LAMP_ON_MODE_OFF;
    return true;
  } else if (lower_arg == "dmx") {
    *mode = ola::rdm::LAMP_ON_MODE_DMX;
    return true;
  } else if (lower_arg == "on") {
    *mode = ola::rdm::LAMP_ON_MODE_ON;
    return true;
  } else if (lower_arg == "calibration") {
    *mode = ola::rdm::LAMP_ON_MODE_AFTER_CAL;
    return true;
  }
  return false;
}


bool RDMController::StringToOnOffAuto(const string &arg, uint8_t *mode) {
  string lower_arg = arg;
  ola::ToLower(&lower_arg);
  if (arg == "off") {
    *mode = 0;
    return true;
  } else if (lower_arg == "on") {
    *mode = 1;
    return true;
  } else if (lower_arg == "auto") {
    *mode = 2;
    return true;
  }
  return false;
}

bool RDMController::StringToOnOff(const string &arg, uint8_t *mode) {
  string lower_arg = arg;
  ola::ToLower(&lower_arg);
  if (arg == "off") {
    *mode = 0;
    return true;
  } else if (lower_arg == "on") {
    *mode = 1;
    return true;
  }
  return false;
}


bool RDMController::StringToWarmCold(const string &arg, uint8_t *mode) {
  string lower_arg = arg;
  ola::ToLower(&lower_arg);
  if (arg == "warm") {
    *mode = 1;
    return true;
  } else if (lower_arg == "cold") {
    *mode = 0xff;
    return true;
  }
  return false;
}


//-----------------------------------------------------------------------------
/*
 * A helper method to contruct pid_descriptors
 */
void RDMController::MakeDescriptor(uint16_t pid,
                                   CheckMethod get_verify,
                                   CheckMethod set_verify,
                                   ExecuteMethod get_execute,
                                   ExecuteMethod set_execute) {
  pid_descriptor d = {get_verify, set_verify, get_execute, set_execute};
  s_pid_map.insert(std::pair<uint16_t, const pid_descriptor>(
    pid, d));
}
