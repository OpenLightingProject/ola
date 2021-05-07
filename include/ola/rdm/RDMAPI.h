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
 * RDMAPI.h
 * Provide a generic RDM API that can use different implementations.
 * Copyright (C) 2010 Simon Newton
 *
 * This class provides a high level C++ RDM API for PIDs defined in
 * E1.20. It includes errors checking for out-of-range arguments. Each RDM
 * method takes a pointer to a string, which will be populated with an English
 * error message if the command fails.
 */

/**
 * @addtogroup rdm_api
 * @{
 * @file RDMAPI.h
 * @brief Provide a generic RDM API that can use different implementations.
 *
 * This class provides a high level C++ RDM API for PIDS defined in E1.20. It
 * includes error checking for out-of-range arguments. Each RDM method takes a
 * pointer to a string, which will be populated with an english error message
 * if the command fails.
 * @}
 *
 */
#ifndef INCLUDE_OLA_RDM_RDMAPI_H_
#define INCLUDE_OLA_RDM_RDMAPI_H_

#include <stdint.h>
#include <ola/Callback.h>
#include <ola/base/Macro.h>
#include <ola/rdm/RDMAPIImplInterface.h>
#include <ola/rdm/RDMEnums.h>
#include <ola/rdm/UID.h>
#include <map>
#include <string>
#include <vector>

namespace ola {
namespace rdm {

/*
 * Represents a Status Message
 */
typedef struct {
  uint16_t sub_device;
  uint16_t status_message_id;
  int16_t value1;
  int16_t value2;
  uint8_t status_type;
} StatusMessage;


/*
 * Represents the description for a parameter
 */
typedef struct {
  uint16_t pid;
  uint8_t pdl_size;
  uint8_t data_type;
  uint8_t command_class;
  uint8_t unit;
  uint8_t prefix;
  uint32_t min_value;
  uint32_t default_value;
  uint32_t max_value;
  std::string description;
} ParameterDescriptor;


/*
 * Represents a DeviceDescriptor reply
 */
PACK(
struct device_info_s {
  uint8_t protocol_version_high;
  uint8_t protocol_version_low;
  uint16_t device_model;
  uint16_t product_category;
  uint32_t software_version;
  uint16_t dmx_footprint;
  uint8_t current_personality;
  uint8_t personality_count;
  uint16_t dmx_start_address;
  uint16_t sub_device_count;
  uint8_t sensor_count;
});

typedef struct device_info_s DeviceDescriptor;


/*
 * Information about a DMX slot
 */
PACK(
struct slot_info_s {
  uint16_t slot_offset;
  uint8_t slot_type;
  uint16_t slot_label;
});

typedef struct slot_info_s SlotDescriptor;

/*
 * The default values for a slot
 */
PACK(
struct slot_default_s {
  uint16_t slot_offset;
  uint8_t default_value;
});

typedef struct slot_default_s SlotDefault;


/*
 * Sensor definition
 */
typedef struct {
  uint8_t sensor_number;
  uint8_t type;
  uint8_t unit;
  uint8_t prefix;
  int16_t range_min;
  int16_t range_max;
  int16_t normal_min;
  int16_t normal_max;
  uint8_t recorded_value_support;
  std::string description;
} SensorDescriptor;


/*
 * Sensor values
 */
PACK(
struct sensor_values_s {
  uint8_t sensor_number;
  int16_t present_value;
  int16_t lowest;
  int16_t highest;
  int16_t recorded;
});

typedef struct sensor_values_s SensorValueDescriptor;

/*
 * Clock structure
 */
PACK(
struct clock_value_s {
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
});

typedef struct clock_value_s ClockValue;

/*
 * Dimmer info values
 */
typedef struct {
  uint16_t min_level_lower_limit;
  uint16_t min_level_upper_limit;
  uint16_t max_level_lower_limit;
  uint16_t max_level_upper_limit;
  uint8_t curves_supported;
  uint8_t resolution;
  bool split_levels_supported;
} DimmerInfoDescriptor;

/*
 * Dimmer minimum values
 */
typedef struct {
  uint16_t min_level_increasing;
  uint16_t min_level_decreasing;
  bool on_below_min;
} DimmerMinimumDescriptor;

/*
 * The interface for objects which deal with queued messages
 */
class QueuedMessageHandler {
 public:
    virtual ~QueuedMessageHandler() {}

    virtual void ProxiedDeviceCount(const ResponseStatus &status,
                                    uint16_t device_count,
                                    bool list_changed) = 0;
    virtual void ProxiedDevices(const ResponseStatus &status,
                                const std::vector<UID> &uids) = 0;
    virtual void CommStatus(const ResponseStatus &status,
                            uint16_t short_message,
                            uint16_t length_mismatch,
                            uint16_t checksum_fail) = 0;
    virtual void StatusMessages(const ResponseStatus &status,
                                const std::vector<StatusMessage> &messages) = 0;
    virtual void StatusIdDescription(const ResponseStatus &status,
                                     const std::string &status_id) = 0;
    virtual void SubDeviceReporting(const ResponseStatus &status,
                                    uint8_t status_type) = 0;
    virtual void SupportedParameters(
        const ResponseStatus &status,
        const std::vector<uint16_t> &parameters) = 0;
    virtual void ParameterDescription(
        const ResponseStatus &status,
        const ParameterDescriptor &description) = 0;
    virtual void DeviceInfo(const ResponseStatus &status,
                            const DeviceDescriptor &device_info) = 0;
    virtual void ProductDetailIdList(const ResponseStatus &status,
                                     const std::vector<uint16_t> &ids) = 0;
    virtual void DeviceModelDescription(const ResponseStatus &status,
                                        const std::string &description) = 0;
    virtual void ManufacturerLabel(const ResponseStatus &status,
                                   const std::string &label) = 0;
    virtual void DeviceLabel(const ResponseStatus &status,
                             const std::string &label) = 0;
    virtual void FactoryDefaults(const ResponseStatus &status,
                                 bool using_defaults) = 0;
    virtual void LanguageCapabilities(
        const ResponseStatus &status,
        const std::vector<std::string> &langs) = 0;
    virtual void Language(const ResponseStatus &status,
                          const std::string &language) = 0;
    virtual void SoftwareVersionLabel(const ResponseStatus &status,
                                      const std::string &label) = 0;
    virtual void BootSoftwareVersion(const ResponseStatus &status,
                                     uint32_t version) = 0;
    virtual void BootSoftwareVersionLabel(const ResponseStatus &status,
                                          const std::string &label) = 0;
    virtual void DMXPersonality(const ResponseStatus &status,
                                uint8_t current_personality,
                                uint8_t personality_count) = 0;
    virtual void DMXPersonalityDescription(const ResponseStatus &status,
                                           uint8_t personality,
                                           uint16_t slots_requires,
                                           const std::string &label) = 0;
    virtual void DMXAddress(const ResponseStatus &status,
                            uint16_t start_address) = 0;
    virtual void SlotInfo(const ResponseStatus &status,
                          const std::vector<SlotDescriptor> &slots) = 0;
    virtual void SlotDescription(const ResponseStatus &status,
                                 uint16_t slot_offset,
                                 const std::string &description) = 0;
    virtual void SlotDefaultValues(
        const ResponseStatus &status,
        const std::vector<SlotDefault> &defaults) = 0;
    virtual void SensorDefinition(const ResponseStatus &status,
                                  const SensorDescriptor &descriptor) = 0;
    virtual void SensorValue(const ResponseStatus &status,
                             const SensorValueDescriptor &descriptor) = 0;
    virtual void DeviceHours(const ResponseStatus &status,
                             uint32_t hours) = 0;
    virtual void LampHours(const ResponseStatus &status,
                           uint32_t hours) = 0;
    virtual void LampStrikes(const ResponseStatus &status,
                             uint32_t hours) = 0;
    virtual void LampState(const ResponseStatus &status,
                           uint8_t state) = 0;
    virtual void LampMode(const ResponseStatus &status,
                          uint8_t mode) = 0;
    virtual void DevicePowerCycles(const ResponseStatus &status,
                                   uint32_t hours) = 0;
    virtual void DisplayInvert(const ResponseStatus &status,
                               uint8_t invert_mode) = 0;
    virtual void DisplayLevel(const ResponseStatus &status,
                              uint8_t level) = 0;
    virtual void PanInvert(const ResponseStatus &status,
                           uint8_t inverted) = 0;
    virtual void TiltInvert(const ResponseStatus &status,
                            uint8_t inverted) = 0;
    virtual void PanTiltSwap(const ResponseStatus &status,
                             uint8_t inverted) = 0;
    virtual void IdentifyDevice(const ResponseStatus &status,
                                bool mode) = 0;
    virtual void Clock(const ResponseStatus &status,
                       const ClockValue &clock) = 0;
    virtual void PowerState(const ResponseStatus &status,
                            uint8_t power_state) = 0;
    virtual void ResetDevice(const ResponseStatus &status,
                             uint8_t reset_device) = 0;
    virtual void SelfTestEnabled(const ResponseStatus &status,
                                 bool is_enabled) = 0;
    virtual void SelfTestDescription(const ResponseStatus &status,
                                     uint8_t self_test_number,
                                     const std::string &description) = 0;
    virtual void PresetPlaybackMode(const ResponseStatus &status,
                                    uint16_t preset_mode,
                                    uint8_t level) = 0;

    virtual void DefaultHandler(const ResponseStatus &status,
                                uint16_t pid,
                                const std::string &data) = 0;
};


/*
 * The high level RDM API.
 */
class RDMAPI {
 public:
    explicit RDMAPI(class RDMAPIImplInterface *impl):
      m_impl(impl) {
    }
    ~RDMAPI() {}

    // This is used to check for queued messages
    uint8_t OutstandingMessagesCount(const UID &uid);

    // Proxy methods
    bool GetProxiedDeviceCount(
        unsigned int universe,
        const UID &uid,
        ola::SingleUseCallback3<void,
                                const ResponseStatus&,
                                uint16_t,
                                bool> *callback,
        std::string *error);

    bool GetProxiedDevices(
        unsigned int universe,
        const UID &uid,
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                const std::vector<UID>&> *callback,
        std::string *error);

    // Network Management Methods
    bool GetCommStatus(
        unsigned int universe,
        const UID &uid,
        ola::SingleUseCallback4<void,
                                const ResponseStatus&,
                                uint16_t,
                                uint16_t,
                                uint16_t> *callback,
        std::string *error);

    bool ClearCommStatus(
        unsigned int universe,
        const UID &uid,
        ola::SingleUseCallback1<void, const ResponseStatus&> *callback,
        std::string *error);

    // There are two types of queued message calls, one that takes a
    // QueuedMessageHandler and the other than just takes a callback.

    // When complete, the appropriate method will be called on the handler.
    bool GetQueuedMessage(
        unsigned int universe,
        const UID &uid,
        rdm_status_type status_type,
        QueuedMessageHandler *handler,
        std::string *error);

    // When complete, the callback will be run. It's up to the caller to unpack
    // the message.
    bool GetQueuedMessage(
        unsigned int universe,
        const UID &uid,
        rdm_status_type status_type,
        ola::SingleUseCallback3<void,
                                const ResponseStatus&,
                                uint16_t,
                                const std::string&> *callback,
        std::string *error);

    bool GetStatusMessage(
        unsigned int universe,
        const UID &uid,
        rdm_status_type status_type,
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                const std::vector<StatusMessage>&> *callback,
        std::string *error);

    bool GetStatusIdDescription(
        unsigned int universe,
        const UID &uid,
        uint16_t status_id,
        ola::SingleUseCallback2<void, const ResponseStatus&,
                                const std::string&> *callback,
        std::string *error);

    bool ClearStatusId(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        ola::SingleUseCallback1<void, const ResponseStatus&> *callback,
        std::string *error);

    bool GetSubDeviceReporting(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                uint8_t> *callback,
        std::string *error);

    bool SetSubDeviceReporting(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        rdm_status_type status_type,
        ola::SingleUseCallback1<void, const ResponseStatus&> *callback,
        std::string *error);

    // Information Methods
    bool GetSupportedParameters(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                const std::vector<uint16_t> &> *callback,
        std::string *error);

    bool GetParameterDescription(
        unsigned int universe,
        const UID &uid,
        uint16_t pid,
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                const ParameterDescriptor&> *callback,
        std::string *error);

    bool GetDeviceInfo(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                const DeviceDescriptor&> *callback,
        std::string *error);

    bool GetProductDetailIdList(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                const std::vector<uint16_t> &> *callback,
        std::string *error);

    bool GetDeviceModelDescription(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                const std::string&> *callback,
        std::string *error);

    bool GetManufacturerLabel(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                const std::string&> *callback,
        std::string *error);

    bool GetDeviceLabel(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                const std::string&> *callback,
        std::string *error);

    bool SetDeviceLabel(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        const std::string &label,
        ola::SingleUseCallback1<void, const ResponseStatus&> *callback,
        std::string *error);

    bool GetFactoryDefaults(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                bool> *callback,
        std::string *error);

    bool ResetToFactoryDefaults(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        ola::SingleUseCallback1<void, const ResponseStatus&> *callback,
        std::string *error);

    bool GetLanguageCapabilities(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                const std::vector<std::string>&> *callback,
        std::string *error);

    bool GetLanguage(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                const std::string&> *callback,
        std::string *error);

    bool SetLanguage(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        const std::string &language,
        ola::SingleUseCallback1<void, const ResponseStatus&> *callback,
        std::string *error);

    bool GetSoftwareVersionLabel(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                const std::string&> *callback,
        std::string *error);

    bool GetBootSoftwareVersion(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                uint32_t> *callback,
        std::string *error);

    bool GetBootSoftwareVersionLabel(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                const std::string&> *callback,
        std::string *error);

    bool GetDMXPersonality(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        ola::SingleUseCallback3<void,
                                const ResponseStatus&,
                                uint8_t,
                                uint8_t> *callback,
        std::string *error);

    bool SetDMXPersonality(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        uint8_t personality,
        ola::SingleUseCallback1<void, const ResponseStatus&> *callback,
        std::string *error);

    bool GetDMXPersonalityDescription(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        uint8_t personality,
        ola::SingleUseCallback4<void,
                                const ResponseStatus&,
                                uint8_t,
                                uint16_t,
                                const std::string&> *callback,
        std::string *error);

    bool GetDMXAddress(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                uint16_t> *callback,
        std::string *error);

    bool SetDMXAddress(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        uint16_t start_address,
        ola::SingleUseCallback1<void, const ResponseStatus&> *callback,
        std::string *error);

    bool GetSlotInfo(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                const std::vector<SlotDescriptor>&> *callback,
        std::string *error);

    bool GetSlotDescription(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        uint16_t slot_offset,
        ola::SingleUseCallback3<void,
                                const ResponseStatus&,
                                uint16_t,
                                const std::string&> *callback,
        std::string *error);

    bool GetSlotDefaultValues(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                const std::vector<SlotDefault>&> *callback,
        std::string *error);

    bool GetSensorDefinition(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        uint8_t sensor_number,
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                const SensorDescriptor&> *callback,
        std::string *error);

    bool GetSensorValue(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        uint8_t sensor_number,
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                const SensorValueDescriptor&> *callback,
        std::string *error);

    bool SetSensorValue(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        uint8_t sensor_number,
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                const SensorValueDescriptor&> *callback,
        std::string *error);

    bool RecordSensors(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        uint8_t sensor_number,
        ola::SingleUseCallback1<void, const ResponseStatus&> *callback,
        std::string *error);

    bool GetDeviceHours(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                uint32_t> *callback,
        std::string *error);

    bool SetDeviceHours(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        uint32_t device_hours,
        ola::SingleUseCallback1<void, const ResponseStatus&> *callback,
        std::string *error);

    bool GetLampHours(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                uint32_t> *callback,
        std::string *error);

    bool SetLampHours(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        uint32_t lamp_hours,
        ola::SingleUseCallback1<void, const ResponseStatus&> *callback,
        std::string *error);

    bool GetLampStrikes(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                uint32_t> *callback,
        std::string *error);

    bool SetLampStrikes(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        uint32_t lamp_strikes,
        ola::SingleUseCallback1<void, const ResponseStatus&> *callback,
        std::string *error);

    bool GetLampState(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        ola::SingleUseCallback2<void, const ResponseStatus&, uint8_t> *callback,
        std::string *error);

    bool SetLampState(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        uint8_t lamp_state,
        ola::SingleUseCallback1<void, const ResponseStatus&> *callback,
        std::string *error);

    bool GetLampMode(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        ola::SingleUseCallback2<void, const ResponseStatus&, uint8_t> *callback,
        std::string *error);

    bool SetLampMode(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        uint8_t lamp_mode,
        ola::SingleUseCallback1<void, const ResponseStatus&> *callback,
        std::string *error);

    bool GetDevicePowerCycles(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                uint32_t> *callback,
        std::string *error);

    bool SetDevicePowerCycles(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        uint32_t power_cycles,
        ola::SingleUseCallback1<void, const ResponseStatus&> *callback,
        std::string *error);

    bool GetDisplayInvert(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        ola::SingleUseCallback2<void, const ResponseStatus&, uint8_t> *callback,
        std::string *error);

    bool SetDisplayInvert(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        uint8_t display_invert,
        ola::SingleUseCallback1<void, const ResponseStatus&> *callback,
        std::string *error);

    bool GetDisplayLevel(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        ola::SingleUseCallback2<void, const ResponseStatus&, uint8_t> *callback,
        std::string *error);

    bool SetDisplayLevel(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        uint8_t display_level,
        ola::SingleUseCallback1<void, const ResponseStatus&> *callback,
        std::string *error);

    bool GetPanInvert(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        ola::SingleUseCallback2<void, const ResponseStatus&, uint8_t> *callback,
        std::string *error);

    bool SetPanInvert(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        uint8_t invert,
        ola::SingleUseCallback1<void, const ResponseStatus&> *callback,
        std::string *error);

    bool GetTiltInvert(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        ola::SingleUseCallback2<void, const ResponseStatus&, uint8_t> *callback,
        std::string *error);

    bool SetTiltInvert(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        uint8_t invert,
        ola::SingleUseCallback1<void, const ResponseStatus&> *callback,
        std::string *error);

    bool GetPanTiltSwap(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        ola::SingleUseCallback2<void, const ResponseStatus&, uint8_t> *callback,
        std::string *error);

    bool SetPanTiltSwap(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        uint8_t swap,
        ola::SingleUseCallback1<void, const ResponseStatus&> *callback,
        std::string *error);

    bool GetClock(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                const ClockValue&> *callback,
        std::string *error);

    bool SetClock(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        const ClockValue &clock,
        ola::SingleUseCallback1<void, const ResponseStatus&> *callback,
        std::string *error);

    bool GetIdentifyDevice(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        ola::SingleUseCallback2<void, const ResponseStatus&, bool> *callback,
        std::string *error);

    bool IdentifyDevice(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        bool mode,
        ola::SingleUseCallback1<void, const ResponseStatus&> *callback,
        std::string *error);

    bool ResetDevice(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        bool warm_reset,
        ola::SingleUseCallback1<void, const ResponseStatus&> *callback,
        std::string *error);

    bool GetPowerState(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        ola::SingleUseCallback2<void, const ResponseStatus&, uint8_t> *callback,
        std::string *error);

    bool SetPowerState(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        rdm_power_state power_state,
        ola::SingleUseCallback1<void, const ResponseStatus&> *callback,
        std::string *error);

    bool SetResetDevice(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        rdm_reset_device_mode reset_device,
        ola::SingleUseCallback1<void, const ResponseStatus&> *callback,
        std::string *error);

    bool GetDnsHostname(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                const std::string&> *callback,
        std::string *error);

    bool SetDnsHostname(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        const std::string &label,
        ola::SingleUseCallback1<void, const ResponseStatus&> *callback,
        std::string *error);

    bool GetDnsDomainName(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                const std::string&> *callback,
        std::string *error);

    bool SetDnsDomainName(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        const std::string &label,
        ola::SingleUseCallback1<void, const ResponseStatus&> *callback,
        std::string *error);

    bool GetCurve(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        ola::SingleUseCallback3<void,
                                const ResponseStatus&,
                                uint8_t,
                                uint8_t> *callback,
        std::string *error);

    bool SetCurve(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        uint8_t curve,
        ola::SingleUseCallback1<void, const ResponseStatus&> *callback,
        std::string *error);

    bool GetCurveDescription(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        uint8_t curve,
        ola::SingleUseCallback3<void,
                                const ResponseStatus&,
                                uint8_t,
                                const std::string&> *callback,
        std::string *error);

    bool GetDimmerInfo(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                const DimmerInfoDescriptor&> *callback,
        std::string *error);

    bool GetDimmerMinimumLevels(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                const DimmerMinimumDescriptor&> *callback,
        std::string *error);

    bool SetDimmerMinimumLevels(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        uint16_t min_increasing,
        uint16_t min_decreasing,
        bool on_below_min,
        ola::SingleUseCallback1<void, const ResponseStatus&> *callback,
        std::string *error);

    bool GetDimmerMaximumLevel(
        unsigned int universe,
        const UID &uid,
        uint16_t pid,
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                uint16_t> *callback,
        std::string *error);

    bool SetDimmerMaximumLevel(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        uint16_t maximum_level,
        ola::SingleUseCallback1<void, const ResponseStatus&> *callback,
        std::string *error);

    bool SelfTestEnabled(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        ola::SingleUseCallback2<void, const ResponseStatus&, bool> *callback,
        std::string *error);

    bool PerformSelfTest(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        uint8_t self_test_number,
        ola::SingleUseCallback1<void, const ResponseStatus&> *callback,
        std::string *error);

    bool SelfTestDescription(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        uint8_t self_test_number,
        ola::SingleUseCallback3<void,
                                const ResponseStatus&,
                                uint8_t,
                                const std::string&> *callback,
        std::string *error);

    bool CapturePreset(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        uint16_t scene,
        uint16_t fade_up_time,
        uint16_t fade_down_time,
        uint16_t wait_time,
        ola::SingleUseCallback1<void, const ResponseStatus&> *callback,
        std::string *error);

    bool PresetPlaybackMode(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        ola::SingleUseCallback3<void,
                                const ResponseStatus&,
                                uint16_t,
                                uint8_t> *callback,
        std::string *error);

    bool SetPresetPlaybackMode(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        uint16_t playback_mode,
        uint8_t level,
        ola::SingleUseCallback1<void, const ResponseStatus&> *callback,
        std::string *error);

    // Handlers, these are called by the RDMAPIImpl.

    // Generic handlers
    void _HandleCustomLengthLabelResponse(
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                const std::string&> *callback,
        uint8_t length,
        const ResponseStatus &status,
        const std::string &data);

    void _HandleLabelResponse(
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                const std::string&> *callback,
        const ResponseStatus &status,
        const std::string &data);

    void _HandleBoolResponse(
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                bool> *callback,
        const ResponseStatus &status,
        const std::string &data);

    void _HandleU8Response(
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                uint8_t> *callback,
        const ResponseStatus &status,
        const std::string &data);

    void _HandleU16Response(
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                uint16_t> *callback,
        const ResponseStatus &status,
        const std::string &data);

    void _HandleU32Response(
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                uint32_t> *callback,
        const ResponseStatus &status,
        const std::string &data);

    void _HandleEmptyResponse(
        ola::SingleUseCallback1<void, const ResponseStatus&> *callback,
        const ResponseStatus &status,
        const std::string &data);

    // specific handlers follow
    void _HandleGetProxiedDeviceCount(
      ola::SingleUseCallback3<void,
                              const ResponseStatus&,
                              uint16_t,
                              bool> *callback,
      const ResponseStatus &status,
      const std::string &data);

    void _HandleGetProxiedDevices(
      ola::SingleUseCallback2<void,
                              const ResponseStatus&,
                              const std::vector<UID>&> *callback,
      const ResponseStatus &status,
      const std::string &data);

    void _HandleGetCommStatus(
        ola::SingleUseCallback4<void,
                                const ResponseStatus&,
                                uint16_t,
                                uint16_t,
                                uint16_t> *callback,
        const ResponseStatus &status,
        const std::string &data);

    void _HandleQueuedMessage(
        QueuedMessageHandler *handler,
        const ResponseStatus &status,
        uint16_t pid,
        const std::string &data);

    void _HandleGetStatusMessage(
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                const std::vector<StatusMessage>&> *callback,
        const ResponseStatus &status,
        const std::string &data);

    void _HandleGetSubDeviceReporting(
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                uint8_t> *callback,
        const ResponseStatus &status,
        const std::string &data);

    void _HandleGetSupportedParameters(
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                const std::vector<uint16_t>&> *callback,
        const ResponseStatus &status,
        const std::string &data);

    void _HandleGetParameterDescriptor(
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                const ParameterDescriptor&> *callback,
        const ResponseStatus &status,
        const std::string &data);

    void _HandleGetDeviceDescriptor(
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                const DeviceDescriptor&> *callback,
        const ResponseStatus &status,
        const std::string &data);

    void _HandleGetProductDetailIdList(
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                const std::vector<uint16_t>&> *callback,
        const ResponseStatus &status,
        const std::string &data);

    void _HandleGetLanguageCapabilities(
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                const std::vector<std::string>&> *callback,
        const ResponseStatus &status,
        const std::string &data);

    void _HandleGetLanguage(
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                const std::string&> *callback,
        const ResponseStatus &status,
        const std::string &data);

    void _HandleGetBootSoftwareVersion(
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                uint32_t> *callback,
        const ResponseStatus &status,
        const std::string &data);

    void _HandleGetDMXPersonality(
        ola::SingleUseCallback3<void,
                                const ResponseStatus&,
                                uint8_t,
                                uint8_t> *callback,
        const ResponseStatus &status,
        const std::string &data);

    void _HandleGetDMXPersonalityDescription(
        ola::SingleUseCallback4<void,
                                const ResponseStatus&,
                                uint8_t,
                                uint16_t,
                                const std::string&> *callback,
        const ResponseStatus &status,
        const std::string &data);

    void _HandleGetSlotInfo(
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                const std::vector<SlotDescriptor>&> *callback,
        const ResponseStatus &status,
        const std::string &data);

    void _HandleGetSlotDescription(
        ola::SingleUseCallback3<void,
                                const ResponseStatus&,
                                uint16_t,
                                const std::string&> *callback,
        const ResponseStatus &status,
        const std::string &data);

    void _HandleGetSlotDefaultValues(
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                const std::vector<SlotDefault>&> *callback,
        const ResponseStatus &status,
        const std::string &data);

    void _HandleGetSensorDefinition(
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                const SensorDescriptor&> *callback,
        const ResponseStatus &status,
        const std::string &data);

    void _HandleSensorValue(
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                const SensorValueDescriptor&> *callback,
        const ResponseStatus &status,
        const std::string &data);

    void _HandleClock(
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                const ClockValue&> *callback,
        const ResponseStatus &status,
        const std::string &data);

    void _HandleSelfTestDescription(
        ola::SingleUseCallback3<void,
                                const ResponseStatus&,
                                uint8_t,
                                const std::string&> *callback,
        const ResponseStatus &status,
        const std::string &data);

    void _HandlePlaybackMode(
        ola::SingleUseCallback3<void,
                                const ResponseStatus&,
                                uint16_t,
                                uint8_t> *callback,
        const ResponseStatus &status,
        const std::string &data);

    void _HandleGetCurve(
        ola::SingleUseCallback3<void,
                                const ResponseStatus&,
                                uint8_t,
                                uint8_t> *callback,
        const ResponseStatus &status,
        const std::string &data);

    void _HandleGetCurveDescription(
        ola::SingleUseCallback3<void,
                                const ResponseStatus&,
                                uint8_t,
                                const std::string&> *callback,
        const ResponseStatus &status,
        const std::string &data);

    void _HandleGetDimmerInfo(
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                const DimmerInfoDescriptor&> *callback,
        const ResponseStatus &status,
        const std::string &data);

    void _HandleGetDimmerMinimumLevels(
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                const DimmerMinimumDescriptor&> *callback,
        const ResponseStatus &status,
        const std::string &data);

 private:
    class RDMAPIImplInterface *m_impl;
    std::map<UID, uint8_t> m_outstanding_messages;

    bool GenericGetU8(
        unsigned int universe,
        const UID &uid,
        uint8_t sub_device,
        ola::SingleUseCallback2<void, const ResponseStatus&, uint8_t> *callback,
        uint16_t pid,
        std::string *error);

    bool GenericSetU8(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        uint8_t value,
        ola::SingleUseCallback1<void, const ResponseStatus&> *callback,
        uint16_t pid,
        std::string *error);

    bool GenericGetU16(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                uint16_t> *callback,
        uint16_t pid,
        std::string *error);

    bool GenericSetU16(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        uint16_t value,
        ola::SingleUseCallback1<void, const ResponseStatus&> *callback,
        uint16_t pid,
        std::string *error);

    bool GenericGetU32(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        ola::SingleUseCallback2<void,
                                const ResponseStatus&,
                                uint32_t> *callback,
        uint16_t pid,
        std::string *error);

    bool GenericSetU32(
        unsigned int universe,
        const UID &uid,
        uint16_t sub_device,
        uint32_t value,
        ola::SingleUseCallback1<void, const ResponseStatus&> *callback,
        uint16_t pid,
        std::string *error);

    // Check that a callback is not null
    template <typename callback_type>
    bool CheckCallback(std::string *error, const callback_type *cb) {
      if (cb == NULL) {
        if (error)
          *error = "Callback is null, this is a programming error";
        return true;
      }
      return false;
    }

    // Check that a UID is not a broadcast address
    template <typename callback_type>
    bool CheckNotBroadcast(const UID &uid, std::string *error,
                           const callback_type *cb) {
      if (uid.IsBroadcast()) {
        if (error)
          *error = "Cannot send to broadcast address";
        delete cb;
        return true;
      }
      return false;
    }

    // Check the subdevice value is valid
    template <typename callback_type>
    bool CheckValidSubDevice(uint16_t sub_device,
                             bool broadcast_allowed,
                             std::string *error,
                             const callback_type *cb) {
      if (sub_device <= 0x0200)
        return false;

      if (broadcast_allowed && sub_device == ALL_RDM_SUBDEVICES)
        return false;

      if (error) {
        *error = "Sub device must be <= 0x0200";
        if (broadcast_allowed)
          *error += " or 0xffff";
      }
      delete cb;
      return true;
    }

    bool CheckReturnStatus(bool status, std::string *error);
    void SetIncorrectPDL(ResponseStatus *status,
                         unsigned int actual,
                         unsigned int expected);
};
}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_RDMAPI_H_
