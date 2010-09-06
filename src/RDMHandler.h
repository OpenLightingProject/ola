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
 *  RDMHandler.h
 *  The interface for a RDM Handler class.
 *  This class defines the actions to take for RDM responses.
 *  Copyright (C) 2010 Simon Newton
 */

#ifndef SRC_RDMHANDLER_H_
#define SRC_RDMHANDLER_H_

#include <ola/Callback.h>
#include <ola/network/SelectServer.h>
#include <ola/rdm/RDMAPI.h>
#include <ola/rdm/UID.h>

#include <string>
#include <vector>
#include <map>

using ola::network::SelectServer;
using ola::rdm::RDMAPI;
using ola::rdm::ResponseStatus;
using ola::rdm::UID;
using std::map;
using std::string;
using std::vector;

class ResponseHandler: public ola::rdm::QueuedMessageHandler {
  public:
    ResponseHandler(RDMAPI *api, SelectServer *ss,
                    const map<uint16_t, string> &pid_to_name_map);

    void ProxiedDeviceCount(const ResponseStatus &status,
                            uint16_t device_count,
                            bool list_changed);
    void ProxiedDevices(const ResponseStatus &status,
                        const vector<UID> &uids);
    void CommStatus(const ResponseStatus &status,
                    uint16_t short_message,
                    uint16_t length_mismatch,
                    uint16_t checksum_fail);
    void StatusMessages(const ResponseStatus &status,
                        const vector<ola::rdm::StatusMessage> &messages);
    void StatusIdDescription(const ResponseStatus &status,
                             const string &status_id);
    void SubDeviceReporting(const ResponseStatus &status,
                            uint8_t status_type);
    void SupportedParameters(const ResponseStatus &status,
                             const vector<uint16_t> &parameters);
    void ParameterDescription(
        const ResponseStatus &status,
        const ola::rdm::ParameterDescriptor &description);
    void DeviceInfo(const ResponseStatus &status,
                    const ola::rdm::DeviceDescriptor &device_info);
    void ProductDetailIdList(const ResponseStatus &status,
                             const vector<uint16_t> &ids);
    void DeviceModelDescription(const ResponseStatus &status,
                                const string &description);
    void ManufacturerLabel(const ResponseStatus &status,
                           const string &label);
    void DeviceLabel(const ResponseStatus &status,
                     const string &label);
    void FactoryDefaults(const ResponseStatus &status,
                         bool using_defaults);
    void LanguageCapabilities(const ResponseStatus &status,
                              const vector<string> &langs);
    void Language(const ResponseStatus &status,
                  const string &language);
    void SoftwareVersionLabel(const ResponseStatus &status,
                              const string &label);
    void BootSoftwareVersion(const ResponseStatus &status,
                             uint32_t version);
    void BootSoftwareVersionLabel(const ResponseStatus &status,
                                  const string &label);
    void DMXPersonality(const ResponseStatus &status,
                        uint8_t current_personality,
                        uint8_t personality_count);
    void DMXPersonalityDescription(const ResponseStatus &status,
                                   uint8_t personality,
                                   uint16_t slots_requires,
                                   const string &label);
    void DMXAddress(const ResponseStatus &status,
                    uint16_t start_address);
    void SlotInfo(const ResponseStatus &status,
                  const vector<ola::rdm::SlotDescriptor> &slots);
    void SlotDescription(const ResponseStatus &status,
                         uint16_t slot_offset,
                         const string &description);
    void SlotDefaultValues(const ResponseStatus &status,
                           const vector<ola::rdm::SlotDefault> &defaults);
    void SensorDefinition(const ResponseStatus &status,
                          const ola::rdm::SensorDescriptor &descriptor);
    void SensorValue(const ResponseStatus &status,
                     const ola::rdm::SensorValueDescriptor &descriptor);
    void DeviceHours(const ResponseStatus &status, uint32_t hours);
    void LampHours(const ResponseStatus &status, uint32_t hours);
    void LampStrikes(const ResponseStatus &status, uint32_t hours);
    void LampState(const ResponseStatus &status, uint8_t state);
    void LampMode(const ResponseStatus &status, uint8_t mode);
    void DevicePowerCycles(const ResponseStatus &status, uint32_t hours);
    void DisplayInvert(const ResponseStatus &status, uint8_t invert_mode);
    void DisplayLevel(const ResponseStatus &status, uint8_t level);
    void PanInvert(const ResponseStatus &status, uint8_t inverted);
    void TiltInvert(const ResponseStatus &status, uint8_t inverted);
    void PanTiltSwap(const ResponseStatus &status, uint8_t inverted);
    void Clock(const ResponseStatus &status,
               const ola::rdm::ClockValue &clock);
    void IdentifyMode(const ResponseStatus &status, bool mode);


    // Define these here so we can use the same object to handle non-queued
    // messages
    void ClearCommStatus(const ResponseStatus &status);
    void ClearStatusId(const ResponseStatus &status);
    void SetSubDeviceReporting(const ResponseStatus &status);
    void SetDeviceLabel(const ResponseStatus &status);
    void ResetToFactoryDefaults(const ResponseStatus &status);
    void SetLanguage(const ResponseStatus &status);
    void SetDMXPersonality(const ResponseStatus &status);
    void SetDMXAddress(const ResponseStatus &status);
    void SetSensorValue(const ResponseStatus &status,
                        const ola::rdm::SensorValueDescriptor &descriptor);
    void RecordSensors(const ResponseStatus &status);
    void SetDeviceHours(const ResponseStatus &status);
    void SetLampHours(const ResponseStatus &status);
    void SetLampStrikes(const ResponseStatus &status);
    void SetDevicePowerCycles(const ResponseStatus &status);
    void SetLampState(const ResponseStatus &status);
    void SetLampMode(const ResponseStatus &status);
    void SetDisplayInvert(const ResponseStatus &status);
    void SetDisplayLevel(const ResponseStatus &status);
    void SetPanInvert(const ResponseStatus &status);
    void SetPanTiltSwap(const ResponseStatus &status);
    void SetTiltInvert(const ResponseStatus &status);
    void SetClock(const ResponseStatus &status);
    void IdentifyDevice(const ResponseStatus &status);
    void ResetDevice(const ResponseStatus &status);


    // this returns the exit code once all is said and done
    uint8_t ExitCode() const { return m_exit_code; }

  private:
    RDMAPI *m_api;
    SelectServer *m_ss;
    uint8_t m_exit_code;
    map<uint16_t, string> m_pid_to_name_map;

    bool CheckForSuccess(const ResponseStatus &status);
    void PrintDataType(uint8_t type);
    void PrintType(uint8_t type);
    void PrintUnit(uint8_t unit);
    void PrintPrefix(uint8_t prefix);
    void PrintProductCategory(uint16_t category);
    void PrintSlotInfo(uint8_t slot_type, uint16_t slot_label);
    void PrintStatusType(uint8_t status_type);
    void PrintStatusMessageId(uint16_t message_id);
    void PrintLampState(uint8_t lamp_state);
    void PrintLampMode(uint8_t lamp_mode);
    void PrintNackReason(uint16_t reason);
    void PrintProductDetail(uint16_t detail);
};
#endif  // SRC_RDMHANDLER_H_
