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
 *  RDMController.h
 *  A command line based RDM controller
 *  Copyright (C) 2010 Simon Newton
 */

#ifndef SRC_RDMCONTROLLER_H_
#define SRC_RDMCONTROLLER_H_

#include <ola/rdm/RDMAPI.h>
#include <ola/rdm/UID.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "src/RDMHandler.h"

using std::map;
using std::string;
using std::vector;
using ola::rdm::UID;


class RDMController;

typedef bool (RDMController::*CheckMethod)(const UID &uid,
                                           uint16_t sub_device,
                                           const vector<string> &args,
                                           string *error);
typedef bool (RDMController::*ExecuteMethod)(const UID &uid,
                                             uint16_t sub_device,
                                             const vector<string> &args,
                                             string *error);
class PidDescriptor {
  public:
    PidDescriptor(ExecuteMethod get_execute, ExecuteMethod set_execute);
    PidDescriptor *AddGetVerify(CheckMethod method);
    PidDescriptor *AddSetVerify(CheckMethod method);

    bool Run(RDMController *controller,
             const UID &uid,
             uint16_t sub_device,
             bool set,
             uint16_t pid,
             const vector<string> &params,
             string *error);

  private:
    vector<CheckMethod> m_get_verify;
    vector<CheckMethod> m_set_verify;
    ExecuteMethod m_get_execute;
    ExecuteMethod m_set_execute;
};

class RDMController {
  public:
    RDMController(ola::rdm::RDMAPI *api, ResponseHandler *handler):
      m_api(api),
      m_handler(handler) {
      RDMController::LoadMap();
    }
    ~RDMController();

    bool RequestPID(const UID &uid,
                    uint16_t sub_device,
                    bool set,
                    uint16_t pid,
                    const vector<string> &params,
                    string *error);

  private:
    map<uint16_t, PidDescriptor*> m_pid_map;
    ola::rdm::RDMAPI *m_api;
    ResponseHandler *m_handler;

    void LoadMap();
    PidDescriptor* MakeDescriptor(uint16_t pid,
                                  ExecuteMethod get_execute,
                                  ExecuteMethod set_execute);

    // generic methods
    bool RootDeviceCheck(const UID &uid,
                         uint16_t sub_device,
                         const vector<string> &args,
                         string *error);
    bool ValidSubDeviceCheck(const UID &uid,
                             uint16_t sub_device,
                             const vector<string> &args,
                             string *error);
    bool ValidBroadcastSubDeviceCheck(const UID &uid,
                                      uint16_t sub_device,
                                      const vector<string> &args,
                                      string *error);
    bool NoArgsCheck(const UID &uid,
                     uint16_t sub_device,
                     const vector<string> &args,
                     string *error);


    bool NoArgsRootDeviceSetCheck(const UID &uid,
                                  uint16_t sub_device,
                                  const vector<string> &args,
                                  string *error);

    // pid specific methods
    bool GetProxiedDeviceCount(const UID &uid,
                               uint16_t sub_device,
                               const vector<string> &args,
                               string *error);

    bool GetProxiedDevices(const UID &uid,
                           uint16_t sub_device,
                           const vector<string> &args,
                           string *error);

    bool GetCommStatus(const UID &uid,
                       uint16_t sub_device,
                       const vector<string> &args,
                       string *error);

    bool ClearCommStatus(const UID &uid,
                         uint16_t sub_device,
                         const vector<string> &args,
                         string *error);

    bool GetStatusMessage(const UID &uid,
                          uint16_t sub_device,
                          const vector<string> &args,
                          string *error);

    bool GetStatusIdDescription(const UID &uid,
                                uint16_t sub_device,
                                const vector<string> &args,
                                string *error);

    bool ClearStatusId(const UID &uid,
                       uint16_t sub_device,
                       const vector<string> &args,
                       string *error);

    bool GetSubDeviceReporting(const UID &uid,
                               uint16_t sub_device,
                               const vector<string> &args,
                               string *error);

    bool SetSubDeviceReporting(const UID &uid,
                               uint16_t sub_device,
                               const vector<string> &args,
                               string *error);

    bool GetSupportedParameters(const UID &uid,
                                uint16_t sub_device,
                                const vector<string> &args,
                                string *error);

    bool GetParameterDescription(const UID &uid,
                                 uint16_t sub_device,
                                 const vector<string> &args,
                                 string *error);

    bool GetDeviceInfo(const UID &uid,
                       uint16_t sub_device,
                       const vector<string> &args,
                       string *error);

    bool GetProductDetailIdList(const UID &uid,
                                uint16_t sub_device,
                                const vector<string> &args,
                                string *error);

    bool GetDeviceModelDescription(const UID &uid,
                                   uint16_t sub_device,
                                   const vector<string> &args,
                                   string *error);

    bool GetManufacturerLabel(const UID &uid,
                              uint16_t sub_device,
                              const vector<string> &args,
                              string *error);

    bool GetDeviceLabel(const UID &uid,
                        uint16_t sub_device,
                        const vector<string> &args,
                        string *error);

    bool SetDeviceLabel(const UID &uid,
                        uint16_t sub_device,
                        const vector<string> &args,
                        string *error);

    bool GetFactoryDefaults(const UID &uid,
                            uint16_t sub_device,
                            const vector<string> &args,
                            string *error);

    bool ResetToFactoryDefaults(const UID &uid,
                                uint16_t sub_device,
                                const vector<string> &args,
                                string *error);

    bool GetLanguageCapabilities(const UID &uid,
                                 uint16_t sub_device,
                                 const vector<string> &args,
                                 string *error);

    bool GetLanguage(const UID &uid,
                     uint16_t sub_device,
                     const vector<string> &args,
                     string *error);

    bool SetLanguage(const UID &uid,
                     uint16_t sub_device,
                     const vector<string> &args,
                     string *error);

    bool GetSoftwareVersionLabel(const UID &uid,
                                 uint16_t sub_device,
                                 const vector<string> &args,
                                 string *error);

    bool GetBootSoftwareVersion(const UID &uid,
                                uint16_t sub_device,
                                const vector<string> &args,
                                string *error);

    bool GetBootSoftwareVersionLabel(const UID &uid,
                                     uint16_t sub_device,
                                     const vector<string> &args,
                                     string *error);

    bool GetDMXPersonality(const UID &uid,
                           uint16_t sub_device,
                           const vector<string> &args,
                           string *error);

    bool SetDMXPersonality(const UID &uid,
                           uint16_t sub_device,
                           const vector<string> &args,
                           string *error);

    bool GetDMXPersonalityDescription(const UID &uid,
                                      uint16_t sub_device,
                                      const vector<string> &args,
                                      string *error);

    bool GetDMXAddress(const UID &uid,
                       uint16_t sub_device,
                       const vector<string> &args,
                       string *error);

    bool SetDMXAddress(const UID &uid,
                       uint16_t sub_device,
                       const vector<string> &args,
                       string *error);

    bool GetSlotInfo(const UID &uid,
                     uint16_t sub_device,
                     const vector<string> &args,
                     string *error);

    bool GetSlotDescription(const UID &uid,
                            uint16_t sub_device,
                            const vector<string> &args,
                            string *error);

    bool GetSlotDefaultValues(const UID &uid,
                              uint16_t sub_device,
                              const vector<string> &args,
                              string *error);

    bool GetSensorDefinition(const UID &uid,
                             uint16_t sub_device,
                             const vector<string> &args,
                             string *error);

    bool GetSensorValue(const UID &uid,
                        uint16_t sub_device,
                        const vector<string> &args,
                        string *error);

    bool SetSensorValue(const UID &uid,
                        uint16_t sub_device,
                        const vector<string> &args,
                        string *error);

    bool RecordSensors(const UID &uid,
                       uint16_t sub_device,
                       const vector<string> &args,
                       string *error);

    bool GetDeviceHours(const UID &uid,
                        uint16_t sub_device,
                        const vector<string> &args,
                        string *error);

    bool SetDeviceHours(const UID &uid,
                        uint16_t sub_device,
                        const vector<string> &args,
                        string *error);

    bool GetLampHours(const UID &uid,
                      uint16_t sub_device,
                      const vector<string> &args,
                      string *error);

    bool SetLampHours(const UID &uid,
                      uint16_t sub_device,
                      const vector<string> &args,
                      string *error);

    bool GetLampStrikes(const UID &uid,
                        uint16_t sub_device,
                        const vector<string> &args,
                        string *error);

    bool SetLampStrikes(const UID &uid,
                        uint16_t sub_device,
                        const vector<string> &args,
                        string *error);

    bool GetLampState(const UID &uid,
                      uint16_t sub_device,
                      const vector<string> &args,
                      string *error);

    bool SetLampState(const UID &uid,
                      uint16_t sub_device,
                      const vector<string> &args,
                      string *error);

    bool GetLampMode(const UID &uid,
                     uint16_t sub_device,
                     const vector<string> &args,
                     string *error);

    bool SetLampMode(const UID &uid,
                     uint16_t sub_device,
                     const vector<string> &args,
                     string *error);

    bool GetDevicePowerCycles(const UID &uid,
                              uint16_t sub_device,
                              const vector<string> &args,
                              string *error);

    bool SetDevicePowerCycles(const UID &uid,
                              uint16_t sub_device,
                              const vector<string> &args,
                              string *error);

    bool GetDisplayInvert(const UID &uid,
                          uint16_t sub_device,
                          const vector<string> &args,
                          string *error);

    bool SetDisplayInvert(const UID &uid,
                          uint16_t sub_device,
                          const vector<string> &args,
                          string *error);

    bool GetDisplayLevel(const UID &uid,
                         uint16_t sub_device,
                         const vector<string> &args,
                         string *error);

    bool SetDisplayLevel(const UID &uid,
                         uint16_t sub_device,
                         const vector<string> &args,
                         string *error);

    bool GetPanInvert(const UID &uid,
                      uint16_t sub_device,
                      const vector<string> &args,
                      string *error);

    bool SetPanInvert(const UID &uid,
                      uint16_t sub_device,
                      const vector<string> &args,
                      string *error);

    bool GetTiltInvert(const UID &uid,
                       uint16_t sub_device,
                       const vector<string> &args,
                       string *error);

    bool SetTiltInvert(const UID &uid,
                       uint16_t sub_device,
                       const vector<string> &args,
                       string *error);

    bool GetPanTiltSwap(const UID &uid,
                        uint16_t sub_device,
                        const vector<string> &args,
                        string *error);

    bool SetPanTiltSwap(const UID &uid,
                        uint16_t sub_device,
                        const vector<string> &args,
                        string *error);

    bool GetClock(const UID &uid,
                  uint16_t sub_device,
                  const vector<string> &args,
                  string *error);

    bool SetClock(const UID &uid,
                  uint16_t sub_device,
                  const vector<string> &args,
                  string *error);

    bool GetIdentifyMode(const UID &uid,
                         uint16_t sub_device,
                         const vector<string> &args,
                         string *error);

    bool IdentifyDevice(const UID &uid,
                        uint16_t sub_device,
                        const vector<string> &args,
                        string *error);

    bool ResetDevice(const UID &uid,
                     uint16_t sub_device,
                     const vector<string> &args,
                     string *error);

    // util methods
    bool CheckForUInt16(uint16_t *value,
                        string *error,
                        const vector <string> &args);
    bool StringToStatusType(const string &arg,
                            ola::rdm::rdm_status_type *status_type);
    bool StringToLampState(const string &arg,
                           ola::rdm::rdm_lamp_state *state);
    bool StringToLampMode(const string &arg,
                          ola::rdm::rdm_lamp_mode *mode);
    bool StringToOnOffAuto(const string &arg, uint8_t *mode);
    bool StringToOnOff(const string &arg, uint8_t *mode);
    bool StringToWarmCold(const string &arg, uint8_t *mode);
};
#endif  // SRC_RDMCONTROLLER_H_
