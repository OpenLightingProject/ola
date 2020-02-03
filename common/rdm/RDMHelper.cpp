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
 * RDMHelper.cpp
 * Various misc RDM functions.
 * Copyright (C) 2010 Simon Newton
 *
 * At some point we may want to localize this file.
 */

#include <sstream>
#include <string>
#include <vector>
#include "ola/rdm/RDMHelper.h"
#include "ola/StringUtils.h"

namespace ola {
namespace rdm {

using std::ostringstream;
using std::string;
using std::vector;


/**
 * Convert a RDMStatusCode to a string
 */
string StatusCodeToString(RDMStatusCode status) {
  switch (status) {
    case RDM_COMPLETED_OK:
      return "Completed Ok";
    case RDM_WAS_BROADCAST:
      return "Request was broadcast";
    case RDM_FAILED_TO_SEND:
      return "Failed to send request";
    case RDM_TIMEOUT:
      return "Response Timeout";
    case RDM_INVALID_RESPONSE:
      return "Invalid Response";
    case RDM_UNKNOWN_UID:
      return "The RDM device could not be found";
    case RDM_CHECKSUM_INCORRECT:
      return "Incorrect checksum";
    case RDM_TRANSACTION_MISMATCH:
      return "Transaction number mismatch";
    case RDM_SUB_DEVICE_MISMATCH:
      return "Sub device mismatch";
    case RDM_SRC_UID_MISMATCH:
      return "Source UID in response doesn't match";
    case RDM_DEST_UID_MISMATCH:
      return "Destination UID in response doesn't match";
    case RDM_WRONG_SUB_START_CODE:
      return "Incorrect sub start code";
    case RDM_PACKET_TOO_SHORT:
      return "RDM response was smaller than the minimum size";
    case RDM_PACKET_LENGTH_MISMATCH:
      return "The length field of packet didn't match length received";
    case RDM_PARAM_LENGTH_MISMATCH:
      return "The parameter length exceeds the remaining packet size";
    case RDM_INVALID_COMMAND_CLASS:
      return "The command class was not one of GET_RESPONSE or SET_RESPONSE";
    case RDM_COMMAND_CLASS_MISMATCH:
      return "The command class didn't match the request";
    case RDM_INVALID_RESPONSE_TYPE:
      return "The response type was not ACK, ACK_OVERFLOW, ACK_TIMER or NACK";
    case RDM_PLUGIN_DISCOVERY_NOT_SUPPORTED:
      return "The output plugin does not support DISCOVERY commands";
    case RDM_DUB_RESPONSE:
      return "DUB response";
    default:
      return "Unknown";
  }
}


/**
 * Convert a uint8_t representing a data type to a human-readable string.
 * @param type the data type value
 */
string DataTypeToString(uint8_t type) {
  switch (type) {
    case DS_NOT_DEFINED:
      return "Not defined";
    case DS_BIT_FIELD:
      return "Bit field";
    case DS_ASCII:
      return "ASCII";
    case DS_UNSIGNED_BYTE:
      return "uint8";
    case DS_SIGNED_BYTE:
      return "int8";
    case DS_UNSIGNED_WORD:
      return "uint16";
    case DS_SIGNED_WORD:
      return "int16";
    case DS_UNSIGNED_DWORD:
      return "uint32";
    case DS_SIGNED_DWORD:
      return "int32";
    default:
      ostringstream str;
      str << "Unknown, was " << static_cast<int>(type);
      return str.str();
  }
}


/**
 * Convert a uint8_t representing a lamp mode to a human-readable string.
 * @param lamp_mode the lamp mode value
 */
string LampModeToString(uint8_t lamp_mode) {
  switch (lamp_mode) {
    case LAMP_ON_MODE_OFF:
      return "Off";
    case LAMP_ON_MODE_DMX:
      return "DMX";
    case LAMP_ON_MODE_ON:
      return "On";
    case LAMP_ON_MODE_ON_AFTER_CAL:
      return "On after calibration";
    default:
      ostringstream str;
      str << "Unknown, was " << static_cast<int>(lamp_mode);
      return str.str();
  }
}


/**
 * Convert a uint8_t representing a lamp state to a human-readable string.
 * @param lamp_state the lamp state value
 */
string LampStateToString(uint8_t lamp_state) {
  switch (lamp_state) {
    case LAMP_OFF:
      return "Off";
    case LAMP_ON:
      return "On";
    case LAMP_STRIKE:
      return "Strike";
    case LAMP_STANDBY:
      return "Standby";
    case LAMP_NOT_PRESENT:
      return "Lamp not present";
    case LAMP_ERROR:
      return "Error";
    default:
      ostringstream str;
      str << "Unknown, was " << static_cast<int>(lamp_state);
      return str.str();
  }
}


/**
 * Convert a uint16_t representing a nack reason to a human-readable string.
 * @param reason the nack reason value
 */
string NackReasonToString(uint16_t reason) {
  switch (reason) {
    case NR_UNKNOWN_PID:
      return "Unknown PID";
    case NR_FORMAT_ERROR:
      return "Format error";
    case NR_HARDWARE_FAULT:
      return "Hardware fault";
    case NR_PROXY_REJECT:
      return "Proxy reject";
    case NR_WRITE_PROTECT:
      return "Write protect";
    case NR_UNSUPPORTED_COMMAND_CLASS:
      return "Unsupported command class";
    case NR_DATA_OUT_OF_RANGE:
      return "Data out of range";
    case NR_BUFFER_FULL:
      return "Buffer full";
    case NR_PACKET_SIZE_UNSUPPORTED:
      return "Packet size unsupported";
    case NR_SUB_DEVICE_OUT_OF_RANGE:
      return "Sub device out of range";
    case NR_PROXY_BUFFER_FULL:
      return "Proxy buffer full";
    case NR_ACTION_NOT_SUPPORTED:
      return "Action not supported";
    case NR_ENDPOINT_NUMBER_INVALID:
      return "Endpoint number invalid";
    case NR_INVALID_ENDPOINT_MODE:
      return "Invalid endpoint mode";
    case NR_UNKNOWN_UID:
      return "Unknown UID";
    case NR_UNKNOWN_SCOPE:
      return "Unknown scope";
    case NR_INVALID_STATIC_CONFIG_TYPE:
      return "Invalid static config type";
    case NR_INVALID_IPV4_ADDRESS:
      return "Invalid IPv4 address";
    case NR_INVALID_IPV6_ADDRESS:
      return "Invalid IPv6 address";
    case NR_INVALID_PORT:
      return "Invalid port";
    default:
      ostringstream str;
      str << "Unknown, was " << reason;
      return str.str();
  }
}


/**
 * Convert a uint8_t representing a power state to a human-readable string.
 * @param power_state the power state value
 */
string PowerStateToString(uint8_t power_state) {
  switch (power_state) {
    case POWER_STATE_FULL_OFF:
      return "Full Off";
    case POWER_STATE_SHUTDOWN:
      return "Shutdown";
    case POWER_STATE_STANDBY:
      return "Standby";
    case POWER_STATE_NORMAL:
      return "Normal";
    default:
      ostringstream str;
      str << "Unknown, was " << static_cast<int>(power_state);
      return str.str();
  }
}


/**
 * Safely convert a uint8_t to a rdm_power_state
 */
bool UIntToPowerState(uint8_t state, rdm_power_state *power_state) {
  switch (state) {
    case POWER_STATE_FULL_OFF:
      *power_state = POWER_STATE_FULL_OFF;
      return true;
    case POWER_STATE_SHUTDOWN:
      *power_state = POWER_STATE_SHUTDOWN;
      return true;
    case POWER_STATE_STANDBY:
      *power_state = POWER_STATE_STANDBY;
      return true;
    case POWER_STATE_NORMAL:
      *power_state = POWER_STATE_NORMAL;
      return true;
    default:
      return false;
  }
}


/**
 * Convert a uint8 representing a prefix to a human-readable string.
 * @param prefix the prefix value
 */
string PrefixToString(uint8_t prefix) {
  switch (prefix) {
    case PREFIX_NONE:
      return "";
    case PREFIX_DECI:
      return "Deci";
    case PREFIX_CENTI:
      return "Centi";
    case PREFIX_MILLI:
      return "Milli";
    case PREFIX_MICRO:
      return "Micro";
    case PREFIX_NANO:
      return "Nano";
    case PREFIX_PICO:
      return "Pico";
    case PREFIX_FEMTO:
      return "Femto";
    case PREFIX_ATTO:
      return "Atto";
    case PREFIX_ZEPTO:
      return "Zepto";
    case PREFIX_YOCTO:
      return "Yocto";
    case PREFIX_DECA:
      return "Deca";
    case PREFIX_HECTO:
      return "Hecto";
    case PREFIX_KILO:
      return "Kilo";
    case PREFIX_MEGA:
      return "Mega";
    case PREFIX_GIGA:
      return "Giga";
    case PREFIX_TERA:
      return "Tera";
    case PREFIX_PETA:
      return "Peta";
    case PREFIX_EXA:
      return "Exa";
    case PREFIX_ZETTA:
      return "Zetta";
    case PREFIX_YOTTA:
      return "Yotta";
    default:
      ostringstream str;
      str << "Unknown, was " << static_cast<int>(prefix);
      return str.str();
  }
}


/**
 * Convert a uint16_t representing a product category to a human-readable
 *   string.
 * @param category the product category value
 */
string ProductCategoryToString(uint16_t category) {
  switch (category) {
    case PRODUCT_CATEGORY_NOT_DECLARED:
      return "Not declared";
    case PRODUCT_CATEGORY_FIXTURE:
      return "Fixture";
    case PRODUCT_CATEGORY_FIXTURE_FIXED:
      return "Fixed fixture";
    case PRODUCT_CATEGORY_FIXTURE_MOVING_YOKE:
      return "Moving yoke fixture";
    case PRODUCT_CATEGORY_FIXTURE_MOVING_MIRROR:
      return "Moving mirror fixture";
    case PRODUCT_CATEGORY_FIXTURE_OTHER:
      return "Fixture other";
    case PRODUCT_CATEGORY_FIXTURE_ACCESSORY:
      return "Fixture accessory";
    case PRODUCT_CATEGORY_FIXTURE_ACCESSORY_COLOR:
      return "Fixture accessory color";
    case PRODUCT_CATEGORY_FIXTURE_ACCESSORY_YOKE:
      return "Fixture accessory yoke";
    case PRODUCT_CATEGORY_FIXTURE_ACCESSORY_MIRROR:
      return "Fixture accessory mirror";
    case PRODUCT_CATEGORY_FIXTURE_ACCESSORY_EFFECT:
      return "Fixture accessory effect";
    case PRODUCT_CATEGORY_FIXTURE_ACCESSORY_BEAM:
      return "Fixture accessory beam";
    case PRODUCT_CATEGORY_FIXTURE_ACCESSORY_OTHER:
      return "Fixture accessory other";
    case PRODUCT_CATEGORY_PROJECTOR:
      return "Projector";
    case PRODUCT_CATEGORY_PROJECTOR_FIXED:
      return "Projector fixed";
    case PRODUCT_CATEGORY_PROJECTOR_MOVING_YOKE:
      return "Projector moving yoke";
    case PRODUCT_CATEGORY_PROJECTOR_MOVING_MIRROR:
      return "Projector moving mirror";
    case PRODUCT_CATEGORY_PROJECTOR_OTHER:
      return "Projector other";
    case PRODUCT_CATEGORY_ATMOSPHERIC:
      return "Atmospheric";
    case PRODUCT_CATEGORY_ATMOSPHERIC_EFFECT:
      return "Atmospheric effect";
    case PRODUCT_CATEGORY_ATMOSPHERIC_PYRO:
      return "Atmospheric pyro";
    case PRODUCT_CATEGORY_ATMOSPHERIC_OTHER:
      return "Atmospheric other";
    case PRODUCT_CATEGORY_DIMMER:
      return "Dimmer";
    case PRODUCT_CATEGORY_DIMMER_AC_INCANDESCENT:
      return "Dimmer AC incandescent";
    case PRODUCT_CATEGORY_DIMMER_AC_FLUORESCENT:
      return "Dimmer AC fluorescent";
    case PRODUCT_CATEGORY_DIMMER_AC_COLDCATHODE:
      return "Dimmer AC cold cathode";
    case PRODUCT_CATEGORY_DIMMER_AC_NONDIM:
      return "Dimmer AC no dim";
    case PRODUCT_CATEGORY_DIMMER_AC_ELV:
      return "Dimmer AC ELV";
    case PRODUCT_CATEGORY_DIMMER_AC_OTHER:
      return "Dimmer AC other";
    case PRODUCT_CATEGORY_DIMMER_DC_LEVEL:
      return "Dimmer DC level";
    case PRODUCT_CATEGORY_DIMMER_DC_PWM:
      return "Dimmer DC PWM";
    case PRODUCT_CATEGORY_DIMMER_CS_LED:
      return "Dimmer DC LED";
    case PRODUCT_CATEGORY_DIMMER_OTHER:
      return "Dimmer other";
    case PRODUCT_CATEGORY_POWER:
      return "Power";
    case PRODUCT_CATEGORY_POWER_CONTROL:
      return "Power control";
    case PRODUCT_CATEGORY_POWER_SOURCE:
      return "Power source";
    case PRODUCT_CATEGORY_POWER_OTHER:
      return "Power other";
    case PRODUCT_CATEGORY_SCENIC:
      return "Scenic";
    case PRODUCT_CATEGORY_SCENIC_DRIVE:
      return "Scenic drive";
    case PRODUCT_CATEGORY_SCENIC_OTHER:
      return "Scenic other";
    case PRODUCT_CATEGORY_DATA:
      return "Data";
    case PRODUCT_CATEGORY_DATA_DISTRIBUTION:
      return "Data distribution";
    case PRODUCT_CATEGORY_DATA_CONVERSION:
      return "Data conversion";
    case PRODUCT_CATEGORY_DATA_OTHER:
      return "Data other";
    case PRODUCT_CATEGORY_AV:
      return "A/V";
    case PRODUCT_CATEGORY_AV_AUDIO:
      return "A/V audio";
    case PRODUCT_CATEGORY_AV_VIDEO:
      return "A/V video";
    case PRODUCT_CATEGORY_AV_OTHER:
      return "AV other";
    case PRODUCT_CATEGORY_MONITOR:
      return "Monitor";
    case PRODUCT_CATEGORY_MONITOR_ACLINEPOWER:
      return "AC line power monitor";
    case PRODUCT_CATEGORY_MONITOR_DCPOWER:
      return "DC power monitor";
    case PRODUCT_CATEGORY_MONITOR_ENVIRONMENTAL:
      return "Environmental monitor";
    case PRODUCT_CATEGORY_MONITOR_OTHER:
      return "Other monitor";
    case PRODUCT_CATEGORY_CONTROL:
      return "Control";
    case PRODUCT_CATEGORY_CONTROL_CONTROLLER:
      return "Controller";
    case PRODUCT_CATEGORY_CONTROL_BACKUPDEVICE:
      return "Backup device";
    case PRODUCT_CATEGORY_CONTROL_OTHER:
      return "Other control";
    case PRODUCT_CATEGORY_TEST:
      return "Test";
    case PRODUCT_CATEGORY_TEST_EQUIPMENT:
      return "Test equipment";
    case PRODUCT_CATEGORY_TEST_EQUIPMENT_OTHER:
      return "Test equipment other";
    case PRODUCT_CATEGORY_OTHER:
      return "Other";
    default:
      ostringstream str;
      str << "Unknown, was " << static_cast<int>(category);
      return str.str();
  }
}


/**
 * Convert a uint16_t representing a product detail to a human-readable string.
 * @param detail the product detail value.
 */
string ProductDetailToString(uint16_t detail) {
  switch (detail) {
    case PRODUCT_DETAIL_NOT_DECLARED:
      return "Not declared";
    case PRODUCT_DETAIL_ARC:
      return "Arc Lamp";
    case PRODUCT_DETAIL_METAL_HALIDE:
      return "Metal Halide Lamp";
    case PRODUCT_DETAIL_INCANDESCENT:
      return "Incandescent Lamp";
    case PRODUCT_DETAIL_LED:
      return "LED";
    case PRODUCT_DETAIL_FLUORESCENT:
      return "Fluorescent";
    case PRODUCT_DETAIL_COLDCATHODE:
      return "Cold Cathode";
    case PRODUCT_DETAIL_ELECTROLUMINESCENT:
      return "Electro-luminescent";
    case PRODUCT_DETAIL_LASER:
      return "Laser";
    case PRODUCT_DETAIL_FLASHTUBE:
      return "Flash Tube";
    case PRODUCT_DETAIL_COLORSCROLLER:
      return "Color Scroller";
    case PRODUCT_DETAIL_COLORWHEEL:
      return "Color Wheel";
    case PRODUCT_DETAIL_COLORCHANGE:
      return "Color Changer (Semaphore or other type)";
    case PRODUCT_DETAIL_IRIS_DOUSER:
      return "Iris";
    case PRODUCT_DETAIL_DIMMING_SHUTTER:
      return "Dimming Shuttle";
    case PRODUCT_DETAIL_PROFILE_SHUTTER:
      return "Profile Shuttle";
    case PRODUCT_DETAIL_BARNDOOR_SHUTTER:
      return "Barndoor Shuttle";
    case PRODUCT_DETAIL_EFFECTS_DISC:
      return "Effects Disc";
    case PRODUCT_DETAIL_GOBO_ROTATOR:
      return "Gobo Rotator";
    case PRODUCT_DETAIL_VIDEO:
      return "Video";
    case PRODUCT_DETAIL_SLIDE:
      return "Slide";
    case PRODUCT_DETAIL_FILM:
      return "Film";
    case PRODUCT_DETAIL_OILWHEEL:
      return "Oil Wheel";
    case PRODUCT_DETAIL_LCDGATE:
      return "LCD Gate";
    case PRODUCT_DETAIL_FOGGER_GLYCOL:
      return "Fogger, Glycol";
    case PRODUCT_DETAIL_FOGGER_MINERALOIL:
      return "Fogger, Mineral Oil";
    case PRODUCT_DETAIL_FOGGER_WATER:
      return "Fogger, Water";
    case PRODUCT_DETAIL_CO2:
      return "Dry Ice/ Carbon Dioxide Device";
    case PRODUCT_DETAIL_LN2:
      return "Nitrogen based";
    case PRODUCT_DETAIL_BUBBLE:
      return "Bubble or Foam Machine";
    case PRODUCT_DETAIL_FLAME_PROPANE:
      return "Propane Flame";
    case PRODUCT_DETAIL_FLAME_OTHER:
      return "Other Flame";
    case PRODUCT_DETAIL_OLEFACTORY_STIMULATOR:
      return "Scents";
    case PRODUCT_DETAIL_SNOW:
      return "Snow Machine";
    case PRODUCT_DETAIL_WATER_JET:
      return "Water Jet";
    case PRODUCT_DETAIL_WIND:
      return "Wind Machine";
    case PRODUCT_DETAIL_CONFETTI:
      return "Confetti Machine";
    case PRODUCT_DETAIL_HAZARD:
      return "Hazard (Any form of pyrotechnic control or device.)";
    case PRODUCT_DETAIL_PHASE_CONTROL:
      return "Phase Control";
    case PRODUCT_DETAIL_REVERSE_PHASE_CONTROL:
      return "Phase Angle";
    case PRODUCT_DETAIL_SINE:
      return "Sine";
    case PRODUCT_DETAIL_PWM:
      return "PWM";
    case PRODUCT_DETAIL_DC:
      return "DC";
    case PRODUCT_DETAIL_HFBALLAST:
      return "HF Ballast";
    case PRODUCT_DETAIL_HFHV_NEONBALLAST:
      return "HFHV Neon/Argon";
    case PRODUCT_DETAIL_HFHV_EL:
      return "HFHV Electroluminscent";
    case PRODUCT_DETAIL_MHR_BALLAST:
      return "Metal Halide Ballast";
    case PRODUCT_DETAIL_BITANGLE_MODULATION:
      return "Bit Angle Modulation";
    case PRODUCT_DETAIL_FREQUENCY_MODULATION:
      return "Frequency Modulation";
    case PRODUCT_DETAIL_HIGHFREQUENCY_12V:
      return "High Frequency 12V";
    case PRODUCT_DETAIL_RELAY_MECHANICAL:
      return "Mechanical Relay";
    case PRODUCT_DETAIL_RELAY_ELECTRONIC:
      return "Electronic Relay";
    case PRODUCT_DETAIL_SWITCH_ELECTRONIC:
      return "Electronic Switch";
    case PRODUCT_DETAIL_CONTACTOR:
      return "Contactor";
    case PRODUCT_DETAIL_MIRRORBALL_ROTATOR:
      return "Mirror Ball Rotator";
    case PRODUCT_DETAIL_OTHER_ROTATOR:
      return "Other Rotator";
    case PRODUCT_DETAIL_KABUKI_DROP:
      return "Kabuki Drop";
    case PRODUCT_DETAIL_CURTAIN:
      return "Curtain";
    case PRODUCT_DETAIL_LINESET:
      return "Line Set";
    case PRODUCT_DETAIL_MOTOR_CONTROL:
      return "Motor Control";
    case PRODUCT_DETAIL_DAMPER_CONTROL:
      return "Damper Control";
    case PRODUCT_DETAIL_SPLITTER:
      return "Splitter";
    case PRODUCT_DETAIL_ETHERNET_NODE:
      return "Ethernet Node";
    case PRODUCT_DETAIL_MERGE:
      return "DMX512 Merger";
    case PRODUCT_DETAIL_DATAPATCH:
      return "Data Patch";
    case PRODUCT_DETAIL_WIRELESS_LINK:
      return "Wireless link";
    case PRODUCT_DETAIL_PROTOCOL_CONVERTER:
      return "Protocol Converter";
    case PRODUCT_DETAIL_ANALOG_DEMULTIPLEX:
      return "DMX512 to DC Voltage";
    case PRODUCT_DETAIL_ANALOG_MULTIPLEX:
      return "DC Voltage to DMX512";
    case PRODUCT_DETAIL_SWITCH_PANEL:
      return "Switch Panel";
    case PRODUCT_DETAIL_ROUTER:
      return "Router";
    case PRODUCT_DETAIL_FADER:
      return "Fader, Single Channel";
    case PRODUCT_DETAIL_MIXER:
      return "Mixer, Multi Channel";
    case PRODUCT_DETAIL_CHANGEOVER_MANUAL:
      return "Manual Changeover";
    case PRODUCT_DETAIL_CHANGEOVER_AUTO:
      return "Auto Changeover";
    case PRODUCT_DETAIL_TEST:
      return "Test Device";
    case PRODUCT_DETAIL_GFI_RCD:
      return "GFI / RCD Device";
    case PRODUCT_DETAIL_BATTERY:
      return "Battery";
    case PRODUCT_DETAIL_CONTROLLABLE_BREAKER:
      return "Controllable Breaker";
    case PRODUCT_DETAIL_OTHER:
      return "Other Device";
    default:
      ostringstream str;
      str << "Unknown, was " << detail;
      return str.str();
  }
}


/**
 * Convert a uint8_t representing a reset device to a human-readable string.
 * @param reset_device the reset device value
 */
string ResetDeviceToString(uint8_t reset_device) {
  switch (reset_device) {
    case RESET_WARM:
      return "Warm";
    case RESET_COLD:
      return "Cold";
    default:
      ostringstream str;
      str << "Unknown, was " << static_cast<int>(reset_device);
      return str.str();
  }
}


/**
 * Safely convert a uint8_t to a rdm_reset_device_mode
 */
bool UIntToResetDevice(uint8_t state, rdm_reset_device_mode *reset_device) {
  switch (state) {
    case RESET_WARM:
      *reset_device = RESET_WARM;
      return true;
    case RESET_COLD:
      *reset_device = RESET_COLD;
      return true;
    default:
      return false;
  }
}


/**
 * Convert a uint8_t representing a sensor type to a human-readable string.
 * @param type the sensor type value
 */
string SensorTypeToString(uint8_t type) {
  switch (type) {
    case SENSOR_TEMPERATURE:
      return "Temperature";
    case SENSOR_VOLTAGE:
      return "Voltage";
    case SENSOR_CURRENT:
      return "Current";
    case SENSOR_FREQUENCY:
      return "Frequency";
    case SENSOR_RESISTANCE:
      return "Resistance";
    case SENSOR_POWER:
      return "Power";
    case SENSOR_MASS:
      return "Mass";
    case SENSOR_LENGTH:
      return "Length";
    case SENSOR_AREA:
      return "Area";
    case SENSOR_VOLUME:
      return "Volume";
    case SENSOR_DENSITY:
      return "Density";
    case SENSOR_VELOCITY:
      return "Velocity";
    case SENSOR_ACCELERATION:
      return "Acceleration";
    case SENSOR_FORCE:
      return "Force";
    case SENSOR_ENERGY:
      return "Energy";
    case SENSOR_PRESSURE:
      return "Pressure";
    case SENSOR_TIME:
      return "Time";
    case SENSOR_ANGLE:
      return "Angle";
    case SENSOR_POSITION_X:
      return "Position X";
    case SENSOR_POSITION_Y:
      return "Position Y";
    case SENSOR_POSITION_Z:
      return "Position Z";
    case SENSOR_ANGULAR_VELOCITY:
      return "Angular velocity";
    case SENSOR_LUMINOUS_INTENSITY:
      return "Luminous intensity";
    case SENSOR_LUMINOUS_FLUX:
      return "Luminous flux";
    case SENSOR_ILLUMINANCE:
      return "Illuminance";
    case SENSOR_CHROMINANCE_RED:
      return "Chrominance red";
    case SENSOR_CHROMINANCE_GREEN:
      return "Chrominance green";
    case SENSOR_CHROMINANCE_BLUE:
      return "Chrominance blue";
    case SENSOR_CONTACTS:
      return "Contacts";
    case SENSOR_MEMORY:
      return "Memory";
    case SENSOR_ITEMS:
      return "Items";
    case SENSOR_HUMIDITY:
      return "Humidity";
    case SENSOR_COUNTER_16BIT:
      return "16 bit counter";
    case SENSOR_OTHER:
      return "Other";
    default:
      ostringstream str;
      str << "Unknown, was " << static_cast<int>(type);
      return str.str();
  }
}


/**
 * Convert a uint8_t representing a sensor's recording support to a
 * human-readable string.
 * @param supports_recording the sensor recording support bitmask
 */
string SensorSupportsRecordingToString(uint8_t supports_recording) {
  vector<string> recording_support;
  if (supports_recording & SENSOR_RECORDED_VALUE) {
    recording_support.push_back("Recorded Value");
  }
  if (supports_recording & SENSOR_RECORDED_RANGE_VALUES) {
    recording_support.push_back("Lowest/Highest Detected Values");
  }
  return StringJoin(", ", recording_support);
}


/**
 * Convert a uint16_t representing a slot type to a human-readable string.
 * @param slot_type the type of the slot.
 * @param slot_label the label for the slot.
 */
string SlotInfoToString(uint8_t slot_type, uint16_t slot_label) {
  if (slot_type == ST_PRIMARY) {
    switch (slot_label) {
      case SD_INTENSITY:
        return "Primary, intensity";
      case SD_INTENSITY_MASTER:
        return "Primary, intensity master";
      case SD_PAN:
        return "Primary, pan";
      case SD_TILT:
        return "Primary, tilt";
      case SD_COLOR_WHEEL:
        return "Primary, color wheel";
      case SD_COLOR_SUB_CYAN:
        return "Primary, subtractive cyan";
      case SD_COLOR_SUB_YELLOW:
        return "Primary, subtractive yellow";
      case SD_COLOR_SUB_MAGENTA:
        return "Primary, subtractive magenta";
      case SD_COLOR_ADD_RED:
        return "Primary, additive red";
      case SD_COLOR_ADD_GREEN:
        return "Primary, additive green";
      case SD_COLOR_ADD_BLUE:
        return "Primary, additive blue";
      case SD_COLOR_CORRECTION:
        return "Primary, color correction";
      case SD_COLOR_SCROLL:
        return "Primary, scroll";
      case SD_COLOR_SEMAPHORE:
        return "Primary, color semaphore";
      case SD_COLOR_ADD_AMBER:
        return "Primary, additive amber";
      case SD_COLOR_ADD_WHITE:
        return "Primary, additive white";
      case SD_COLOR_ADD_WARM_WHITE:
        return "Primary, additive warm white";
      case SD_COLOR_ADD_COOL_WHITE:
        return "Primary, additive cool white";
      case SD_COLOR_SUB_UV:
        return "Primary, subtractive UV";
      case SD_COLOR_HUE:
        return "Primary, hue";
      case SD_COLOR_SATURATION:
        return "Primary, saturation";
      case SD_STATIC_GOBO_WHEEL:
        return "Primary, static gobo wheel";
      case SD_ROTO_GOBO_WHEEL:
        return "Primary, gobo wheel";
      case SD_PRISM_WHEEL:
        return "Primary, prism wheel";
      case SD_EFFECTS_WHEEL:
        return "Primary, effects wheel";
      case SD_BEAM_SIZE_IRIS:
        return "Primary, iris size";
      case SD_EDGE:
        return "Primary, edge";
      case SD_FROST:
        return "Primary, frost";
      case SD_STROBE:
        return "Primary, strobe";
      case SD_ZOOM:
        return "Primary, zoom";
      case SD_FRAMING_SHUTTER:
        return "Primary, framing shutter";
      case SD_SHUTTER_ROTATE:
        return "Primary, shuttle rotate";
      case SD_DOUSER:
        return "Primary, douser";
      case SD_BARN_DOOR:
        return "Primary, barn door";
      case SD_LAMP_CONTROL:
        return "Primary, lamp control";
      case SD_FIXTURE_CONTROL:
        return "Primary, fixture control";
      case SD_FIXTURE_SPEED:
        return "Primary, fixture speed";
      case SD_MACRO:
        return "Primary, macro";
      case SD_POWER_CONTROL:
        return "Primary, relay or power control";
      case SD_FAN_CONTROL:
        return "Primary, fan control";
      case SD_HEATER_CONTROL:
        return "Primary, heater control";
      case SD_FOUNTAIN_CONTROL:
        return "Primary, fountain water pump control";
      case SD_UNDEFINED:
        return "Primary, undefined";
      default:
        ostringstream str;
        str << "Primary, unknown, was " << slot_label;
        return str.str();
    }
  } else {
    ostringstream str;
    str << "Secondary, ";
    switch (slot_type) {
      case ST_SEC_FINE:
        str << "fine control for slot " << slot_label;
        break;
      case ST_SEC_TIMING:
        str << "timing control for slot " << slot_label;
        break;
      case ST_SEC_SPEED:
        str << "speed control for slot " << slot_label;
        break;
      case ST_SEC_CONTROL:
        str << "mode control for slot " << slot_label;
        break;
      case ST_SEC_INDEX:
        str << "index control for slot " << slot_label;
        break;
      case ST_SEC_ROTATION:
        str << "rotation speed control for slot " << slot_label;
        break;
      case ST_SEC_INDEX_ROTATE:
        str << "rotation index control for slot " << slot_label;
        break;
      case ST_SEC_UNDEFINED:
        str << "undefined for slot " << slot_label;
        break;
      default:
        str << "unknown, was type " << static_cast<int>(slot_type)
            << ", for slot " << slot_label;
    }
    return str.str();
  }
}


/**
 * Convert a uint16_t representing a status message to a human-readable string.
 * @param message_id the status message value
 * @param data1 the first data value for the message
 * @param data2 the second data value for the message
 */
string StatusMessageIdToString(uint16_t message_id,
                               int16_t data1,
                               int16_t data2) {
  ostringstream str;
  switch (message_id) {
    case STS_CAL_FAIL:
      str << "Slot " << data1 << " failed calibration";
      break;
    case STS_SENS_NOT_FOUND:
      str << "Sensor " << data1 << " not found";
      break;
    case STS_SENS_ALWAYS_ON:
      str << "Sensor " << data1 << " always on";
      break;
    case STS_FEEDBACK_ERROR:
      str << "Slot " << data1 << " feedback error";
      break;
    case STS_INDEX_ERROR:
      str << "Slot " << data1 << " index circuit error";
      break;
    case STS_LAMP_DOUSED:
      str << "Lamp doused";
      break;
    case STS_LAMP_STRIKE:
      str<< "Lamp failed to strike";
      break;
    case STS_LAMP_ACCESS_OPEN:
      str << "Lamp access open";
      break;
    case STS_LAMP_ALWAYS_ON:
      str << "Lamp on without command";
      break;
    case STS_OVERTEMP:
      str << "Sensor " << data1 << " over temp at " << data2 << " degrees C";
      break;
    case STS_UNDERTEMP:
      str << "Sensor " << data1 << " under temp at " << data2 << " degrees C";
      break;
    case STS_SENS_OUT_RANGE:
      str << "Sensor " << data1 << " out of range";
      break;
    case STS_OVERVOLTAGE_PHASE:
      str << "Phase " << data1 << " over voltage at " << data2 << "V";
      break;
    case STS_UNDERVOLTAGE_PHASE:
      str << "Phase " << data1 << " under voltage at " << data2 << "V";
      break;
    case STS_OVERCURRENT:
      str << "Phase " << data1 << " over current at " << data2 << "V";
      break;
    case STS_UNDERCURRENT:
      str << "Phase " << data1 << " under current at " << data2 << "V";
      break;
    case STS_PHASE:
      str << "Phase " << data1 << " is at " << data2 << " degrees";
      break;
    case STS_PHASE_ERROR:
      str << "Phase " << data1 << " error";
      break;
    case STS_AMPS:
      str << data1 <<  " Amps";
      break;
    case STS_VOLTS:
      str << data1 <<  " Volts";
      break;
    case STS_DIMSLOT_OCCUPIED:
      str << "No Dimmer";
      break;
    case STS_BREAKER_TRIP:
      str <<  "Tripped Breaker";
      break;
    case STS_WATTS:
      str << data1 <<  " Watts";
      break;
    case STS_DIM_FAILURE:
      str << "Dimmer Failure";
      break;
    case STS_DIM_PANIC:
      str << "Dimmer panic mode";
      break;
    case STS_LOAD_FAILURE:
      str << "Lamp or cable failure";
      break;
    case STS_READY:
      str << "Slot " << data1 << " ready";
      break;
    case STS_NOT_READY:
      str << "Slot " << data1 << " not ready";
      break;
    case STS_LOW_FLUID:
      str << "Slot " << data1 << " low fluid";
      break;
    case STS_EEPROM_ERROR:
      str << "EEPROM error";
      break;
    case STS_RAM_ERROR:
      str << "RAM error";
      break;
    case STS_FPGA_ERROR:
      str << "FPGA programming error";
      break;
    case STS_PROXY_BROADCAST_DROPPED:
      // This is technically against the standard, which in 10.3.2.4 says "Each
      // Data Value shall be a signed integer." but I'm sure it's what was
      // intended. The same thing is technically true with the slots too.
      str << "Proxy Drop: PID "
          << strings::ToHex(reinterpret_cast<uint16_t&>(data1)) << " at TN "
          << data2;
      break;
    case STS_ASC_RXOK:
      str << "DMX ASC " << strings::ToHex(reinterpret_cast<uint16_t&>(data1))
          << " received OK";
      break;
    case STS_ASC_DROPPED:
      str << "DMX ASC " << strings::ToHex(reinterpret_cast<uint16_t&>(data1))
          << " now dropped";
      break;
    case STS_DMXNSCNONE:
      str << "DMX NSC never received";
      break;
    case STS_DMXNSCLOSS:
      str << "DMX NSC received, now dropped";
      break;
    case STS_DMXNSCERROR:
      str << "DMX NSC timing or packet error";
      break;
    case STS_DMXNSC_OK:
      str << "DMX NSC received OK";
      break;
    default:
      str << "Unknown, was status message " << message_id << " with data value"
             " 1 " << data1 << " and data value 2 " << data2;
      break;
  }
  return str.str();
}


/**
 * Convert a uint8_t representing a status type to a human-readable string.
 * @param status_type the status type value
 */
string StatusTypeToString(uint8_t status_type) {
  switch (status_type) {
    case STATUS_NONE:
      return "None";
    case STATUS_GET_LAST_MESSAGE:
      return "Get last messages";
    case STATUS_ADVISORY:
      return "Advisory";
    case STATUS_WARNING:
      return "Warning";
    case STATUS_ERROR:
      return "Error";
    case STATUS_ADVISORY_CLEARED:
      return "Advisory cleared";
    case STATUS_WARNING_CLEARED:
      return "Warning cleared";
    case STATUS_ERROR_CLEARED:
      return "Error cleared";
    default:
      ostringstream str;
      str << "Unknown, was " << static_cast<int>(status_type);
      return str.str();
  }
}



/**
 * Convert a uint8_t representing a unit to a human-readable string.
 * @param unit the unit value
 */
string UnitToString(uint8_t unit) {
  switch (unit) {
    case UNITS_NONE:
      return "";
    case UNITS_CENTIGRADE:
      return "degrees C";
    case UNITS_VOLTS_DC:
      return "Volts (DC)";
    case UNITS_VOLTS_AC_PEAK:
      return "Volts (AC Peak)";
    case UNITS_VOLTS_AC_RMS:
      return "Volts (AC RMS)";
    case UNITS_AMPERE_DC:
      return "Amps (DC)";
    case UNITS_AMPERE_AC_PEAK:
      return "Amps (AC Peak)";
    case UNITS_AMPERE_AC_RMS:
      return "Amps (AC RMS)";
    case UNITS_HERTZ:
      return "Hz";
    case UNITS_OHM:
      return "ohms";
    case UNITS_WATT:
      return "W";
    case UNITS_KILOGRAM:
      return "kg";
    case UNITS_METERS:
      return "m";
    case UNITS_METERS_SQUARED:
      return "m^2";
    case UNITS_METERS_CUBED:
      return "m^3";
    case UNITS_KILOGRAMMES_PER_METER_CUBED:
      return "kg/m^3";
    case UNITS_METERS_PER_SECOND:
      return "m/s";
    case UNITS_METERS_PER_SECOND_SQUARED:
      return "m/s^2";
    case UNITS_NEWTON:
      return "newton";
    case UNITS_JOULE:
      return "joule";
    case UNITS_PASCAL:
      return "pascal";
    case UNITS_SECOND:
      return "second";
    case UNITS_DEGREE:
      return "degree";
    case UNITS_STERADIAN:
      return "steradian";
    case UNITS_CANDELA:
      return "candela";
    case UNITS_LUMEN:
      return "lumen";
    case UNITS_LUX:
      return "lux";
    case UNITS_IRE:
      return "ire";
    case UNITS_BYTE:
      return "bytes";
    default:
      ostringstream str;
      str << "Unknown, was " << static_cast<int>(unit);
      return str.str();
  }
}
}  // namespace rdm
}  // namespace  ola
