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

#include <time.h>
#include <ola/BaseTypes.h>
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


PidDescriptor::PidDescriptor(ExecuteMethod get_execute,
                               ExecuteMethod set_execute):
  m_get_execute(get_execute),
  m_set_execute(set_execute) {
}

PidDescriptor *PidDescriptor::AddGetVerify(CheckMethod method) {
  m_get_verify.push_back(method);
  return this;
}


PidDescriptor *PidDescriptor::AddSetVerify(CheckMethod method) {
  m_set_verify.push_back(method);
  return this;
}

bool PidDescriptor::Run(RDMController *controller,
                        const UID &uid,
                        uint16_t sub_device,
                        bool set,
                        uint16_t pid,
                        const vector<string> &params,
                        string *error) {
  vector<CheckMethod> &methods = set ? m_set_verify : m_get_verify;
  vector<CheckMethod>::const_iterator iter;
  for (iter = methods.begin(); iter != methods.end(); ++iter) {
    if (!(controller->*(*iter))(uid, sub_device, params, error))
      return false;
  }

  ExecuteMethod exec_method = set ? m_set_execute : m_get_execute;

  if (!exec_method) {
    *error = set ? "Set" : "Get";
    *error += " not permitted";
    return false;
  }
  return (controller->*exec_method)(uid, sub_device, params, error);
}


RDMController::~RDMController() {
  map<uint16_t, PidDescriptor*>::iterator iter;
  for (iter = m_pid_map.begin(); iter != m_pid_map.end(); ++iter) {
    delete iter->second;
  }
  m_pid_map.clear();
}


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
                               bool is_set,
                               uint16_t pid,
                               const vector<string> &params,
                               string *error) {
  map<uint16_t, PidDescriptor*>::const_iterator iter =
    m_pid_map.find(pid);

  if (iter == m_pid_map.end()) {
    *error = "Unknown PID";
    return false;
  }
  return iter->second->Run(this, uid, sub_device, is_set, pid, params, error);
}


/*
 * Populate the pid maps
 */
void RDMController::LoadMap() {
  if (m_pid_map.size())
    return;

  // populate the PID descriptor map
  MakeDescriptor(ola::rdm::PID_PROXIED_DEVICES,
                 &RDMController::GetProxiedDevices,
                 NULL)->AddGetVerify(
      &RDMController::NoArgsCheck)->AddGetVerify(
      &RDMController::RootDeviceCheck);

  MakeDescriptor(ola::rdm::PID_PROXIED_DEVICE_COUNT,
                 &RDMController::GetProxiedDeviceCount,
                 NULL)->AddGetVerify(
      &RDMController::NoArgsCheck)->AddGetVerify(
      &RDMController::RootDeviceCheck);

  MakeDescriptor(ola::rdm::PID_COMMS_STATUS,
                 &RDMController::GetCommStatus,
                 &RDMController::ClearCommStatus)->AddGetVerify(
      &RDMController::NoArgsCheck)->AddGetVerify(
      &RDMController::RootDeviceCheck)->AddSetVerify(
      &RDMController::NoArgsCheck)->AddSetVerify(
      &RDMController::RootDeviceCheck);

  MakeDescriptor(ola::rdm::PID_STATUS_MESSAGES,
                 &RDMController::GetStatusMessage,
                 NULL)->AddGetVerify(
      &RDMController::RootDeviceCheck);

  MakeDescriptor(ola::rdm::PID_STATUS_ID_DESCRIPTION,
                 &RDMController::GetStatusIdDescription,
                 NULL)->AddGetVerify(
      &RDMController::RootDeviceCheck);

  MakeDescriptor(ola::rdm::PID_CLEAR_STATUS_ID,
                 NULL,
                 &RDMController::ClearStatusId)->AddSetVerify(
      &RDMController::ValidBroadcastSubDeviceCheck);

  MakeDescriptor(ola::rdm::PID_SUB_DEVICE_STATUS_REPORT_THRESHOLD,
                 &RDMController::GetSubDeviceReporting,
                 &RDMController::SetSubDeviceReporting)->AddGetVerify(
      &RDMController::NoArgsCheck)->AddGetVerify(
      &RDMController::ValidSubDeviceCheck)->AddSetVerify(
      &RDMController::ValidBroadcastSubDeviceCheck);

  MakeDescriptor(ola::rdm::PID_SUPPORTED_PARAMETERS,
                 &RDMController::GetSupportedParameters,
                 NULL)->AddGetVerify(
      &RDMController::NoArgsCheck)->AddGetVerify(
      &RDMController::ValidSubDeviceCheck);

  MakeDescriptor(ola::rdm::PID_PARAMETER_DESCRIPTION,
                 &RDMController::GetParameterDescription,
                 NULL)->AddGetVerify(
      &RDMController::RootDeviceCheck);

  MakeDescriptor(ola::rdm::PID_DEVICE_INFO,
                 &RDMController::GetDeviceInfo,
                 NULL)->AddGetVerify(
      &RDMController::NoArgsCheck)->AddGetVerify(
      &RDMController::ValidSubDeviceCheck);

  MakeDescriptor(ola::rdm::PID_PRODUCT_DETAIL_ID_LIST,
                 &RDMController::GetProductDetailIdList,
                 NULL)->AddGetVerify(
      &RDMController::NoArgsCheck)->AddGetVerify(
      &RDMController::ValidSubDeviceCheck);

  MakeDescriptor(ola::rdm::PID_DEVICE_MODEL_DESCRIPTION,
                 &RDMController::GetDeviceModelDescription,
                 NULL)->AddGetVerify(
      &RDMController::NoArgsCheck)->AddGetVerify(
      &RDMController::ValidSubDeviceCheck);

  MakeDescriptor(ola::rdm::PID_MANUFACTURER_LABEL,
                 &RDMController::GetManufacturerLabel,
                 NULL)->AddGetVerify(
      &RDMController::NoArgsCheck)->AddGetVerify(
      &RDMController::ValidSubDeviceCheck);

  MakeDescriptor(ola::rdm::PID_DEVICE_LABEL,
                 &RDMController::GetDeviceLabel,
                 &RDMController::SetDeviceLabel)->AddGetVerify(
      &RDMController::NoArgsCheck)->AddGetVerify(
      &RDMController::ValidSubDeviceCheck)->AddSetVerify(
      &RDMController::ValidBroadcastSubDeviceCheck);

  // DONE UP TO HERE!!!

  MakeDescriptor(ola::rdm::PID_FACTORY_DEFAULTS,
                 &RDMController::GetFactoryDefaults,
                 &RDMController::ResetToFactoryDefaults)->AddGetVerify(
      &RDMController::NoArgsCheck)->AddGetVerify(
      &RDMController::ValidSubDeviceCheck)->AddSetVerify(
      &RDMController::NoArgsCheck)->AddSetVerify(
      &RDMController::ValidBroadcastSubDeviceCheck);

  MakeDescriptor(ola::rdm::PID_LANGUAGE_CAPABILITIES,
                 &RDMController::GetLanguageCapabilities,
                  NULL)->AddGetVerify(
      &RDMController::NoArgsCheck)->AddGetVerify(
      &RDMController::ValidSubDeviceCheck);

  MakeDescriptor(ola::rdm::PID_LANGUAGE,
                 &RDMController::GetLanguage,
                 &RDMController::SetLanguage)->AddGetVerify(
      &RDMController::NoArgsCheck)->AddGetVerify(
      &RDMController::ValidSubDeviceCheck)->AddSetVerify(
      &RDMController::ValidBroadcastSubDeviceCheck);

  MakeDescriptor(ola::rdm::PID_SOFTWARE_VERSION_LABEL,
                 &RDMController::GetSoftwareVersionLabel,
                 NULL)->AddGetVerify(
      &RDMController::NoArgsCheck)->AddGetVerify(
      &RDMController::ValidSubDeviceCheck);

  MakeDescriptor(ola::rdm::PID_BOOT_SOFTWARE_VERSION_ID,
                 &RDMController::GetBootSoftwareVersion,
                 NULL)->AddGetVerify(
      &RDMController::NoArgsCheck)->AddGetVerify(
      &RDMController::ValidSubDeviceCheck);

  MakeDescriptor(ola::rdm::PID_BOOT_SOFTWARE_VERSION_LABEL,
                 &RDMController::GetBootSoftwareVersionLabel,
                 NULL)->AddGetVerify(
      &RDMController::NoArgsCheck)->AddGetVerify(
      &RDMController::ValidSubDeviceCheck);

  MakeDescriptor(ola::rdm::PID_DMX_PERSONALITY,
                 &RDMController::GetDMXPersonality,
                 &RDMController::SetDMXPersonality)->AddGetVerify(
      &RDMController::NoArgsCheck)->AddGetVerify(
      &RDMController::ValidSubDeviceCheck)->AddSetVerify(
      &RDMController::ValidBroadcastSubDeviceCheck);


  MakeDescriptor(ola::rdm::PID_DMX_PERSONALITY_DESCRIPTION,
                 &RDMController::GetDMXPersonalityDescription,
                 NULL)->AddGetVerify(
      &RDMController::ValidSubDeviceCheck);

  MakeDescriptor(ola::rdm::PID_DMX_START_ADDRESS,
                 &RDMController::GetDMXAddress,
                 &RDMController::SetDMXAddress)->AddGetVerify(
      &RDMController::NoArgsCheck)->AddGetVerify(
      &RDMController::ValidSubDeviceCheck)->AddSetVerify(
      &RDMController::ValidBroadcastSubDeviceCheck);

  MakeDescriptor(ola::rdm::PID_SLOT_INFO,
                 &RDMController::GetSlotInfo,
                 NULL)->AddGetVerify(
      &RDMController::NoArgsCheck)->AddGetVerify(
      &RDMController::ValidSubDeviceCheck);

  MakeDescriptor(ola::rdm::PID_SLOT_DESCRIPTION,
                 &RDMController::GetSlotDescription,
                 NULL)->AddGetVerify(
      &RDMController::ValidSubDeviceCheck);

  MakeDescriptor(ola::rdm::PID_DEFAULT_SLOT_VALUE,
                 &RDMController::GetSlotDefaultValues,
                 NULL)->AddGetVerify(
      &RDMController::ValidSubDeviceCheck);

  MakeDescriptor(ola::rdm::PID_SENSOR_DEFINITION,
                 &RDMController::GetSensorDefinition,
                 NULL)->AddGetVerify(
      &RDMController::ValidSubDeviceCheck);

  MakeDescriptor(ola::rdm::PID_SENSOR_VALUE,
                 &RDMController::GetSensorValue,
                 &RDMController::SetSensorValue)->AddGetVerify(
      &RDMController::ValidSubDeviceCheck)->AddSetVerify(
      &RDMController::ValidBroadcastSubDeviceCheck);

  MakeDescriptor(ola::rdm::PID_RECORD_SENSORS,
                 NULL,
                 &RDMController::RecordSensors)->AddSetVerify(
      &RDMController::ValidBroadcastSubDeviceCheck);

  MakeDescriptor(ola::rdm::PID_DEVICE_HOURS,
    &RDMController::GetDeviceHours,
    &RDMController::SetDeviceHours)->AddGetVerify(
      &RDMController::NoArgsCheck)->AddGetVerify(
      &RDMController::ValidSubDeviceCheck)->AddSetVerify(
      &RDMController::ValidBroadcastSubDeviceCheck);

  MakeDescriptor(ola::rdm::PID_LAMP_HOURS,
                 &RDMController::GetLampHours,
                 &RDMController::SetLampHours)->AddGetVerify(
      &RDMController::NoArgsCheck)->AddGetVerify(
      &RDMController::ValidSubDeviceCheck)->AddSetVerify(
      &RDMController::ValidBroadcastSubDeviceCheck);

  MakeDescriptor(ola::rdm::PID_LAMP_STRIKES,
                 &RDMController::GetLampStrikes,
                 &RDMController::SetLampStrikes)->AddGetVerify(
      &RDMController::NoArgsCheck)->AddGetVerify(
      &RDMController::ValidSubDeviceCheck)->AddSetVerify(
      &RDMController::ValidBroadcastSubDeviceCheck);

  MakeDescriptor(ola::rdm::PID_LAMP_STATE,
                 &RDMController::GetLampState,
                 &RDMController::SetLampState)->AddGetVerify(
      &RDMController::NoArgsCheck)->AddGetVerify(
      &RDMController::ValidSubDeviceCheck)->AddSetVerify(
      &RDMController::ValidBroadcastSubDeviceCheck);

  MakeDescriptor(ola::rdm::PID_LAMP_ON_MODE,
                 &RDMController::GetLampMode,
                 &RDMController::SetLampMode)->AddGetVerify(
      &RDMController::NoArgsCheck)->AddGetVerify(
      &RDMController::ValidSubDeviceCheck)->AddSetVerify(
      &RDMController::ValidBroadcastSubDeviceCheck);

  MakeDescriptor(ola::rdm::PID_DEVICE_POWER_CYCLES,
                 &RDMController::GetDevicePowerCycles,
                 &RDMController::SetDevicePowerCycles)->AddGetVerify(
      &RDMController::NoArgsCheck)->AddGetVerify(
      &RDMController::ValidSubDeviceCheck)->AddSetVerify(
      &RDMController::ValidBroadcastSubDeviceCheck);

  // done up to here
  MakeDescriptor(ola::rdm::PID_DISPLAY_INVERT,
                 &RDMController::GetDisplayInvert,
                 &RDMController::SetDisplayInvert)->AddGetVerify(
      &RDMController::NoArgsCheck)->AddGetVerify(
      &RDMController::ValidSubDeviceCheck)->AddSetVerify(
      &RDMController::ValidBroadcastSubDeviceCheck);

  MakeDescriptor(ola::rdm::PID_DISPLAY_LEVEL,
                 &RDMController::GetDisplayLevel,
                 &RDMController::SetDisplayLevel)->AddGetVerify(
      &RDMController::NoArgsCheck)->AddGetVerify(
      &RDMController::ValidSubDeviceCheck)->AddSetVerify(
      &RDMController::ValidBroadcastSubDeviceCheck);

  MakeDescriptor(ola::rdm::PID_PAN_INVERT,
                 &RDMController::GetPanInvert,
                 &RDMController::SetPanInvert)->AddGetVerify(
      &RDMController::NoArgsCheck)->AddGetVerify(
      &RDMController::ValidSubDeviceCheck)->AddSetVerify(
      &RDMController::ValidBroadcastSubDeviceCheck);

  MakeDescriptor(ola::rdm::PID_TILT_INVERT,
                 &RDMController::GetTiltInvert,
                 &RDMController::SetTiltInvert)->AddGetVerify(
      &RDMController::NoArgsCheck)->AddGetVerify(
      &RDMController::ValidSubDeviceCheck)->AddSetVerify(
      &RDMController::ValidBroadcastSubDeviceCheck);

  MakeDescriptor(ola::rdm::PID_PAN_TILT_SWAP,
                 &RDMController::GetPanTiltSwap,
                 &RDMController::SetPanTiltSwap)->AddGetVerify(
      &RDMController::NoArgsCheck)->AddGetVerify(
      &RDMController::ValidSubDeviceCheck)->AddSetVerify(
      &RDMController::ValidBroadcastSubDeviceCheck);

  MakeDescriptor(ola::rdm::PID_REAL_TIME_CLOCK,
                 &RDMController::GetClock,
                 &RDMController::SetClock)->AddGetVerify(
      &RDMController::NoArgsCheck)->AddGetVerify(
      &RDMController::ValidSubDeviceCheck)->AddSetVerify(
      &RDMController::ValidBroadcastSubDeviceCheck);

  MakeDescriptor(ola::rdm::PID_IDENTIFY_DEVICE,
                 &RDMController::GetIdentifyMode,
                 &RDMController::IdentifyDevice)->AddGetVerify(
      &RDMController::NoArgsCheck)->AddGetVerify(
      &RDMController::ValidSubDeviceCheck)->AddSetVerify(
      &RDMController::ValidBroadcastSubDeviceCheck);

  MakeDescriptor(ola::rdm::PID_RESET_DEVICE,
                 NULL,
                 &RDMController::ResetDevice)->AddSetVerify(
      &RDMController::ValidBroadcastSubDeviceCheck);
}


//-----------------------------------------------------------------------------
PidDescriptor *RDMController::MakeDescriptor(uint16_t pid,
                                             ExecuteMethod get_execute,
                                             ExecuteMethod set_execute) {
  PidDescriptor *d = new PidDescriptor(get_execute, set_execute);
  m_pid_map.insert(std::pair<uint16_t, PidDescriptor*>(pid, d));
  return d;
}

//-----------------------------------------------------------------------------

// First up are generic check methods


/*
 * Check that the sub device is 0
 */
bool RDMController::RootDeviceCheck(const UID &uid,
                                    uint16_t sub_device,
                                    const vector<string> &args,
                                    string *error) {
  if (sub_device) {
    *error = "Sub device must be 0 (root device)";
    return false;
  }
  return true;
}


/*
 * Check the sub device is valid
 */
bool RDMController::ValidSubDeviceCheck(const UID &uid,
                                        uint16_t sub_device,
                                        const vector<string> &args,
                                        string *error) {
  if (sub_device > ola::rdm::MAX_SUBDEVICE_NUMBER) {
    std::stringstream str;
    str << "Sub device must be <= " << ola::rdm::MAX_SUBDEVICE_NUMBER;
    *error = str.str();
    return false;
  }
  return true;
}


/*
 * Check the sub device is valid or broadcast
 */
bool RDMController::ValidBroadcastSubDeviceCheck(
    const UID &uid,
    uint16_t sub_device,
    const vector<string> &args,
    string *error) {
  if (sub_device > ola::rdm::MAX_SUBDEVICE_NUMBER &&
      sub_device != ola::rdm::ALL_RDM_SUBDEVICES) {
    std::stringstream str;
    str << "Sub device must be <= " << ola::rdm::MAX_SUBDEVICE_NUMBER <<
      " or " << ola::rdm::ALL_RDM_SUBDEVICES << " (all subdevices)";
    *error = str.str();
    return false;
  }
  return true;
}



/*
 * Check that no arguments were supplied
 */
bool RDMController::NoArgsCheck(const UID &uid,
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
  if (args.size() != 1 || (!StringToStatusType(args[0], &status_type))) {
    *error = "arg must be one of {none, last, error, warning, advisory}";
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
  if (!CheckForUInt16(&status_id, error, args))
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
  if (args.size() != 1 || (!StringToStatusType(args[0], &status_type))) {
    *error = "arg must be one of {none, error, warning, advisory}";
    return false;
  }
  if (status_type == ola::rdm::STATUS_GET_LAST_MESSAGE) {
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
  if (!CheckForUInt16(&pid, error, args))
    return false;
  std::cout << pid << std::endl;
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
  if (args.size() != 1) {
    *error = "Argument must be supplied, the first 32 characters will be used";
    return false;
  }
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
  if (args.size() != 1 || args[0].size() != 2) {
    *error = "Argument must be a 2 char string";
    return false;
  }
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
  if (args.size() != 1 || (!ola::StringToUInt8(args[0], &personality))) {
    *error = "Argument must be an integer between 1 and 255";
    return false;
  }
  if (personality == 0) {
    *error = "Personality can't be 0";
    return false;
  }
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
  if (args.size() != 1 || (!ola::StringToUInt8(args[0], &personality))) {
    *error = "Argument must be an integer between 1 and 255";
    return false;
  }
  if (personality == 0) {
    *error = "Personality can't be 0";
    return false;
  }
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
  if (args.size() != 1 || (!ola::StringToUInt16(args[0], &dmx_address))) {
    *error = "Argument must be a uint16";
    return false;
  }
  if (dmx_address == 0 || dmx_address > DMX_UNIVERSE_SIZE) {
    *error = "Dmx address must be between 1 and 512";
    return false;
  }
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
  if (!CheckForUInt16(&slot_id, error, args))
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
  if (args.size() != 1 || (!ola::StringToUInt8(args[0], &sensor))) {
    *error = "Argument must be a uint8";
    return false;
  }
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
  if (args.size() != 1 || (!ola::StringToUInt8(args[0], &sensor))) {
    *error = "Argument must be a uint8";
    return false;
  }
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
  if (args.size() != 1 || (!ola::StringToUInt8(args[0], &sensor))) {
    *error = "Argument must be a uint8";
    return false;
  }
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
  if (args.size() != 1 || (!ola::StringToUInt8(args[0], &sensor))) {
    *error = "Argument must be a uint8";
    return false;
  }
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
  if (args.size() != 1 || (!ola::StringToUInt(args[0], &device_hours))) {
    *error = "Argument must be a uint32";
    return false;
  }
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
  if (args.size() != 1 || (!ola::StringToUInt(args[0], &lamp_hours))) {
    *error = "Argument must be a uint32";
    return false;
  }
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
  if (args.size() != 1 || (!ola::StringToUInt(args[0], &lamp_strikes))) {
    *error = "Argument must be a uint32";
    return false;
  }
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
  if (args.size() != 1 || (!StringToLampState(args[0], &state))) {
    *error = "Argument must be one of {off, on, strike, standby}";
    return false;
  }
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
  if (args.size() != 1 || (!StringToLampMode(args[0], &mode))) {
    *error = "Argument must be one of {off, dmx, on, calibration}";
    return false;
  }
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
  if (args.size() != 1 || (!ola::StringToUInt(args[0], &power_cycles))) {
    *error = " Argument must be a uint32";
    return false;
  }
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
  if (args.size() != 1 || (!StringToOnOffAuto(args[0], &mode))) {
    *error = "Argument must be one of {on, off, auto}";
    return false;
  }
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
  if (args.size() != 1 || (!ola::StringToUInt8(args[0], &level))) {
    *error = "Argument must be a uint8";
    return false;
  }
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
  if (args.size() != 1 || (!StringToOnOff(args[0], &mode))) {
    *error = "Argument must be on of {on, off}";
    return false;
  }
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
  if (args.size() != 1 || (!StringToOnOff(args[0], &mode))) {
    *error = "Argument must be on of {on, off}";
    return false;
  }
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
  if (args.size() != 1 || (!StringToOnOff(args[0], &mode))) {
    *error = "Argument must be on of {on, off}";
    return false;
  }
  return m_api->SetPanTiltSwap(
      uid,
      sub_device,
      mode,
      ola::NewSingleCallback(m_handler, &ResponseHandler::SetPanTiltSwap),
      error);
}


bool RDMController::GetClock(const UID &uid,
                             uint16_t sub_device,
                             const vector<string> &args,
                             string *error) {
  return m_api->GetClock(
      uid,
      sub_device,
      ola::NewSingleCallback(m_handler, &ResponseHandler::Clock),
      error);
}


bool RDMController::SetClock(const UID &uid,
                             uint16_t sub_device,
                             const vector<string> &args,
                             string *error) {
  const string invalid_format_error =
    "Requires an argument in the form YYYY-MM-DD hh:mm:ss";
  if (args.size() != 1) {
    *error = invalid_format_error;
    return false;
  }
  struct tm time_spec;
  char *result = strptime(args[0].data(), "%Y-%m-%d %H:%M:%S", &time_spec);
  if (!result) {
    *error = invalid_format_error;
    return false;
  }
  ola::rdm::ClockValue clock;
  clock.year = time_spec.tm_year;
  clock.month = time_spec.tm_mon;
  clock.day = time_spec.tm_mday;
  clock.hour = time_spec.tm_hour;
  clock.minute = time_spec.tm_min;
  clock.second = time_spec.tm_sec;
  return m_api->SetClock(
      uid,
      sub_device,
      clock,
      ola::NewSingleCallback(m_handler, &ResponseHandler::SetClock),
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
  if (args.size() != 1 || (!StringToOnOff(args[0], &mode))) {
    *error = "Argument must be on of {on, off}";
    return false;
  }
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
  if (args.size() != 1 || (!StringToWarmCold(args[0], &mode))) {
    *error = "Argument must be on of {warm, cold}";
    return false;
  }
  return m_api->ResetDevice(
      uid,
      sub_device,
      mode,
      ola::NewSingleCallback(m_handler, &ResponseHandler::ResetDevice),
      error);
}


//-----------------------------------------------------------------------------
// Util methods

bool RDMController::CheckForUInt16(uint16_t *value,
                                   string *error,
                                   const vector <string> &args) {
  const string error_message =
    "Argument must be a value between 0 - 65535.  Use 0x for hex values";
  const string hex_prefix = "0x";
  if (args.size() == 1) {
    if (hex_prefix == args[0].substr(0, hex_prefix.size())) {
      const string digit = args[0].substr(hex_prefix.size(),
                                          args[0].size() - hex_prefix.size());
      if (ola::HexStringToUInt16(digit, value)) {
        return true;
      } else {
        *error = error_message;
        return false;
      }
    } else if (ola::StringToUInt16(args[0], value)) {
      return true;
    }
  }
  *error = error_message;
  return false;
}


bool RDMController::StringToStatusType(
    const string &arg,
    ola::rdm::rdm_status_type *status_type) {
  string lower_arg = arg;
  ola::ToLower(&lower_arg);
  if (arg == "none") {
    *status_type = ola::rdm::STATUS_NONE;
    return true;
  } else if (lower_arg == "last") {
    *status_type = ola::rdm::STATUS_GET_LAST_MESSAGE;
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


