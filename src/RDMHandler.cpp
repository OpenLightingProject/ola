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
 *  This class defines the actions to take for RDM responses.
 *  Copyright (C) 2010 Simon Newton
 */

#include <sysexits.h>
#include <ola/BaseTypes.h>
#include <ola/Callback.h>
#include <ola/Logging.h>
#include <ola/network/SelectServer.h>
#include <ola/rdm/RDMAPI.h>
#include <ola/rdm/RDMEnums.h>
#include <ola/rdm/RDMHelper.h>
#include <ola/rdm/UID.h>

#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "src/RDMHandler.h"

using std::cout;
using std::endl;
using std::map;
using std::string;
using std::vector;
using ola::rdm::UID;
using ola::network::SelectServer;
using ola::rdm::RDMAPI;
using ola::rdm::ResponseStatus;


ResponseHandler::ResponseHandler(RDMAPI *api, SelectServer *ss,
                                 const map<uint16_t, string> &pid_to_name_map)
  : m_api(api),
    m_ss(ss),
    m_exit_code(EX_OK),
    m_pid_to_name_map(pid_to_name_map) {
}


void ResponseHandler::ProxiedDeviceCount(const ResponseStatus &status,
                                         uint16_t device_count,
                                         bool list_changed) {
  if (!CheckForSuccess(status))
    return;
  cout << "Proxied Devices: " << device_count << endl;
  cout << (list_changed ? "List has changed" : "List hasn't changed") << endl;
}


void ResponseHandler::ProxiedDevices(const ResponseStatus &status,
                                     const vector<UID> &uids) {
  if (!CheckForSuccess(status))
    return;
  vector<UID>::const_iterator iter = uids.begin();
  cout << "Proxied PIDs" << endl;
  for (; iter != uids.end(); ++iter)
    cout << *iter << endl;
}


void ResponseHandler::CommStatus(const ResponseStatus &status,
                uint16_t short_message,
                uint16_t length_mismatch,
                uint16_t checksum_fail) {
  if (!CheckForSuccess(status))
    return;
  cout << "Communication Status" << endl;
  cout << "  Short Messages: " << short_message << endl;
  cout << "  Length Mismatch: " << length_mismatch << endl;
  cout << "  Checksum Failed: " << checksum_fail << endl;
}


void ResponseHandler::StatusMessages(
    const ResponseStatus &status,
    const vector<ola::rdm::StatusMessage> &messages) {
  if (!CheckForSuccess(status))
    return;
  vector<ola::rdm::StatusMessage>::const_iterator iter = messages.begin();
  cout << "Status Messages" << endl;
  cout << "------------------" << endl;
  for (; iter != messages.end(); ++iter) {
    cout << "Sub Device: " << iter->sub_device << endl;
    cout << "Status Type: " << ola::rdm::StatusTypeToString(iter->status_type)
      << endl;
    cout << "Message ID: " <<
      ola::rdm::StatusMessageIdToString(iter->status_message_id) << endl;
    cout << "Value 1: " << iter->value1 << endl;
    cout << "Value 2: " << iter->value2 << endl;
    cout << "------------------" << endl;
  }
}


void ResponseHandler::StatusIdDescription(const ResponseStatus &status,
                                          const string &status_id) {
  if (!CheckForSuccess(status))
    return;
  cout << "Status ID: " << status_id << endl;
}


void ResponseHandler::SubDeviceReporting(const ResponseStatus &status,
                                         uint8_t status_type) {
  if (!CheckForSuccess(status))
    return;

  cout << "Sub device reporting: ";
  switch (status_type) {
    case ola::rdm::STATUS_NONE:
      cout << "Status None";
      break;
    case ola::rdm::STATUS_GET_LAST_MESSAGE:
      cout << "Get last message";
      break;
    case ola::rdm::STATUS_ADVISORY:
      cout << "Advisory";
      break;
    case ola::rdm::STATUS_WARNING:
      cout << "Warning";
      break;
    case ola::rdm::STATUS_ERROR:
      cout << "Error";
      break;
    case ola::rdm::STATUS_ADVISORY_CLEARED:
      cout << "Advisory cleared";
      break;
    case ola::rdm::STATUS_WARNING_CLEARED:
      cout << "Warning cleared";
      break;
    case ola::rdm::STATUS_ERROR_CLEARED:
      cout << "Error cleared";
      break;
  }
  cout << endl;
}


void ResponseHandler::SupportedParameters(
    const ResponseStatus &status,
    const vector<uint16_t> &parameters) {
  if (!CheckForSuccess(status))
    return;
  vector<uint16_t>::const_iterator iter = parameters.begin();
  cout << "Supported Parameters" << endl;
  for (; iter != parameters.end(); ++iter) {
    cout << "  0x" << std::hex << *iter;
    map<uint16_t, string>::const_iterator pid_to_name_iter =
      m_pid_to_name_map.find(*iter);
    if (pid_to_name_iter != m_pid_to_name_map.end())
      cout << " (" << pid_to_name_iter->second << ")";
    cout << endl;
  }
}


void ResponseHandler::ParameterDescription(
    const ResponseStatus &status,
    const ola::rdm::ParameterDescriptor &description) {
  if (!CheckForSuccess(status))
    return;
  cout << "PID: 0x" << std::hex << description.pid << endl;
  cout << "PDL Size: " << static_cast<int>(description.pdl_size) <<
    endl;

  cout << "Data Type: " << ola::rdm::DataTypeToString(description.data_type) <<
    endl;
  cout << "Command class: ";
  switch (description.command_class) {
    case ola::rdm::CC_GET:
      cout << "Get";
      break;
    case ola::rdm::CC_SET:
      cout << "Set";
      break;
    case ola::rdm::CC_GET_SET:
      cout << "Get/Set";
      break;
    default:
      cout << "Unknown, was 0x" << std::hex <<
        static_cast<int>(description.command_class);
  }
  cout << endl;
  cout << "Unit: " << ola::rdm::UnitToString(description.unit) << endl;
  cout << "Prefix: " << ola::rdm::PrefixToString(description.prefix) << endl;
  cout << "Min Value: " << description.min_value << endl;
  cout << "Default Value: " << description.default_value << endl;
  cout << "Max Value: " << description.max_value << endl;
  cout << "Description: " << description.description << endl;
}


void ResponseHandler::DeviceInfo(
    const ResponseStatus &status,
    const ola::rdm::DeviceDescriptor &device_info) {
  if (!CheckForSuccess(status))
    return;

  cout << "Device Info" << endl;
  cout << "RDM Protocol Version: " <<
    static_cast<int>(device_info.protocol_version_high) <<
    "." << static_cast<int>(device_info.protocol_version_low) << endl;
  cout << "Device Model: 0x" << std::hex << device_info.device_model << endl;
  cout << "Product Category: " <<
    ola::rdm::ProductCategoryToString(device_info.product_category) << endl;
  cout << "Software Version: 0x" << std::hex << device_info.software_version
    << endl;
  cout << "DMX Footprint: " << std::dec << device_info.dmx_footprint << endl;
  cout << "DMX Personality: " <<
    static_cast<int>(device_info.current_personality) << " / " <<
    static_cast<int>(device_info.personaility_count) << endl;
  cout << "DMX Start Address: ";
  if (device_info.dmx_start_address > DMX_UNIVERSE_SIZE) {
    cout << "N/A";
  } else {
    cout << std::dec <<
    static_cast<int>(device_info.dmx_start_address);
  }
  cout << endl;
  cout << "# of Subdevices: " << device_info.sub_device_count << endl;
  cout << "Sensor Count: " << static_cast<int>(device_info.sensor_count) <<
    endl;
}


void ResponseHandler::ProductDetailIdList(
    const ResponseStatus &status,
    const vector<uint16_t> &ids) {
  if (!CheckForSuccess(status))
    return;
  vector<uint16_t>::const_iterator iter = ids.begin();
  cout << "Product Detail IDs" << endl;
  for (; iter != ids.end(); ++iter) {
    cout << "  " << ola::rdm::ProductDetailToString(*iter) << endl;
  }
}


void ResponseHandler::DeviceModelDescription(const ResponseStatus &status,
                                             const string &description) {
  if (!CheckForSuccess(status))
    return;
  cout << "Device Model Description: " << description << endl;
}


void ResponseHandler::ManufacturerLabel(const ResponseStatus &status,
                                        const string &label) {
  if (!CheckForSuccess(status))
    return;
  cout << "Manufacturer Label: " << label << endl;
}


void ResponseHandler::DeviceLabel(const ResponseStatus &status,
                                  const string &label) {
  if (!CheckForSuccess(status))
    return;
  cout << "Device Label: " << label << endl;
}


void ResponseHandler::FactoryDefaults(const ResponseStatus &status,
                                      bool using_defaults) {
  if (!CheckForSuccess(status))
    return;
  cout << "Using Factory Defaults: " << (using_defaults ? "Yes" : "No") <<
    endl;
}


void ResponseHandler::LanguageCapabilities(const ResponseStatus &status,
                                           const vector<string> &langs) {
  if (!CheckForSuccess(status))
    return;
  vector<string>::const_iterator iter = langs.begin();
  cout << "Supported Languages" << endl;
  for (; iter != langs.end(); ++iter) {
    cout << "  " << *iter << endl;
  }
}


void ResponseHandler::Language(const ResponseStatus &status,
                               const string &language) {
  if (!CheckForSuccess(status))
    return;
  cout << "Current language: " << language << endl;
}


void ResponseHandler::SoftwareVersionLabel(const ResponseStatus &status,
                                           const string &label) {
  if (!CheckForSuccess(status))
    return;
  cout << "Software Version Label: " << label << endl;
}


void ResponseHandler::BootSoftwareVersion(const ResponseStatus &status,
                                          uint32_t version) {
  if (!CheckForSuccess(status))
    return;
  cout << "Boot Software Version: 0x" << std::hex << version << endl;
}


void ResponseHandler::BootSoftwareVersionLabel(const ResponseStatus &status,
                                               const string &label) {
  if (!CheckForSuccess(status))
    return;
  cout << "Boot Software Version Label: " << label << endl;
}


void ResponseHandler::DMXPersonality(const ResponseStatus &status,
                                     uint8_t current_personality,
                                     uint8_t personality_count) {
  if (!CheckForSuccess(status))
    return;
  cout << "Current Personality: " << static_cast<int>(current_personality) <<
    endl;
  cout << "Personality Count: " << static_cast<int>(personality_count) << endl;
}


void ResponseHandler::DMXPersonalityDescription(
    const ResponseStatus &status,
    uint8_t personality,
    uint16_t slots_required,
    const string &label) {
  if (!CheckForSuccess(status))
    return;
  cout << "Personality #: " << static_cast<int>(personality) << endl;
  cout << "Slots required: " << slots_required << endl;
  cout << "Description: " << label << endl;
}


void ResponseHandler::DMXAddress(const ResponseStatus &status,
                                 uint16_t start_address) {
  if (!CheckForSuccess(status))
    return;
  cout << "DMX Start Address: ";
  if (start_address > DMX_UNIVERSE_SIZE) {
    cout << "N/A";
  } else {
    cout << std::dec <<
    static_cast<int>(start_address);
  }
  cout << endl;
}


void ResponseHandler::SlotInfo(
    const ResponseStatus &status,
    const vector<ola::rdm::SlotDescriptor> &slots) {
  if (!CheckForSuccess(status))
    return;
  vector<ola::rdm::SlotDescriptor>::const_iterator iter = slots.begin();
  for (; iter != slots.end(); ++iter) {
    cout << "Slot " << iter->slot_offset << endl;
    cout << "  ";
    cout << "Slot Type: " <<
      ola::rdm::SlotInfoToString(iter->slot_type, iter->slot_label) << endl;
  }
}


void ResponseHandler::SlotDescription(const ResponseStatus &status,
                                      uint16_t slot_offset,
                                      const string &description) {
  if (!CheckForSuccess(status))
    return;
  cout << "Slot #: " << slot_offset << endl;
  cout << "Description: " << description << endl;
}


void ResponseHandler::SlotDefaultValues(
    const ResponseStatus &status,
    const vector<ola::rdm::SlotDefault> &defaults) {
  if (!CheckForSuccess(status))
    return;
  vector<ola::rdm::SlotDefault>::const_iterator iter = defaults.begin();
  cout << "Default Slot Values" << endl;
  for (; iter != defaults.end(); ++iter) {
    cout << " Slot " << iter->slot_offset << ", default " <<
    static_cast<int>(iter->default_value) << endl;
  }
}


void ResponseHandler::SensorDefinition(
    const ResponseStatus &status,
    const ola::rdm::SensorDescriptor &descriptor) {
  if (!CheckForSuccess(status))
    return;
  cout << "Sensor #: " << static_cast<int>(descriptor.sensor_number) << endl;
  cout << "Type: " << ola::rdm::SensorTypeToString(descriptor.type) << endl;
  cout << "Unit: " << ola::rdm::UnitToString(descriptor.unit) << endl;
  cout << "Prefix: " << ola::rdm::PrefixToString(descriptor.prefix) << endl;
  cout << "Range: " << descriptor.range_min << " - " << descriptor.range_max
    << endl;
  cout << "Normal: " << descriptor.normal_min << " - " << descriptor.normal_max
    << endl;
  cout << "Recording support: " <<
    ((descriptor.recorded_value_support & 0x2) ? "Highest/Lowest" : "") <<
    ((descriptor.recorded_value_support == 0x3) ? "/" : "") <<
    ((descriptor.recorded_value_support & 0x1) ? "Snapshot" : "") <<
    endl;
  cout << "Description: " << descriptor.description << endl;
}


void ResponseHandler::SensorValue(
    const ResponseStatus &status,
    const ola::rdm::SensorValueDescriptor &descriptor) {
  if (!CheckForSuccess(status))
    return;
  cout << "Sensor #: " << static_cast<int>(descriptor.sensor_number) << endl;
  cout << "Present Value: " << descriptor.present_value << endl;
  cout << "Lowest Value: " << descriptor.lowest << endl;
  cout << "Highest Value: " << descriptor.highest << endl;
  cout << "Recorded Value: " << descriptor.recorded << endl;
}


void ResponseHandler::DeviceHours(const ResponseStatus &status,
                                  uint32_t hours) {
  if (!CheckForSuccess(status))
    return;
  cout << "Device Hours: " << hours << endl;
}


void ResponseHandler::LampHours(const ResponseStatus &status,
                                uint32_t hours) {
  if (!CheckForSuccess(status))
    return;
  cout << "Lamp Hours: " << hours << endl;
}


void ResponseHandler::LampStrikes(const ResponseStatus &status,
                                  uint32_t strikes) {
  if (!CheckForSuccess(status))
    return;
  cout << "Lamp Strikes: " << strikes << endl;
}

void ResponseHandler::LampState(const ResponseStatus &status,
                                uint8_t state) {
  if (!CheckForSuccess(status))
    return;
  cout << "Lamp State: " << ola::rdm::LampStateToString(state) << endl;
}

void ResponseHandler::LampMode(const ResponseStatus &status, uint8_t mode) {
  if (!CheckForSuccess(status))
    return;
  cout << "Lamp Mode: " << ola::rdm::LampModeToString(mode) << endl;
}

void ResponseHandler::DevicePowerCycles(const ResponseStatus &status,
                                        uint32_t power_cycles) {
  if (!CheckForSuccess(status))
    return;
  cout << "Device power cycles: " << power_cycles << endl;
}

void ResponseHandler::DisplayInvert(const ResponseStatus &status,
                                    uint8_t invert_mode) {
  if (!CheckForSuccess(status))
    return;
  cout << "Display Invert: ";
  switch (invert_mode) {
    case 0:
      cout << "Off";
      break;
    case 1:
      cout << "On";
      break;
    case 2:
      cout << "Auto";
      break;
    default:
      cout << "Unknown, was " << static_cast<int>(invert_mode);
  }
  cout << endl;
}

void ResponseHandler::DisplayLevel(const ResponseStatus &status,
                                   uint8_t level) {
  if (!CheckForSuccess(status))
    return;
  cout << "Display level: " << static_cast<int>(level) << endl;
}

void ResponseHandler::PanInvert(const ResponseStatus &status,
                                uint8_t inverted) {
  if (!CheckForSuccess(status))
    return;
  cout << "Pan Invert: " << (inverted ? "Yes" : "No") << endl;
}

void ResponseHandler::TiltInvert(const ResponseStatus &status,
                                 uint8_t inverted) {
  if (!CheckForSuccess(status))
    return;
  cout << "Tilt Invert: " << (inverted ? "Yes" : "No") << endl;
}

void ResponseHandler::PanTiltSwap(const ResponseStatus &status,
                                  uint8_t inverted) {
  if (!CheckForSuccess(status))
    return;
  cout << "Pan/Tilt Swap: " << (inverted ? "Yes" : "No") << endl;
}

void ResponseHandler::Clock(const ResponseStatus &status,
                            const ola::rdm::ClockValue &clock) {
  if (!CheckForSuccess(status))
    return;
  cout << "Current time:" << endl;
  cout << "d/m/y: " << static_cast<int>(clock.day) << "/" <<
    static_cast<int>(clock.month) << "/" <<
    static_cast<int>(clock.year) << ", " << static_cast<int>(clock.hour) <<
    ":" << static_cast<int>(clock.minute) << ":" <<
    static_cast<int>(clock.second) << endl;
}

void ResponseHandler::IdentifyMode(const ResponseStatus &status, bool mode) {
  if (!CheckForSuccess(status))
    return;
  cout << (mode ? "Identify on" : "Identify off") << endl;
}


void ResponseHandler::PowerState(const ResponseStatus &status,
                                 uint8_t power_state) {
  if (!CheckForSuccess(status))
    return;
  cout << "Power State: " << ola::rdm::PowerStateToString(power_state) << endl;
}


void ResponseHandler::SelfTestEnabled(const ResponseStatus &status,
                                      bool is_enabled) {
  if (!CheckForSuccess(status))
    return;
  cout << "Self Test Mode: " << (is_enabled ? "On" : "Off") << endl;
}


void ResponseHandler::SelfTestDescription(const ResponseStatus &status,
                                          uint8_t self_test_number,
                                          const string &description) {
  if (!CheckForSuccess(status))
    return;
  cout << "Self Test Number: " << static_cast<int>(self_test_number) << endl;
  cout << description << endl;
}


void ResponseHandler::PresetPlaybackMode(const ResponseStatus &status,
                                         uint16_t preset_mode,
                                         uint8_t level) {
  if (!CheckForSuccess(status))
    return;
  cout << "Preset Mode: ";
  if (preset_mode == ola::rdm::PRESET_PLAYBACK_OFF)
    cout << "Off (DMX Input)";
  else if (preset_mode == ola::rdm::PRESET_PLAYBACK_ALL)
    cout << "All (plays scenes in a sequence)";
  else
    cout << static_cast<int>(preset_mode);
  cout << endl;
  cout << "Level: " << static_cast<int>(level) << endl;
}

void ResponseHandler::ClearCommStatus(const ResponseStatus &status) {
  CheckForSuccess(status);
}


void ResponseHandler::ClearStatusId(const ResponseStatus &status) {
  CheckForSuccess(status);
}


void ResponseHandler::SetSubDeviceReporting(const ResponseStatus &status) {
  CheckForSuccess(status);
}


void ResponseHandler::SetDeviceLabel(const ResponseStatus &status) {
  CheckForSuccess(status);
}


void ResponseHandler::ResetToFactoryDefaults(const ResponseStatus &status) {
  CheckForSuccess(status);
}


void ResponseHandler::SetLanguage(const ResponseStatus &status) {
  CheckForSuccess(status);
}


void ResponseHandler::SetDMXPersonality(const ResponseStatus &status) {
  CheckForSuccess(status);
}


void ResponseHandler::SetDMXAddress(const ResponseStatus &status) {
  CheckForSuccess(status);
}


void ResponseHandler::SetSensorValue(
    const ResponseStatus &status,
    const ola::rdm::SensorValueDescriptor &descriptor) {
  if (!CheckForSuccess(status))
    return;
  // The labpack returns a sensor definition even when it nacks. This behaviour
  // isn't guaranteed however.
  cout << "Sensor #: " << static_cast<int>(descriptor.sensor_number) << endl;
  cout << "Present Value: " << descriptor.present_value << endl;
  cout << "Lowest Value: " << descriptor.lowest << endl;
  cout << "Highest Value: " << descriptor.highest << endl;
  cout << "Recorded Value: " << descriptor.recorded << endl;
}


void ResponseHandler::RecordSensors(const ResponseStatus &status) {
  CheckForSuccess(status);
}


void ResponseHandler::SetDeviceHours(const ResponseStatus &status) {
  CheckForSuccess(status);
}


void ResponseHandler::SetLampHours(const ResponseStatus &status) {
  CheckForSuccess(status);
}


void ResponseHandler::SetLampStrikes(const ResponseStatus &status) {
  CheckForSuccess(status);
}


void ResponseHandler::SetDevicePowerCycles(const ResponseStatus &status) {
  CheckForSuccess(status);
}


void ResponseHandler::SetLampState(const ResponseStatus &status) {
  CheckForSuccess(status);
}

void ResponseHandler::SetLampMode(const ResponseStatus &status) {
  CheckForSuccess(status);
}

void ResponseHandler::SetDisplayInvert(const ResponseStatus &status) {
  CheckForSuccess(status);
}

void ResponseHandler::SetDisplayLevel(const ResponseStatus &status) {
  CheckForSuccess(status);
}

void ResponseHandler::SetPanInvert(const ResponseStatus &status) {
  CheckForSuccess(status);
}

void ResponseHandler::SetPanTiltSwap(const ResponseStatus &status) {
  CheckForSuccess(status);
}

void ResponseHandler::SetTiltInvert(const ResponseStatus &status) {
  CheckForSuccess(status);
}

void ResponseHandler::IdentifyDevice(const ResponseStatus &status) {
  CheckForSuccess(status);
}

void ResponseHandler::SetClock(const ResponseStatus &status) {
  CheckForSuccess(status);
}

void ResponseHandler::ResetDevice(const ResponseStatus &status) {
  CheckForSuccess(status);
}


void ResponseHandler::SetPowerState(const ResponseStatus &status) {
  CheckForSuccess(status);
}


void ResponseHandler::PerformSelfTest(const ResponseStatus &status) {
  CheckForSuccess(status);
}

void ResponseHandler::CapturePreset(const ResponseStatus &status) {
  CheckForSuccess(status);
}


void ResponseHandler::SetPresetPlaybackMode(const ResponseStatus &status) {
  CheckForSuccess(status);
}


void ResponseHandler::DefaultHandler(const ResponseStatus &status,
                                     uint16_t pid,
                                     const string &data) {
  if (!CheckForSuccess(status))
    return;
  cout << "Got queued message for pid 0x" << std::hex << pid <<
    ", length of data was " << data.size();
}


/*
 * Check if a request completed sucessfully, if not display the errors.
 */
bool ResponseHandler::CheckForSuccess(const ResponseStatus &status) {
  // always terminate for now
  m_ss->Terminate();
  if (!status.error.empty()) {
    std::cerr << status.error << endl;
    m_exit_code = EX_SOFTWARE;
    return false;
  }

  if (status.response_code == ola::rdm::RDM_COMPLETED_OK) {
    switch (status.response_type) {
      case ola::rdm::RDM_ACK:
        return true;
      case ola::rdm::RDM_ACK_TIMER:
        // TODO(simon): handle this
        cout << "Got ACK TIMER for " << status.AckTimer() << " ms." << endl;
        break;
      case ola::rdm::RDM_NACK_REASON:
        cout << "Request was NACKED with code: ";
        cout << ola::rdm::NackReasonToString(status.NackReason()) << endl;
        break;
    }
  } else if (status.response_code != ola::rdm::RDM_WAS_BROADCAST) {
    std::cerr << ola::rdm::ResponseCodeToString(status.response_code);
    m_exit_code = EX_SOFTWARE;
  }
  return false;
}
// End implementation
