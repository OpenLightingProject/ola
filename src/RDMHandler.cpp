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
#include <ola/rdm/RDMEnums.h>
#include <ola/rdm/RDMAPI.h>
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
  for (; iter != messages.end(); ++iter) {
    cout << "Sub Device: " << iter->sub_device << endl;
    PrintStatusType(iter->status_type);
    PrintStatusMessageId(iter->status_message_id);
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

  PrintDataType(description.data_type);
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
  PrintUnit(description.unit);
  PrintPrefix(description.prefix);
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
  cout << "RDM Protocol Version: " << (device_info.protocol_version >> 8) <<
    "." << (device_info.protocol_version & 0xff) << endl;
  cout << "Device Model: 0x" << std::hex << device_info.device_model << endl;
  PrintProductCategory(device_info.product_category);
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
  for (; iter != ids.end(); ++iter)
    cout << "  0x" << std::hex << *iter << endl;
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
    PrintSlotInfo(iter->slot_type, iter->slot_label);
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
  PrintType(descriptor.type);
  PrintUnit(descriptor.unit);
  PrintPrefix(descriptor.prefix);
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
  PrintLampState(state);
}

void ResponseHandler::LampMode(const ResponseStatus &status, uint8_t mode) {
  if (!CheckForSuccess(status))
    return;
  PrintLampMode(mode);
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

void ResponseHandler::IdentifyMode(const ResponseStatus &status, bool mode) {
  if (!CheckForSuccess(status))
    return;
  cout << (mode ? "Identify on" : "Identify off") << endl;
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
  CheckForSuccess(status);
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

void ResponseHandler::ResetDevice(const ResponseStatus &status) {
  CheckForSuccess(status);
}


/*
 * Check if a request completed sucessfully, if not display the errors.
 */
bool ResponseHandler::CheckForSuccess(const ResponseStatus &status) {
  // always terminate for now
  m_ss->Terminate();
  switch (status.ResponseType()) {
    case ResponseStatus::TRANSPORT_ERROR:
      std::cerr << status.Error() << endl;
      m_exit_code = EX_SOFTWARE;
      return false;
    case ResponseStatus::BROADCAST_REQUEST:
      return false;
    case ResponseStatus::REQUEST_NACKED:
      cout << "Request was NACKED with code: ";
      PrintNackReason(status.NackReason());
      cout << endl;
      return false;
    case ResponseStatus::MALFORMED_RESPONSE:
      std::cerr << status.Error() << endl;
      m_exit_code = EX_SOFTWARE;
      return false;
    case ResponseStatus::VALID_RESPONSE:
      return true;
    default:
      cout << "Unknown response status " <<
        static_cast<int>(status.ResponseType());
      return false;
  }
}


void ResponseHandler::PrintDataType(uint8_t type) {
  cout << "Data Type: ";
  switch (type) {
    case ola::rdm::DS_NOT_DEFINED:
      cout << "Not defined";
      break;
    case ola::rdm::DS_BIT_FIELD:
      cout << "Bit field";
      break;
    case ola::rdm::DS_ASCII:
      cout << "ASCII";
      break;
    case ola::rdm::DS_UNSIGNED_BYTE:
      cout << "uint8";
      break;
    case ola::rdm::DS_SIGNED_BYTE:
      cout << "int8";
      break;
    case ola::rdm::DS_UNSIGNED_WORD:
      cout << "uint16";
      break;
    case ola::rdm::DS_SIGNED_WORD:
      cout << "int16";
      break;
    case ola::rdm::DS_UNSIGNED_DWORD:
      cout << "uint32 ";
      break;
    case ola::rdm::DS_SIGNED_DWORD:
      cout << "int32 ";
      break;
    default:
      cout << "Unknown, was " << static_cast<int>(type);
  }
  cout << endl;
}


void ResponseHandler::PrintType(uint8_t type) {
  cout << "Type: ";
  switch (type) {
    case ola::rdm::SENSOR_TEMPERATURE:
      cout << "Temperature";
      break;
    case ola::rdm::SENSOR_VOLTAGE:
      cout << "Voltage";
      break;
    case ola::rdm::SENSOR_CURRENT:
      cout << "Current";
      break;
    case ola::rdm::SENSOR_FREQUENCY:
      cout << "Frequency";
      break;
    case ola::rdm::SENSOR_RESISTANCE:
      cout << "Resistance";
      break;
    case ola::rdm::SENSOR_POWER:
      cout << "Power";
      break;
    case ola::rdm::SENSOR_MASS:
      cout << "Mass";
      break;
    case ola::rdm::SENSOR_LENGTH:
      cout << "Length";
      break;
    case ola::rdm::SENSOR_AREA:
      cout << "Area";
      break;
    case ola::rdm::SENSOR_VOLUME:
      cout << "Volume";
      break;
    case ola::rdm::SENSOR_DENSITY:
      cout << "Density";
      break;
    case ola::rdm::SENSOR_VELOCITY:
      cout << "Velocity";
      break;
    case ola::rdm::SENSOR_ACCELERATION:
      cout << "Acceleration";
      break;
    case ola::rdm::SENSOR_FORCE:
      cout << "Force";
      break;
    case ola::rdm::SENSOR_ENERGY:
      cout << "Energy";
      break;
    case ola::rdm::SENSOR_PRESSURE:
      cout << "Pressure";
      break;
    case ola::rdm::SENSOR_TIME:
      cout << "Time";
      break;
    case ola::rdm::SENSOR_ANGLE:
      cout << "Angle";
      break;
    case ola::rdm::SENSOR_POSITION_X:
      cout << "Position X";
      break;
    case ola::rdm::SENSOR_POSITION_Y:
      cout << "Position Y";
      break;
    case ola::rdm::SENSOR_POSITION_Z:
      cout << "Position Z";
      break;
    case ola::rdm::SENSOR_ANGULAR_VELOCITY:
      cout << "Angular velocity";
      break;
    case ola::rdm::SENSOR_LUMINOUS_INTENSITY:
      cout << "luminous intensity";
      break;
    case ola::rdm::SENSOR_LUMINOUS_FLUX:
      cout << "luminous flux";
      break;
    case ola::rdm::SENSOR_ILLUMINANCE:
      cout << "illuminance";
      break;
    case ola::rdm::SENSOR_CHROMINANCE_RED:
      cout << "chrominance red";
      break;
    case ola::rdm::SENSOR_CHROMINANCE_GREEN:
      cout << "chrominance green";
      break;
    case ola::rdm::SENSOR_CHROMINANCE_BLUE:
      cout << "chrominance blue";
      break;
    case ola::rdm::SENSOR_CONTACTS:
      cout << "contacts";
      break;
    case ola::rdm::SENSOR_MEMORY:
      cout << "memory";
      break;
    case ola::rdm::SENSOR_ITEMS:
      cout << "items";
      break;
    case ola::rdm::SENSOR_HUMIDITY:
      cout << "humidity";
      break;
    case ola::rdm::SENSOR_COUNTER_16BIT:
      cout << "16 bith counter";
      break;
    case ola::rdm::SENSOR_OTHER:
      cout << "Other";
      break;
    default:
      cout << "Unknown, was " << static_cast<int>(type);
  }
  cout << endl;
}


void ResponseHandler::PrintUnit(uint8_t unit) {
  cout << "Unit: ";
  switch (unit) {
    case ola::rdm::UNITS_NONE:
      cout << "none";
      break;
    case ola::rdm::UNITS_CENTIGRADE:
      cout << "degrees C";
      break;
    case ola::rdm::UNITS_VOLTS_DC:
      cout << "Volts (DC)";
      break;
    case ola::rdm::UNITS_VOLTS_AC_PEAK:
      cout << "Volts (AC Peak)";
      break;
    case ola::rdm::UNITS_VOLTS_AC_RMS:
      cout << "Volts (AC RMS)";
      break;
    case ola::rdm::UNITS_AMPERE_DC:
      cout << "Amps (DC)";
      break;
    case ola::rdm::UNITS_AMPERE_AC_PEAK:
      cout << "Amps (AC Peak)";
      break;
    case ola::rdm::UNITS_AMPERE_AC_RMS:
      cout << "Amps (AC RMS)";
      break;
    case ola::rdm::UNITS_HERTZ:
      cout << "Hz";
      break;
    case ola::rdm::UNITS_OHM:
      cout << "ohms";
      break;
    case ola::rdm::UNITS_WATT:
      cout << "W";
      break;
    case ola::rdm::UNITS_KILOGRAM:
      cout << "kg";
      break;
    case ola::rdm::UNITS_METERS:
      cout << "m";
      break;
    case ola::rdm::UNITS_METERS_SQUARED:
      cout << "m^2";
      break;
    case ola::rdm::UNITS_METERS_CUBED:
      cout << "m^3";
      break;
    case ola::rdm::UNITS_KILOGRAMMES_PER_METER_CUBED:
      cout << "kg/m^3";
      break;
    case ola::rdm::UNITS_METERS_PER_SECOND:
      cout << "m/s";
      break;
    case ola::rdm::UNITS_METERS_PER_SECOND_SQUARED:
      cout << "m/s^2";
      break;
    case ola::rdm::UNITS_NEWTON:
      cout << "newton";
      break;
    case ola::rdm::UNITS_JOULE:
      cout << "joule";
      break;
    case ola::rdm::UNITS_PASCAL:
      cout << "pascal";
      break;
    case ola::rdm::UNITS_SECOND:
      cout << "second";
      break;
    case ola::rdm::UNITS_DEGREE:
      cout << "degree";
      break;
    case ola::rdm::UNITS_STERADIAN:
      cout << "steradian";
      break;
    case ola::rdm::UNITS_CANDELA:
      cout << "candela";
      break;
    case ola::rdm::UNITS_LUMEN:
      cout << "lumen";
      break;
    case ola::rdm::UNITS_LUX:
      cout << "lux";
      break;
    case ola::rdm::UNITS_IRE:
      cout << "ire";
      break;
    case ola::rdm::UNITS_BYTE:
      cout << "bytes";
      break;
    default:
      cout << "Unknown, was " << static_cast<int>(unit);
  }
  cout << endl;
}


void ResponseHandler::PrintPrefix(uint8_t prefix) {
  cout << "Prefix: ";
  switch (prefix) {
    case ola::rdm::PREFIX_NONE:
      cout << "None";
      break;
    case ola::rdm::PREFIX_DECI:
      cout << "Deci";
      break;
    case ola::rdm::PREFIX_CENTI:
      cout << "Centi";
      break;
    case ola::rdm::PREFIX_MILLI:
      cout << "Milli";
      break;
    case ola::rdm::PREFIX_MICRO:
      cout << "Micro";
      break;
    case ola::rdm::PREFIX_NANO:
      cout << "Nano";
      break;
    case ola::rdm::PREFIX_PICO:
      cout << "Pico";
      break;
    case ola::rdm::PREFIX_FEMPTO:
      cout << "Fempto";
      break;
    case ola::rdm::PREFIX_ATTO:
      cout << "Atto";
      break;
    case ola::rdm::PREFIX_ZEPTO:
      cout << "Zepto";
      break;
    case ola::rdm::PREFIX_YOCTO:
      cout << "Yocto";
      break;
    case ola::rdm::PREFIX_DECA:
      cout << "Deca";
      break;
    case ola::rdm::PREFIX_HECTO:
      cout << "Hecto";
      break;
    case ola::rdm::PREFIX_KILO:
      cout << "Kilo";
      break;
    case ola::rdm::PREFIX_MEGA:
      cout << "Mega";
      break;
    case ola::rdm::PREFIX_GIGA:
      cout << "Giga";
      break;
    case ola::rdm::PREFIX_TERRA:
      cout << "Terra";
      break;
    case ola::rdm::PREFIX_PETA:
      cout << "Peta";
      break;
    case ola::rdm::PREFIX_EXA:
      cout << "Exa";
      break;
    case ola::rdm::PREFIX_ZETTA:
      cout << "Zetta";
      break;
    case ola::rdm::PREFIX_YOTTA:
      cout << "Yotta";
      break;
    default:
      cout << "Unknown, was " << static_cast<int>(prefix);
  }
  cout << endl;
}


void ResponseHandler::PrintProductCategory(uint16_t category) {
  cout << "Product Category: ";
  switch (category) {
    case ola::rdm::PRODUCT_CATEGORY_NOT_DECLARED:
      cout << "Not declared";
      break;
    case ola::rdm::PRODUCT_CATEGORY_FIXTURE:
      cout << "fixture";
      break;
    case ola::rdm::PRODUCT_CATEGORY_FIXTURE_FIXED:
      cout << "fixed fixture";
      break;
    case ola::rdm::PRODUCT_CATEGORY_FIXTURE_MOVING_YOKE:
      cout << "moving yoke fixture";
      break;
    case ola::rdm::PRODUCT_CATEGORY_FIXTURE_MOVING_MIRROR:
      cout << "moving mirror fixture";
      break;
    case ola::rdm::PRODUCT_CATEGORY_FIXTURE_OTHER:
      cout << "fixture other";
      break;
    case ola::rdm::PRODUCT_CATEGORY_FIXTURE_ACCESSORY:
      cout << "fixture accessory";
      break;
    case ola::rdm::PRODUCT_CATEGORY_FIXTURE_ACCESSORY_COLOR:
      cout << "fixture accessory color";
      break;
    case ola::rdm::PRODUCT_CATEGORY_FIXTURE_ACCESSORY_YOKE:
      cout << "fixture accessory yoke";
      break;
    case ola::rdm::PRODUCT_CATEGORY_FIXTURE_ACCESSORY_MIRROR:
      cout << "fixture accessory mirror";
      break;
    case ola::rdm::PRODUCT_CATEGORY_FIXTURE_ACCESSORY_EFFECT:
      cout << "fixture accessory effect";
      break;
    case ola::rdm::PRODUCT_CATEGORY_FIXTURE_ACCESSORY_BEAM:
      cout << "fixture accessory beam";
      break;
    case ola::rdm::PRODUCT_CATEGORY_FIXTURE_ACCESSORY_OTHER:
      cout << "fixture accessory other";
      break;
    case ola::rdm::PRODUCT_CATEGORY_PROJECTOR:
      cout << "projector";
      break;
    case ola::rdm::PRODUCT_CATEGORY_PROJECTOR_FIXED:
      cout << "projector fixed";
      break;
    case ola::rdm::PRODUCT_CATEGORY_PROJECTOR_MOVING_YOKE:
      cout << "projector moving yoke";
      break;
    case ola::rdm::PRODUCT_CATEGORY_PROJECTOR_MOVING_MIRROR:
      cout << "projector moving mirror";
      break;
    case ola::rdm::PRODUCT_CATEGORY_PROJECTOR_OTHER:
      cout << "projector other";
      break;
    case ola::rdm::PRODUCT_CATEGORY_ATMOSPHERIC:
      cout << "atmospheric";
      break;
    case ola::rdm::PRODUCT_CATEGORY_ATMOSPHERIC_EFFECT:
      cout << "atmospheric effect";
      break;
    case ola::rdm::PRODUCT_CATEGORY_ATMOSPHERIC_PYRO:
      cout << "atmospheric pyro";
      break;
    case ola::rdm::PRODUCT_CATEGORY_ATMOSPHERIC_OTHER:
      cout << "atmospheric other";
      break;
    case ola::rdm::PRODUCT_CATEGORY_DIMMER:
      cout << "dimmer";
      break;
    case ola::rdm::PRODUCT_CATEGORY_DIMMER_AC_INCANDESCENT:
      cout << "dimmer ac incandescent";
      break;
    case ola::rdm::PRODUCT_CATEGORY_DIMMER_AC_FLUORESCENT:
      cout << "dimmer ac fluorescent";
      break;
    case ola::rdm::PRODUCT_CATEGORY_DIMMER_AC_COLDCATHODE:
      cout << "dimmer ac cold cathode";
      break;
    case ola::rdm::PRODUCT_CATEGORY_DIMMER_AC_NONDIM:
      cout << "dimmer ac no dim";
      break;
    case ola::rdm::PRODUCT_CATEGORY_DIMMER_AC_ELV:
      cout << "dimmer ac ELV";
      break;
    case ola::rdm::PRODUCT_CATEGORY_DIMMER_AC_OTHER:
      cout << "dimmer ac other";
      break;
    case ola::rdm::PRODUCT_CATEGORY_DIMMER_DC_LEVEL:
      cout << "dimmer dc level";
      break;
    case ola::rdm::PRODUCT_CATEGORY_DIMMER_DC_PWM:
      cout << "dimmer dc pwm";
      break;
    case ola::rdm::PRODUCT_CATEGORY_DIMMER_CS_LED:
      cout << "dimmer dc led";
      break;
    case ola::rdm::PRODUCT_CATEGORY_DIMMER_OTHER:
      cout << "dimmer other";
      break;
    case ola::rdm::PRODUCT_CATEGORY_POWER:
      cout << "power";
      break;
    case ola::rdm::PRODUCT_CATEGORY_POWER_CONTROL:
      cout << "power control";
      break;
    case ola::rdm::PRODUCT_CATEGORY_POWER_SOURCE:
      cout << "power source";
      break;
    case ola::rdm::PRODUCT_CATEGORY_POWER_OTHER:
      cout << "power other";
      break;
    case ola::rdm::PRODUCT_CATEGORY_SCENIC:
      cout << "scenic";
      break;
    case ola::rdm::PRODUCT_CATEGORY_SCENIC_DRIVE:
      cout << "scenic drive";
      break;
    case ola::rdm::PRODUCT_CATEGORY_SCENIC_OTHER:
      cout << "scenic other";
      break;
    case ola::rdm::PRODUCT_CATEGORY_DATA:
      cout << "data";
      break;
    case ola::rdm::PRODUCT_CATEGORY_DATA_DISTRIBUTION:
      cout << "data distribution";
      break;
    case ola::rdm::PRODUCT_CATEGORY_DATA_CONVERSION:
      cout << "data conversion";
      break;
    case ola::rdm::PRODUCT_CATEGORY_DATA_OTHER:
      cout << "data other";
      break;
    case ola::rdm::PRODUCT_CATEGORY_AV:
      cout << "A/V";
      break;
    case ola::rdm::PRODUCT_CATEGORY_AV_AUDIO:
      cout << "A/V audio";
      break;
    case ola::rdm::PRODUCT_CATEGORY_AV_VIDEO:
      cout << "A/V video";
      break;
    case ola::rdm::PRODUCT_CATEGORY_AV_OTHER:
      cout << "AV other";
      break;
    case ola::rdm::PRODUCT_CATEGORY_MONITOR:
      cout << "monitor";
      break;
    case ola::rdm::PRODUCT_CATEGORY_MONITOR_ACLINEPOWER:
      cout << "AC line power monitor";
      break;
    case ola::rdm::PRODUCT_CATEGORY_MONITOR_DCPOWER:
      cout << "DC power monitor";
      break;
    case ola::rdm::PRODUCT_CATEGORY_MONITOR_ENVIRONMENTAL:
      cout << "environmental monitor";
      break;
    case ola::rdm::PRODUCT_CATEGORY_MONITOR_OTHER:
      cout << "other monitor";
      break;
    case ola::rdm::PRODUCT_CATEGORY_CONTROL:
      cout << "control";
      break;
    case ola::rdm::PRODUCT_CATEGORY_CONTROL_CONTROLLER:
      cout << "controller";
      break;
    case ola::rdm::PRODUCT_CATEGORY_CONTROL_BACKUPDEVICE:
      cout << "backup device";
      break;
    case ola::rdm::PRODUCT_CATEGORY_CONTROL_OTHER:
      cout << "other control";
      break;
    case ola::rdm::PRODUCT_CATEGORY_TEST:
      cout << "test";
      break;
    case ola::rdm::PRODUCT_CATEGORY_TEST_EQUIPMENT:
      cout << "test equipment";
      break;
    case ola::rdm::PRODUCT_CATEGORY_TEST_EQUIPMENT_OTHER:
      cout << "test equipment other";
      break;
    case ola::rdm::PRODUCT_CATEGORY_OTHER:
      cout << "other";
      break;
    default:
      cout << "Unknown, was " << static_cast<int>(category);
  }
  cout << endl;
}


void ResponseHandler::PrintSlotInfo(uint8_t slot_type, uint16_t slot_label) {
  cout << "Slot Type: ";
  if (slot_type == ola::rdm::ST_PRIMARY) {
    switch (slot_label) {
      case ola::rdm::SD_INTENSITY:
        cout << "Primary, intensity";
        break;
      case ola::rdm::SD_INTENSITY_MASTER:
        cout << "Primary, intensity master";
        break;
      case ola::rdm::SD_PAN:
        cout << "Primary, pan";
        break;
      case ola::rdm::SD_TILT:
        cout << "Primary, tilt";
        break;
      case ola::rdm::SD_COLOR_WHEEL:
        cout << "Primary, color wheel";
        break;
      case ola::rdm::SD_COLOR_SUB_CYAN:
        cout << "Primary, subtractive cyan";
        break;
      case ola::rdm::SD_COLOR_SUB_YELLOW:
        cout << "Primary, subtractive yellow";
        break;
      case ola::rdm::SD_COLOR_SUB_MAGENTA:
        cout << "Primary, subtractive magenta";
        break;
      case ola::rdm::SD_COLOR_ADD_RED:
        cout << "Primary, additive red";
        break;
      case ola::rdm::SD_COLOR_ADD_GREEN:
        cout << "Primary, additive green";
        break;
      case ola::rdm::SD_COLOR_ADD_BLUE:
        cout << "Primary, additive blue";
        break;
      case ola::rdm::SD_COLOR_CORRECTION:
        cout << "Primary, color correction";
        break;
      case ola::rdm::SD_COLOR_SCROLL:
        cout << "Primary, scroll";
        break;
      case ola::rdm::SD_COLOR_SEMAPHORE:
        cout << "Primary, color semaphone";
        break;
      case ola::rdm::SD_STATIC_GOBO_WHEEL:
        cout << "Primary, static gobo wheel";
        break;
      case ola::rdm::SD_ROTO_GOBO_WHEEL:
        cout << "Primary, gobo wheel";
        break;
      case ola::rdm::SD_PRISM_WHEEL:
        cout << "Primary, prism wheel";
        break;
      case ola::rdm::SD_EFFECTS_WHEEL:
        cout << "Primary, effects wheel";
        break;
      case ola::rdm::SD_BEAM_SIZE_IRIS:
        cout << "Primary, iris size";
        break;
      case ola::rdm::SD_EDGE:
        cout << "Primary, edge";
        break;
      case ola::rdm::SD_FROST:
        cout << "Primary, frost";
        break;
      case ola::rdm::SD_STROBE:
        cout << "Primary, strobe";
        break;
      case ola::rdm::SD_ZOOM:
        cout << "Primary, zoom";
        break;
      case ola::rdm::SD_FRAMING_SHUTTER:
        cout << "Primary, framing shutter";
        break;
      case ola::rdm::SD_SHUTTER_ROTATE:
        cout << "Primary, shuttle rotate";
        break;
      case ola::rdm::SD_DOUSER:
        cout << "Primary, douser";
        break;
      case ola::rdm::SD_BARN_DOOR:
        cout << "Primary, barn door";
        break;
      case ola::rdm::SD_LAMP_CONTROL:
        cout << "Primary, lamp control";
        break;
      case ola::rdm::SD_FIXTURE_CONTROL:
        cout << "Primary, fixture control";
        break;
      case ola::rdm::SD_FIXTURE_SPEED:
        cout << "Primary, fixture speed";
        break;
      case ola::rdm::SD_MACRO:
        cout << "Primary, macro";
        break;
      case ola::rdm::SD_UNDEFINED:
        cout << "Primary, undefined";
        break;
      default:
        cout << "Primary unknown, was " << slot_label;
    }
  } else {
    switch (slot_type) {
      case ola::rdm::ST_SEC_FINE:
        cout << "fine control for slot " << slot_label;
        break;
      case ola::rdm::ST_SEC_TIMING:
        cout << "timing control for slot " << slot_label;
        break;
      case ola::rdm::ST_SEC_SPEED:
        cout << "speed control for slot " << slot_label;
        break;
      case ola::rdm::ST_SEC_CONTROL:
        cout << "mode control for slot " << slot_label;
        break;
      case ola::rdm::ST_SEC_INDEX:
        cout << "index control for slot " << slot_label;
        break;
      case ola::rdm::ST_SEC_ROTATION:
        cout << "rotation speed control for slot " << slot_label;
        break;
      case ola::rdm::ST_SEC_INDEX_ROTATE:
        cout << "rotation index control for slot " << slot_label;
        break;
      case ola::rdm::ST_SEC_UNDEFINED:
        cout << "undefined for slot " << slot_label;
        break;
      default:
        cout << "unknown for slot " << slot_label;
    }
  }
  cout << endl;
}


void ResponseHandler::PrintStatusType(uint8_t status_type) {
  cout << "Status Type: ";
  switch (status_type) {
    case ola::rdm::STATUS_NONE:
      cout << "None";
      break;
    case ola::rdm::STATUS_GET_LAST_MESSAGE:
      cout << "Get last messages";
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
    default:
      cout << "Unknown, was " << static_cast<int>(status_type);
  }
  cout << endl;
}


void ResponseHandler::PrintStatusMessageId(uint16_t message_id) {
  cout << "Message ID: ";
  switch (message_id) {
    case ola::rdm::STS_CAL_FAIL:
      cout << "Failed calibration";
      break;
    case ola::rdm::STS_SENS_NOT_FOUND:
      cout << "sensor not found";
      break;
    case ola::rdm::STS_SENS_ALWAYS_ON:
      cout << "sensor always on";
      break;
    case ola::rdm::STS_LAMP_DOUSED:
      cout << "sensor lamp doused";
      break;
    case ola::rdm::STS_LAMP_STRIKE:
      cout << "sensor lamp failed to strike";
      break;
    case ola::rdm::STS_OVERTEMP:
      cout << "over temp";
      break;
    case ola::rdm::STS_UNDERTEMP:
      cout << "under temp";
      break;
    case ola::rdm::STS_SENS_OUT_RANGE:
      cout << "sensor out of range";
      break;
    case ola::rdm::STS_OVERVOLTAGE_PHASE:
      cout << "phase over voltage";
      break;
    case ola::rdm::STS_UNDERVOLTAGE_PHASE:
      cout << "phase under voltage";
      break;
    case ola::rdm::STS_OVERCURRENT:
      cout << "over current";
      break;
    case ola::rdm::STS_UNDERCURRENT:
      cout << "under current";
      break;
    case ola::rdm::STS_PHASE:
      cout << "out of phase";
      break;
    case ola::rdm::STS_PHASE_ERROR:
      cout << "phase error";
      break;
    case ola::rdm::STS_AMPS:
      cout << "amps";
      break;
    case ola::rdm::STS_VOLTS:
      cout << "volts";
      break;
    case ola::rdm::STS_DIMSLOT_OCCUPIED:
      cout << "no dimmer";
      break;
    case ola::rdm::STS_BREAKER_TRIP:
      cout << "breaker tripped";
      break;
    case ola::rdm::STS_WATTS:
      cout << "watts";
      break;
    case ola::rdm::STS_DIM_FAILURE:
      cout << "dimmer failure";
      break;
    case ola::rdm::STS_DIM_PANIC:
      cout << "dimmer panic mode";
      break;
    case ola::rdm::STS_READY:
      cout << "ready";
      break;
    case ola::rdm::STS_NOT_READY:
      cout << "not ready";
      break;
    case ola::rdm::STS_LOW_FLUID:
      cout << "low fluid";
      break;
    default:
      cout << "Unknown, was " << message_id;
  }
  cout << endl;
}

void ResponseHandler::PrintLampState(uint8_t lamp_state) {
  cout << "Lamp State: ";
  switch (lamp_state) {
    case ola::rdm::LAMP_OFF:
      cout << "off";
      break;
    case ola::rdm::LAMP_ON:
      cout << "on";
      break;
    case ola::rdm::LAMP_STRIKE:
      cout << "strike";
      break;
    case ola::rdm::LAMP_STANDBY:
      cout << "standby";
      break;
    case ola::rdm::LAMP_NOT_PRESENT:
      cout << "lamp not present";
      break;
    case ola::rdm::LAMP_ERROR:
      cout << "error";
      break;
    default:
      cout << "Unknown, was " << static_cast<int>(lamp_state);
  }
  cout << endl;
}


void ResponseHandler::PrintLampMode(uint8_t lamp_mode) {
  cout << "Lamp Mode: ";
  switch (lamp_mode) {
    case ola::rdm::LAMP_ON_MODE_OFF:
      cout << "off";
      break;
    case ola::rdm::LAMP_ON_MODE_DMX:
      cout << "dmx";
      break;
    case ola::rdm::LAMP_ON_MODE_ON:
      cout << "on";
      break;
    case ola::rdm::LAMP_ON_MODE_AFTER_CAL:
      cout << "on after calibration";
      break;
    default:
      cout << "Unknown, was " << static_cast<int>(lamp_mode);
  }
  cout << endl;
}


void ResponseHandler::PrintNackReason(uint16_t reason) {
  switch (reason) {
    case ola::rdm::NR_UNKNOWN_PID:
      cout << "Unknown PID";
      break;
    case ola::rdm::NR_FORMAT_ERROR:
      cout << "Format error";
      break;
    case ola::rdm::NR_HARDWARE_FAULT:
      cout << "Hardware fault";
      break;
    case ola::rdm::NR_PROXY_REJECT:
      cout << "Proxy reject";
      break;
    case ola::rdm::NR_WRITE_PROTECT:
      cout << "Write protect";
      break;
    case ola::rdm::NR_UNSUPPORTED_COMMAND_CLASS:
      cout << "Unsupported command class";
      break;
    case ola::rdm::NR_DATA_OUT_OF_RANGE:
      cout << "Data out of range";
      break;
    case ola::rdm::NR_BUFFER_FULL:
      cout << "Buffer full";
      break;
    case ola::rdm::NR_PACKET_SIZE_UNSUPPORTED:
      cout << "Packet size unsupported";
      break;
    case ola::rdm::NR_SUB_DEVICE_OUT_OF_RANGE:
      cout << "Sub device out of range";
      break;
    default:
      cout << "Unknown, was " << reason;
  }
}
// End  implementation
