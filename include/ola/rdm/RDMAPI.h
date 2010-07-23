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
 * RDMAPI.h
 * Provide a generic RDM API that can use different implementations.
 * Copyright (C) 2010 Simon Newton
 *
 * This class provides a high level C++ RDM API for PIDs defined in
 * E1.20. It includes errors checking for out of range arguments.
 */

#ifndef INCLUDE_OLA_RDM_RDMAPI_H_
#define INCLUDE_OLA_RDM_RDMAPI_H_

#include <stdint.h>
#include <ola/Callback.h>
#include <ola/rdm/RDMAPIImplInterface.h>
#include <ola/rdm/RDMEnums.h>
#include <ola/rdm/UID.h>
#include <map>
#include <string>
#include <vector>

namespace ola {
namespace rdm {

using std::string;
using std::vector;
using ola::SingleUseCallback1;
using ola::SingleUseCallback2;
using ola::SingleUseCallback3;
using ola::SingleUseCallback4;


/*
 * Represents the state of a response.
 * RDM requests can fail in a number of ways:
 *  - transport error talking to the OLA server
 *  - no response received
 *  - response was NACKED
 *  - the Param data wasn't what we expected
 * This object describes the response state, ResponseType() type should be
 * checked first, and for anything other than NACK_REASON & VALID_RESPONSE,
 * there should be a human readable error in Error(). In the event of a
 * NACK_REASON, NackReason() contains the error code (unless it itself was
 * malformed, in which case MALFORMED_RESPONSE is used).
 */
class ResponseStatus {
  public:
    ResponseStatus(const RDMAPIImplResponseStatus &status,
                   const string &data);

    typedef enum {
      TRANSPORT_ERROR,
      BROADCAST_REQUEST,
      REQUEST_NACKED,
      MALFORMED_RESPONSE,
      VALID_RESPONSE,
    } response_type;

    // This indicates this result of the command
    response_type ResponseType() const { return m_response_type; }
    // The NACK reason if the type was NACK_REASON
    uint16_t NackReason() const { return m_nack_reason; }
    // Provides an error string for the user
    const string& Error() const { return m_error; }

    // Used to change the response type to malformed, with an error string
    void MalformedResponse(const string &error) {
      m_response_type = MALFORMED_RESPONSE;
      m_error = error;
    }

  private:
    response_type m_response_type;
    uint16_t m_nack_reason;
    string m_error;
};


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
  string description;
} ParameterDescriptor;


/*
 * Represents a DeviceDescriptor reply
 */
struct device_info_s {
  uint16_t protocol_version;
  uint16_t device_model;
  uint16_t product_category;
  uint32_t software_version;
  uint16_t dmx_footprint;
  uint16_t dmx_personality;
  uint16_t dmx_start_address;
  uint16_t sub_device_count;
  uint8_t sensor_count;
} __attribute__((packed));

typedef struct device_info_s DeviceDescriptor;


/*
 * Information about a DMX slot
 */
struct slot_info_s {
  uint16_t slot_offset;
  uint8_t slot_type;
  uint16_t slot_label;
} __attribute__((packed));

typedef struct slot_info_s SlotDescriptor;

/*
 * The default values for a slot
 */
struct slot_default_s {
  uint16_t slot_offset;
  uint8_t default_value;
} __attribute__((packed));

typedef struct slot_default_s SlotDefault;


/*
 * Sensor definition
 */
typedef struct {
  uint8_t sensor_number;
  uint8_t type;
  uint8_t unit;
  uint8_t prefix;
  uint16_t range_min;
  uint16_t range_max;
  uint16_t normal_min;
  uint16_t normal_max;
  uint8_t recorded_value_support;
  string description;
} SensorDescriptor;


/*
 * Sensor values
 */
struct sensor_values_s {
  uint8_t sensor_number;
  int16_t present_value;
  int16_t lowest;
  int16_t highest;
  int16_t recorded;
} __attribute__((packed));

typedef struct sensor_values_s SensorValueDescriptor;


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
                                const vector<UID> &uids) = 0;
    virtual void CommStatus(const ResponseStatus &status,
                            uint16_t short_message,
                            uint16_t length_mismatch,
                            uint16_t checksum_fail) = 0;
    virtual void StatusMessages(const ResponseStatus &status,
                                const vector<StatusMessage> messages) = 0;
    virtual void StatusIdDescription(const ResponseStatus &status,
                                     const string &status_id) = 0;
    virtual void SubDeviceReporting(const ResponseStatus &status,
                                    uint8_t status_type) = 0;
    virtual void SupportedParameters(const ResponseStatus &status,
                                     const vector<uint16_t> &parameters) = 0;
    virtual void ParameterDescription(
        const ResponseStatus &status,
        const ParameterDescriptor &description) = 0;
    virtual void DeviceInfo(const ResponseStatus &status,
                            const DeviceDescriptor &device_info) = 0;
    virtual void ProductDetailIdList(const ResponseStatus &status,
                                     const vector<uint16_t> &ids) = 0;
    virtual void DeviceModelDescription(const ResponseStatus &status,
                                        const string &description) = 0;
    virtual void ManufacturerLabel(const ResponseStatus &status,
                                   const string &label) = 0;
    virtual void DeviceLabel(const ResponseStatus &status,
                             const string &label) = 0;
    virtual void FactoryDefaults(const ResponseStatus &status,
                                 bool using_defaults) = 0;
    virtual void LanguageCapabilities(const ResponseStatus &status,
                                      const vector<string> &langs) = 0;
    virtual void Language(const ResponseStatus &status,
                          const string &language) = 0;
    virtual void SoftwareVersionLabel(const ResponseStatus &status,
                                      const string &label) = 0;
    virtual void BootSoftwareVersion(const ResponseStatus &status,
                                     uint32_t version) = 0;
    virtual void BootSoftwareVersionLabel(const ResponseStatus &status,
                                          const string &label) = 0;
    virtual void DMXPersonality(const ResponseStatus &status,
                                uint8_t current_personality,
                                uint8_t personality_count) = 0;
    virtual void DMXPersonalityDescription(const ResponseStatus &status,
                                           uint8_t personality,
                                           uint16_t slots_requires,
                                           const string &label) = 0;
    virtual void DMXAddress(const ResponseStatus &status,
                            uint16_t start_address) = 0;
    virtual void SlotInfo(const ResponseStatus &status,
                          const vector<SlotDescriptor> &slots) = 0;
    virtual void SlotDescription(const ResponseStatus &status,
                                 uint16_t slot_offset,
                                 const string &description) = 0;
    virtual void SlotDefaultValues(const ResponseStatus &status,
                                   const vector<SlotDefault> &defaults) = 0;
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
    virtual void IdentifyMode(const ResponseStatus &status,
                              bool mode) = 0;

    // TODO(simon): add a default handler here
};


/*
 * The high level RDM API.
 */
class RDMAPI {
  public:
    explicit RDMAPI(unsigned int universe,
                    class RDMAPIImplInterface *impl):
      m_universe(universe),
      m_impl(impl) {
    }
    ~RDMAPI() {}

    // This is used to check for queued messages
    uint8_t OutstandingMessagesCount(const UID &uid);

    // Proxy methods
    bool GetProxiedDeviceCount(
        const UID &uid,
        SingleUseCallback3<void,
                           const ResponseStatus&,
                           uint16_t,
                           bool> *callback,
        string *error);

    bool GetProxiedDevices(
        const UID &uid,
        SingleUseCallback2<void,
                           const ResponseStatus&,
                           const vector<UID>&> *callback,
        string *error);

    // Network Managment Methods
    bool GetCommStatus(
        const UID &uid,
        SingleUseCallback4<void,
                           const ResponseStatus&,
                           uint16_t,
                           uint16_t,
                           uint16_t> *callback,
        string *error);

    bool ClearCommStatus(
        const UID &uid,
        SingleUseCallback1<void, const ResponseStatus&> *callback,
        string *error);

    /*
    bool GetQueuedMessage(
      const UID &uid,
      rdm_status_type status_type,
      QueuedMessageHandler *message_handler);
    */

    bool GetStatusMessage(
        const UID &uid,
        rdm_status_type status_type,
        SingleUseCallback2<void,
                           const ResponseStatus&,
                           const vector<StatusMessage>&> *callback,
        string *error);

    bool GetStatusIdDescription(
        const UID &uid,
        uint16_t status_id,
        SingleUseCallback2<void, const ResponseStatus&,
                           const string&> *callback,
        string *error);

    bool ClearStatusId(
        const UID &uid,
        uint16_t sub_device,
        SingleUseCallback1<void, const ResponseStatus&> *callback,
        string *error);

    bool GetSubDeviceReporting(
        const UID &uid,
        uint16_t sub_device,
        SingleUseCallback2<void,
                           const ResponseStatus&,
                           uint8_t> *callback,
        string *error);

    bool SetSubDeviceReporting(
        const UID &uid,
        uint16_t sub_device,
        rdm_status_type status_type,
        SingleUseCallback1<void, const ResponseStatus&> *callback,
        string *error);

    // Information Methods
    bool GetSupportedParameters(
        const UID &uid,
        uint16_t sub_device,
        SingleUseCallback2<void,
                           const ResponseStatus&,
                           const vector<uint16_t> &> *callback,
        string *error);

    bool GetParameterDescription(
        const UID &uid,
        uint16_t pid,
        SingleUseCallback2<void,
                           const ResponseStatus&,
                           const ParameterDescriptor&> *callback,
        string *error);

    bool GetDeviceInfo(
        const UID &uid,
        uint16_t sub_device,
        SingleUseCallback2<void,
                           const ResponseStatus&,
                           const DeviceDescriptor&> *callback,
        string *error);

    bool GetProductDetailIdList(
        const UID &uid,
        uint16_t sub_device,
        SingleUseCallback2<void,
                           const ResponseStatus&,
                           const vector<uint16_t> &> *callback,
        string *error);

    bool GetDeviceModelDescription(
        const UID &uid,
        uint16_t sub_device,
        SingleUseCallback2<void,
                           const ResponseStatus&,
                           const string&> *callback,
        string *error);

    bool GetManufacturerLabel(
        const UID &uid,
        uint16_t sub_device,
        SingleUseCallback2<void,
                           const ResponseStatus&,
                           const string&> *callback,
        string *error);

    bool GetDeviceLabel(
        const UID &uid,
        uint16_t sub_device,
        SingleUseCallback2<void,
                           const ResponseStatus&,
                           const string&> *callback,
        string *error);

    bool SetDeviceLabel(
        const UID &uid,
        uint16_t sub_device,
        const string &label,
        SingleUseCallback1<void, const ResponseStatus&> *callback,
        string *error);

    bool GetFactoryDefaults(
        const UID &uid,
        uint16_t sub_device,
        SingleUseCallback2<void,
                           const ResponseStatus&,
                           bool> *callback,
        string *error);

    bool ResetToFactoryDefaults(
        const UID &uid,
        uint16_t sub_device,
        SingleUseCallback1<void, const ResponseStatus&> *callback,
        string *error);

    bool GetLanguageCapabilities(
        const UID &uid,
        uint16_t sub_device,
        SingleUseCallback2<void,
                           const ResponseStatus&,
                           const vector<string>&> *callback,
        string *error);

    bool GetLanguage(
        const UID &uid,
        uint16_t sub_device,
        SingleUseCallback2<void,
                           const ResponseStatus&,
                           const string&> *callback,
        string *error);

    bool SetLanguage(
        const UID &uid,
        uint16_t sub_device,
        const string &language,
        SingleUseCallback1<void, const ResponseStatus&> *callback,
        string *error);

    bool GetSoftwareVersionLabel(
        const UID &uid,
        uint16_t sub_device,
        SingleUseCallback2<void,
                           const ResponseStatus&,
                           const string&> *callback,
        string *error);

    bool GetBootSoftwareVersion(
        const UID &uid,
        uint16_t sub_device,
        SingleUseCallback2<void,
                           const ResponseStatus&,
                           uint32_t> *callback,
        string *error);

    bool GetBootSoftwareVersionLabel(
        const UID &uid,
        uint16_t sub_device,
        SingleUseCallback2<void,
                           const ResponseStatus&,
                           const string&> *callback,
        string *error);

    bool GetDMXPersonality(
        const UID &uid,
        uint16_t sub_device,
        SingleUseCallback3<void,
                           const ResponseStatus&,
                           uint8_t,
                           uint8_t> *callback,
        string *error);

    bool SetDMXPersonality(
        const UID &uid,
        uint16_t sub_device,
        uint8_t personality,
        SingleUseCallback1<void, const ResponseStatus&> *callback,
        string *error);

    bool GetDMXPersonalityDescription(
        const UID &uid,
        uint16_t sub_device,
        uint8_t personality,
        SingleUseCallback4<void,
                           const ResponseStatus&,
                           uint8_t,
                           uint16_t,
                           const string&> *callback,
        string *error);

    bool GetDMXAddress(
        const UID &uid,
        uint16_t sub_device,
        SingleUseCallback2<void,
                           const ResponseStatus&,
                           uint16_t> *callback,
        string *error);

    bool SetDMXAddress(
        const UID &uid,
        uint16_t sub_device,
        uint16_t start_address,
        SingleUseCallback1<void, const ResponseStatus&> *callback,
        string *error);

    bool GetSlotInfo(
        const UID &uid,
        uint16_t sub_device,
        SingleUseCallback2<void,
                           const ResponseStatus&,
                           const vector<SlotDescriptor>&> *callback,
        string *error);

    bool GetSlotDescription(
        const UID &uid,
        uint16_t sub_device,
        uint16_t slot_offset,
        SingleUseCallback3<void,
                           const ResponseStatus&,
                           uint16_t,
                           const string&> *callback,
        string *error);

    bool GetSlotDefaultValues(
        const UID &uid,
        uint16_t sub_device,
        SingleUseCallback2<void,
                           const ResponseStatus&,
                           const vector<SlotDefault>&> *callback,
        string *error);

    bool GetSensorDefinition(
        const UID &uid,
        uint16_t sub_device,
        uint8_t sensor_number,
        SingleUseCallback2<void,
                           const ResponseStatus&,
                           const SensorDescriptor&> *callback,
        string *error);

    bool GetSensorValue(
        const UID &uid,
        uint16_t sub_device,
        uint8_t sensor_number,
        SingleUseCallback2<void,
                           const ResponseStatus&,
                           const SensorValueDescriptor&> *callback,
        string *error);

    bool SetSensorValue(
        const UID &uid,
        uint16_t sub_device,
        uint8_t sensor_number,
        SingleUseCallback2<void,
                           const ResponseStatus&,
                           const SensorValueDescriptor&> *callback,
        string *error);

    bool RecordSensors(
        const UID &uid,
        uint16_t sub_device,
        uint8_t sensor_number,
        SingleUseCallback1<void, const ResponseStatus&> *callback,
        string *error);

    bool GetDeviceHours(
        const UID &uid,
        uint16_t sub_device,
        SingleUseCallback2<void, const ResponseStatus&, uint32_t> *callback,
        string *error);

    bool SetDeviceHours(
        const UID &uid,
        uint16_t sub_device,
        uint32_t device_hours,
        SingleUseCallback1<void, const ResponseStatus&> *callback,
        string *error);

    bool GetLampHours(
        const UID &uid,
        uint16_t sub_device,
        SingleUseCallback2<void, const ResponseStatus&, uint32_t> *callback,
        string *error);

    bool SetLampHours(
        const UID &uid,
        uint16_t sub_device,
        uint32_t lamp_hours,
        SingleUseCallback1<void, const ResponseStatus&> *callback,
        string *error);

    bool GetLampStrikes(
        const UID &uid,
        uint16_t sub_device,
        SingleUseCallback2<void, const ResponseStatus&, uint32_t> *callback,
        string *error);

    bool SetLampStrikes(
        const UID &uid,
        uint16_t sub_device,
        uint32_t lamp_strikes,
        SingleUseCallback1<void, const ResponseStatus&> *callback,
        string *error);

    bool GetLampState(
        const UID &uid,
        uint16_t sub_device,
        SingleUseCallback2<void, const ResponseStatus&, uint8_t> *callback,
        string *error);

    bool SetLampState(
        const UID &uid,
        uint16_t sub_device,
        uint8_t lamp_state,
        SingleUseCallback1<void, const ResponseStatus&> *callback,
        string *error);

    bool GetLampMode(
        const UID &uid,
        uint16_t sub_device,
        SingleUseCallback2<void, const ResponseStatus&, uint8_t> *callback,
        string *error);

    bool SetLampMode(
        const UID &uid,
        uint16_t sub_device,
        uint8_t lamp_mode,
        SingleUseCallback1<void, const ResponseStatus&> *callback,
        string *error);

    bool GetDevicePowerCycles(
        const UID &uid,
        uint16_t sub_device,
        SingleUseCallback2<void, const ResponseStatus&, uint32_t> *callback,
        string *error);

    bool SetDevicePowerCycles(
        const UID &uid,
        uint16_t sub_device,
        uint32_t power_cycles,
        SingleUseCallback1<void, const ResponseStatus&> *callback,
        string *error);

    bool GetDisplayInvert(
        const UID &uid,
        uint16_t sub_device,
        SingleUseCallback2<void, const ResponseStatus&, uint8_t> *callback,
        string *error);

    bool SetDisplayInvert(
        const UID &uid,
        uint16_t sub_device,
        uint8_t display_invert,
        SingleUseCallback1<void, const ResponseStatus&> *callback,
        string *error);

    bool GetDisplayLevel(
        const UID &uid,
        uint16_t sub_device,
        SingleUseCallback2<void, const ResponseStatus&, uint8_t> *callback,
        string *error);

    bool SetDisplayLevel(
        const UID &uid,
        uint16_t sub_device,
        uint8_t display_level,
        SingleUseCallback1<void, const ResponseStatus&> *callback,
        string *error);

    bool GetPanInvert(
        const UID &uid,
        uint16_t sub_device,
        SingleUseCallback2<void, const ResponseStatus&, uint8_t> *callback,
        string *error);

    bool SetPanInvert(
        const UID &uid,
        uint16_t sub_device,
        uint8_t invert,
        SingleUseCallback1<void, const ResponseStatus&> *callback,
        string *error);

    bool GetTiltInvert(
        const UID &uid,
        uint16_t sub_device,
        SingleUseCallback2<void, const ResponseStatus&, uint8_t> *callback,
        string *error);

    bool SetTiltInvert(
        const UID &uid,
        uint16_t sub_device,
        uint8_t invert,
        SingleUseCallback1<void, const ResponseStatus&> *callback,
        string *error);

    bool GetPanTiltSwap(
        const UID &uid,
        uint16_t sub_device,
        SingleUseCallback2<void, const ResponseStatus&, uint8_t> *callback,
        string *error);

    bool SetPanTiltSwap(
        const UID &uid,
        uint16_t sub_device,
        uint8_t swap,
        SingleUseCallback1<void, const ResponseStatus&> *callback,
        string *error);

    bool GetIdentifyMode(
        const UID &uid,
        uint16_t sub_device,
        SingleUseCallback2<void, const ResponseStatus&, bool> *callback,
        string *error);

    bool IdentifyDevice(
        const UID &uid,
        uint16_t sub_device,
        bool mode,
        SingleUseCallback1<void, const ResponseStatus&> *callback,
        string *error);

    bool ResetDevice(
        const UID &uid,
        uint16_t sub_device,
        bool warm_reset,
        SingleUseCallback1<void, const ResponseStatus&> *callback,
        string *error);

    // Handlers, these are called by the RDMAPIImpl.

    // Generic handlers
    void _HandleLabelResponse(
        SingleUseCallback2<void,
                           const ResponseStatus&,
                           const string&> *callback,
        const RDMAPIImplResponseStatus &status,
        const string &data);

    void _HandleBoolResponse(
        SingleUseCallback2<void,
                           const ResponseStatus&,
                           bool> *callback,
        const RDMAPIImplResponseStatus &status,
        const string &data);

    void _HandleU8Response(
        SingleUseCallback2<void,
                           const ResponseStatus&,
                           uint8_t> *callback,
        const RDMAPIImplResponseStatus &status,
        const string &data);

    void _HandleU32Response(
        SingleUseCallback2<void,
                           const ResponseStatus&,
                           uint32_t> *callback,
        const RDMAPIImplResponseStatus &status,
        const string &data);

    void _HandleEmptyResponse(
        SingleUseCallback1<void, const ResponseStatus&> *callback,
        const RDMAPIImplResponseStatus &status,
        const string &data);

    // specific handlers follow
    void _HandleGetProxiedDeviceCount(
      SingleUseCallback3<void,
                         const ResponseStatus&,
                         uint16_t,
                         bool> *callback,
      const RDMAPIImplResponseStatus &status,
      const string &data);

    void _HandleGetProxiedDevices(
      SingleUseCallback2<void,
                         const ResponseStatus&,
                         const vector<UID>&> *callback,
      const RDMAPIImplResponseStatus &status,
      const string &data);

    void _HandleGetCommStatus(
        SingleUseCallback4<void,
                           const ResponseStatus&,
                           uint16_t,
                           uint16_t,
                           uint16_t> *callback,
        const RDMAPIImplResponseStatus &status,
        const string &data);

    void _HandleGetStatusMessage(
        SingleUseCallback2<void,
                           const ResponseStatus&,
                           const vector<StatusMessage>&> *callback,
        const RDMAPIImplResponseStatus &status,
        const string &data);

    void _HandleGetStatusIdDescription(
        SingleUseCallback2<void, const ResponseStatus&,
                           const string&> *callback,
        const RDMAPIImplResponseStatus &status,
        const string &data);

    void _HandleGetSubDeviceReporting(
        SingleUseCallback2<void,
                           const ResponseStatus&,
                           uint8_t> *callback,
        const RDMAPIImplResponseStatus &status,
        const string &data);

    void _HandleGetSupportedParameters(
        SingleUseCallback2<void,
                           const ResponseStatus&,
                           const vector<uint16_t>&> *callback,
        const RDMAPIImplResponseStatus &status,
        const string &data);

    void _HandleGetParameterDescriptor(
        SingleUseCallback2<void,
                           const ResponseStatus&,
                           const ParameterDescriptor&> *callback,
        const RDMAPIImplResponseStatus &status,
        const string &data);

    void _HandleGetDeviceDescriptor(
        SingleUseCallback2<void,
                           const ResponseStatus&,
                           const DeviceDescriptor&> *callback,
        const RDMAPIImplResponseStatus &status,
        const string &data);

    void _HandleGetProductDetailIdList(
        SingleUseCallback2<void,
                           const ResponseStatus&,
                           const vector<uint16_t>&> *callback,
        const RDMAPIImplResponseStatus &status,
        const string &data);

    void _HandleGetLanguageCapabilities(
        SingleUseCallback2<void,
                           const ResponseStatus&,
                           const vector<string>&> *callback,
        const RDMAPIImplResponseStatus &status,
        const string &data);

    void _HandleGetLanguage(
        SingleUseCallback2<void,
                           const ResponseStatus&,
                           const string&> *callback,
        const RDMAPIImplResponseStatus &status,
        const string &data);

    void _HandleGetBootSoftwareVersion(
        SingleUseCallback2<void,
                           const ResponseStatus&,
                           uint32_t> *callback,
        const RDMAPIImplResponseStatus &status,
        const string &data);

    void _HandleGetDMXPersonality(
        SingleUseCallback3<void,
                           const ResponseStatus&,
                           uint8_t,
                           uint8_t> *callback,
        const RDMAPIImplResponseStatus &status,
        const string &data);

    void _HandleGetDMXPersonalityDescription(
        SingleUseCallback4<void,
                           const ResponseStatus&,
                           uint8_t,
                           uint16_t,
                           const string&> *callback,
        const RDMAPIImplResponseStatus &status,
        const string &data);

    void _HandleGetDMXAddress(
        SingleUseCallback2<void,
                           const ResponseStatus&,
                           uint16_t> *callback,
        const RDMAPIImplResponseStatus &status,
        const string &data);

    void _HandleGetSlotInfo(
        SingleUseCallback2<void,
                           const ResponseStatus&,
                           const vector<SlotDescriptor>&> *callback,
        const RDMAPIImplResponseStatus &status,
        const string &data);

    void _HandleGetSlotDescription(
        SingleUseCallback3<void,
                           const ResponseStatus&,
                           uint16_t,
                           const string&> *callback,
        const RDMAPIImplResponseStatus &status,
        const string &data);

    void _HandleGetSlotDefaultValues(
        SingleUseCallback2<void,
                           const ResponseStatus&,
                           const vector<SlotDefault>&> *callback,
        const RDMAPIImplResponseStatus &status,
        const string &data);

    void _HandleGetSensorDefinition(
        SingleUseCallback2<void,
                           const ResponseStatus&,
                           const SensorDescriptor&> *callback,
        const RDMAPIImplResponseStatus &status,
        const string &data);

    void _HandleSensorValue(
        SingleUseCallback2<void,
                           const ResponseStatus&,
                           const SensorValueDescriptor&> *callback,
        const RDMAPIImplResponseStatus &status,
        const string &data);

  private:
    unsigned int m_universe;
    class RDMAPIImplInterface *m_impl;
    std::map<UID, uint8_t> m_outstanding_messages;

    enum {LABEL_SIZE = 32};

    bool GenericGetU8(
        const UID &uid,
        uint8_t sub_device,
        SingleUseCallback2<void, const ResponseStatus&, uint8_t> *callback,
        uint16_t pid,
        string *error);

    bool GenericSetU8(
        const UID &uid,
        uint16_t sub_device,
        uint8_t value,
        SingleUseCallback1<void, const ResponseStatus&> *callback,
        uint16_t pid,
        string *error);

    bool GenericGetU32(
        const UID &uid,
        uint16_t sub_device,
        SingleUseCallback2<void, const ResponseStatus&, uint32_t> *callback,
        uint16_t pid,
        string *error);

    bool GenericSetU32(
        const UID &uid,
        uint16_t sub_device,
        uint32_t value,
        SingleUseCallback1<void, const ResponseStatus&> *callback,
        uint16_t pid,
        string *error);

    bool CheckNotBroadcast(const UID &uid, string *error);
    bool CheckValidSubDevice(uint16_t sub_device,
                             bool broadcast_allowed,
                             string *error);
    bool CheckReturnStatus(bool status, string *error);
    void SetIncorrectPDL(ResponseStatus *status,
                         unsigned int actual,
                         unsigned int expected);
};
}  // rdm
}  // ola
#endif  // INCLUDE_OLA_RDM_RDMAPI_H_
