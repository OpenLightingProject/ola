/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * RDMHTTPModule.h
 * This module acts as the HTTP -> olad gateway for RDM commands.
 * Copyright (C) 2010 Simon Newton
 */

#ifndef OLAD_RDMHTTPMODULE_H_
#define OLAD_RDMHTTPMODULE_H_

#include <map>
#include <queue>
#include <string>
#include <utility>
#include <vector>
#include "ola/base/Macro.h"
#include "ola/client/ClientRDMAPIShim.h"
#include "ola/client/OlaClient.h"
#include "ola/http/HTTPServer.h"
#include "ola/rdm/PidStore.h"
#include "ola/rdm/RDMAPI.h"
#include "ola/rdm/UID.h"
#include "ola/thread/Mutex.h"
#include "ola/web/JsonSections.h"

namespace ola {


/*
 * The module that deals with RDM requests.
 */
class RDMHTTPModule {
 public:
    RDMHTTPModule(ola::http::HTTPServer *http_server,
                  ola::client::OlaClient *client);
    ~RDMHTTPModule();

    void SetPidStore(const ola::rdm::RootPidStore *pid_store);

    int RunRDMDiscovery(const ola::http::HTTPRequest *request,
                        ola::http::HTTPResponse *response);

    int JsonUIDs(const ola::http::HTTPRequest *request,
                 ola::http::HTTPResponse *response);

    // these are used by the RDM Patcher
    int JsonUIDInfo(const ola::http::HTTPRequest *request,
                    ola::http::HTTPResponse *response);
    int JsonUIDIdentifyDevice(const ola::http::HTTPRequest *request,
                              ola::http::HTTPResponse *response);
    int JsonUIDPersonalities(const ola::http::HTTPRequest *request,
                             ola::http::HTTPResponse *response);

    // these are used by the RDM Attributes Panel
    int JsonSupportedPIDs(const ola::http::HTTPRequest *request,
                          ola::http::HTTPResponse *response);
    int JsonSupportedSections(const ola::http::HTTPRequest *request,
                              ola::http::HTTPResponse *response);
    int JsonSectionInfo(const ola::http::HTTPRequest *request,
                        ola::http::HTTPResponse *response);
    int JsonSaveSectionInfo(const ola::http::HTTPRequest *request,
                            ola::http::HTTPResponse *response);

    void PruneUniverseList(const std::vector<client::OlaUniverse> &universes);

 private:
    typedef struct {
      std::string manufacturer;
      std::string device;
      bool active;
    } resolved_uid;

    typedef enum {
      RESOLVE_MANUFACTURER,
      RESOLVE_DEVICE,
    } uid_resolve_action;

    typedef struct {
      std::map<ola::rdm::UID, resolved_uid> resolved_uids;
      std::queue<std::pair<ola::rdm::UID, uid_resolve_action> > pending_uids;
      bool uid_resolution_running;
      bool active;
    } uid_resolution_state;

    ola::http::HTTPServer *m_server;
    ola::client::OlaClient *m_client;
    ola::client::ClientRDMAPIShim m_shim;
    ola::rdm::RDMAPI m_rdm_api;
    std::map<unsigned int, uid_resolution_state*> m_universe_uids;

    ola::thread::Mutex m_pid_store_mu;
    const ola::rdm::RootPidStore *m_pid_store;  // GUARDED_BY(m_pid_store_mu);

    typedef struct {
      std::string id;
      std::string name;
      std::string hint;
    } section_info;

    struct lt_section_info {
      bool operator()(const section_info &left, const section_info &right) {
        return left.name < right.name;
      }
    };

    typedef struct {
      unsigned int universe_id;
      const ola::rdm::UID uid;
      std::string hint;
      std::string device_model;
      std::string software_version;
    } device_info;

    typedef struct {
      unsigned int universe_id;
      const ola::rdm::UID *uid;
      bool include_descriptions;
      bool return_as_section;
      unsigned int active;
      unsigned int next;
      unsigned int total;
      std::vector<std::pair<uint32_t, std::string> > personalities;
    } personality_info;

    typedef struct {
      unsigned int universe_id;
      const ola::rdm::UID *uid;
      bool include_descriptions;
      unsigned int active;
      unsigned int next;
      unsigned int total;
      std::vector<std::string> curve_descriptions;
    } curve_info;

    // UID resolution methods
    void HandleUIDList(ola::http::HTTPResponse *response,
                       unsigned int universe_id,
                       const client::Result &result,
                       const ola::rdm::UIDSet &uids);

    void ResolveNextUID(unsigned int universe_id);

    void UpdateUIDManufacturerLabel(unsigned int universe,
                                    ola::rdm::UID uid,
                                    const ola::rdm::ResponseStatus &status,
                                    const std::string &device_label);

    void UpdateUIDDeviceLabel(unsigned int universe,
                              ola::rdm::UID uid,
                              const ola::rdm::ResponseStatus &status,
                              const std::string &device_label);

    uid_resolution_state *GetUniverseUids(unsigned int universe);
    uid_resolution_state *GetUniverseUidsOrCreate(unsigned int universe);

    // uid info handler
    void UIDInfoHandler(ola::http::HTTPResponse *response,
                        const ola::rdm::ResponseStatus &status,
                        const ola::rdm::DeviceDescriptor &device);

    // uid identify handler
    void UIDIdentifyDeviceHandler(ola::http::HTTPResponse *response,
                                  const ola::rdm::ResponseStatus &status,
                                  bool value);

    // personality handler
    void SendPersonalityResponse(ola::http::HTTPResponse *response,
                                 personality_info *info);


    // supported params / sections
    void SupportedParamsHandler(ola::http::HTTPResponse *response,
                                const ola::rdm::ResponseStatus &status,
                                const std::vector<uint16_t> &pids);
    void SupportedSectionsHandler(ola::http::HTTPResponse *response,
                                  unsigned int universe,
                                  ola::rdm::UID uid,
                                  const ola::rdm::ResponseStatus &status,
                                  const std::vector<uint16_t> &pids);
    void SupportedSectionsDeviceInfoHandler(
        ola::http::HTTPResponse *response,
        const std::vector<uint16_t> pids,
        const ola::rdm::ResponseStatus &status,
        const ola::rdm::DeviceDescriptor &device);

    // section methods
    std::string GetCommStatus(ola::http::HTTPResponse *response,
                              unsigned int universe_id,
                              const ola::rdm::UID &uid);

    void CommStatusHandler(ola::http::HTTPResponse *response,
                           const ola::rdm::ResponseStatus &status,
                           uint16_t short_messages,
                           uint16_t length_mismatch,
                           uint16_t checksum_fail);

    std::string ClearCommsCounters(ola::http::HTTPResponse *response,
                                   unsigned int universe_id,
                                   const ola::rdm::UID &uid);

    std::string GetProxiedDevices(ola::http::HTTPResponse *response,
                             unsigned int universe_id,
                             const ola::rdm::UID &uid);


    void ProxiedDevicesHandler(ola::http::HTTPResponse *response,
                               unsigned int universe_id,
                               const ola::rdm::ResponseStatus &status,
                               const std::vector<ola::rdm::UID> &uids);

    std::string GetDeviceInfo(const ola::http::HTTPRequest *request,
                              ola::http::HTTPResponse *response,
                              unsigned int universe_id,
                              const ola::rdm::UID &uid);

    void GetSoftwareVersionHandler(ola::http::HTTPResponse *response,
                                   device_info dev_info,
                                   const ola::rdm::ResponseStatus &status,
                                   const std::string &software_version);

    void GetDeviceModelHandler(ola::http::HTTPResponse *response,
                               device_info dev_info,
                               const ola::rdm::ResponseStatus &status,
                               const std::string &device_model);

    void GetDeviceInfoHandler(ola::http::HTTPResponse *response,
                              device_info dev_info,
                              const ola::rdm::ResponseStatus &status,
                              const ola::rdm::DeviceDescriptor &device);

    std::string GetProductIds(const ola::http::HTTPRequest *request,
                              ola::http::HTTPResponse *response,
                              unsigned int universe_id,
                              const ola::rdm::UID &uid);

    void GetProductIdsHandler(ola::http::HTTPResponse *response,
                              const ola::rdm::ResponseStatus &status,
                              const std::vector<uint16_t> &ids);

    std::string GetManufacturerLabel(const ola::http::HTTPRequest *request,
                                     ola::http::HTTPResponse *response,
                                     unsigned int universe_id,
                                     const ola::rdm::UID &uid);

    void GetManufacturerLabelHandler(ola::http::HTTPResponse *response,
                                     unsigned int universe_id,
                                     const ola::rdm::UID uid,
                                     const ola::rdm::ResponseStatus &status,
                                     const std::string &label);

    std::string GetDeviceLabel(const ola::http::HTTPRequest *request,
                          ola::http::HTTPResponse *response,
                          unsigned int universe_id,
                          const ola::rdm::UID &uid);

    void GetDeviceLabelHandler(ola::http::HTTPResponse *response,
                               unsigned int universe_id,
                               const ola::rdm::UID uid,
                               const ola::rdm::ResponseStatus &status,
                               const std::string &label);

    std::string SetDeviceLabel(const ola::http::HTTPRequest *request,
                               ola::http::HTTPResponse *response,
                               unsigned int universe_id,
                               const ola::rdm::UID &uid);

    std::string GetFactoryDefaults(ola::http::HTTPResponse *response,
                                   unsigned int universe_id,
                                   const ola::rdm::UID &uid);

    void FactoryDefaultsHandler(ola::http::HTTPResponse *response,
                                const ola::rdm::ResponseStatus &status,
                                bool defaults);

    std::string SetFactoryDefault(ola::http::HTTPResponse *response,
                                  unsigned int universe_id,
                                  const ola::rdm::UID &uid);

    std::string GetLanguage(ola::http::HTTPResponse *response,
                            unsigned int universe_id,
                            const ola::rdm::UID &uid);

    void GetSupportedLanguagesHandler(
        ola::http::HTTPResponse *response,
        unsigned int universe_id,
        const ola::rdm::UID uid,
        const ola::rdm::ResponseStatus &status,
        const std::vector<std::string> &languages);

    void GetLanguageHandler(ola::http::HTTPResponse *response,
                            std::vector<std::string> languages,
                            const ola::rdm::ResponseStatus &status,
                            const std::string &language);

    std::string SetLanguage(const ola::http::HTTPRequest *request,
                            ola::http::HTTPResponse *response,
                            unsigned int universe_id,
                            const ola::rdm::UID &uid);

    std::string GetBootSoftware(ola::http::HTTPResponse *response,
                                unsigned int universe_id,
                                const ola::rdm::UID &uid);

    void GetBootSoftwareLabelHandler(ola::http::HTTPResponse *response,
                                     unsigned int universe_id,
                                     const ola::rdm::UID uid,
                                     const ola::rdm::ResponseStatus &status,
                                     const std::string &label);

    void GetBootSoftwareVersionHandler(
        ola::http::HTTPResponse *response,
        std::string label,
        const ola::rdm::ResponseStatus &status,
        uint32_t version);

    std::string GetPersonalities(const ola::http::HTTPRequest *request,
                                 ola::http::HTTPResponse *response,
                                 unsigned int universe_id,
                                 const ola::rdm::UID &uid,
                                 bool return_as_section,
                                 bool include_description = false);

    void GetPersonalityHandler(
        ola::http::HTTPResponse *response,
        personality_info *info,
        const ola::rdm::ResponseStatus &status,
        uint8_t current,
        uint8_t total);

    void GetNextPersonalityDescription(ola::http::HTTPResponse *response,
                                       personality_info *info);

    void GetPersonalityLabelHandler(
        ola::http::HTTPResponse *response,
        personality_info *info,
        const ola::rdm::ResponseStatus &status,
        uint8_t personality,
        uint16_t slot_count,
        const std::string &label);

    void SendSectionPersonalityResponse(ola::http::HTTPResponse *response,
                                        personality_info *info);

    std::string SetPersonality(const ola::http::HTTPRequest *request,
                               ola::http::HTTPResponse *response,
                               unsigned int universe_id,
                               const ola::rdm::UID &uid);

    std::string GetStartAddress(const ola::http::HTTPRequest *request,
                                ola::http::HTTPResponse *response,
                                unsigned int universe_id,
                                const ola::rdm::UID &uid);

    void GetStartAddressHandler(ola::http::HTTPResponse *response,
                                const ola::rdm::ResponseStatus &status,
                                uint16_t address);

    std::string SetStartAddress(const ola::http::HTTPRequest *request,
                                ola::http::HTTPResponse *response,
                                unsigned int universe_id,
                                const ola::rdm::UID &uid);

    std::string GetSensor(const ola::http::HTTPRequest *request,
                          ola::http::HTTPResponse *response,
                          unsigned int universe_id,
                          const ola::rdm::UID &uid);

    void SensorDefinitionHandler(ola::http::HTTPResponse *response,
                                 unsigned int universe_id,
                                 const ola::rdm::UID uid,
                                 uint8_t sensor_id,
                                 const ola::rdm::ResponseStatus &status,
                                 const ola::rdm::SensorDescriptor &definition);

    void SensorValueHandler(ola::http::HTTPResponse *response,
                            ola::rdm::SensorDescriptor *definition,
                            const ola::rdm::ResponseStatus &status,
                            const ola::rdm::SensorValueDescriptor &value);

    std::string RecordSensor(const ola::http::HTTPRequest *request,
                             ola::http::HTTPResponse *response,
                             unsigned int universe_id,
                             const ola::rdm::UID &uid);

    std::string GetDeviceHours(const ola::http::HTTPRequest *request,
                               ola::http::HTTPResponse *response,
                               unsigned int universe_id,
                               const ola::rdm::UID &uid);

    std::string SetDeviceHours(const ola::http::HTTPRequest *request,
                               ola::http::HTTPResponse *response,
                               unsigned int universe_id,
                               const ola::rdm::UID &uid);

    std::string GetLampHours(const ola::http::HTTPRequest *request,
                             ola::http::HTTPResponse *response,
                             unsigned int universe_id,
                             const ola::rdm::UID &uid);

    std::string SetLampHours(const ola::http::HTTPRequest *request,
                             ola::http::HTTPResponse *response,
                             unsigned int universe_id,
                             const ola::rdm::UID &uid);

    std::string GetLampStrikes(const ola::http::HTTPRequest *request,
                               ola::http::HTTPResponse *response,
                               unsigned int universe_id,
                               const ola::rdm::UID &uid);

    std::string SetLampStrikes(const ola::http::HTTPRequest *request,
                               ola::http::HTTPResponse *response,
                               unsigned int universe_id,
                               const ola::rdm::UID &uid);

    std::string GetLampState(const ola::http::HTTPRequest *request,
                             ola::http::HTTPResponse *response,
                             unsigned int universe_id,
                             const ola::rdm::UID &uid);

    void LampStateHandler(ola::http::HTTPResponse *response,
                          const ola::rdm::ResponseStatus &status,
                          uint8_t state);

    std::string SetLampState(const ola::http::HTTPRequest *request,
                             ola::http::HTTPResponse *response,
                             unsigned int universe_id,
                             const ola::rdm::UID &uid);

    std::string GetLampMode(const ola::http::HTTPRequest *request,
                            ola::http::HTTPResponse *response,
                            unsigned int universe_id,
                            const ola::rdm::UID &uid);

    void LampModeHandler(ola::http::HTTPResponse *response,
                         const ola::rdm::ResponseStatus &status,
                         uint8_t mode);

    std::string SetLampMode(const ola::http::HTTPRequest *request,
                            ola::http::HTTPResponse *response,
                            unsigned int universe_id,
                            const ola::rdm::UID &uid);

    std::string GetPowerCycles(const ola::http::HTTPRequest *request,
                               ola::http::HTTPResponse *response,
                               unsigned int universe_id,
                               const ola::rdm::UID &uid);

    std::string SetPowerCycles(const ola::http::HTTPRequest *request,
                               ola::http::HTTPResponse *response,
                               unsigned int universe_id,
                               const ola::rdm::UID &uid);

    std::string GetDisplayInvert(ola::http::HTTPResponse *response,
                                 unsigned int universe_id,
                                 const ola::rdm::UID &uid);

    void DisplayInvertHandler(ola::http::HTTPResponse *response,
                              const ola::rdm::ResponseStatus &status,
                              uint8_t value);

    std::string SetDisplayInvert(const ola::http::HTTPRequest *request,
                                 ola::http::HTTPResponse *response,
                                 unsigned int universe_id,
                                 const ola::rdm::UID &uid);

    std::string GetDisplayLevel(ola::http::HTTPResponse *response,
                                unsigned int universe_id,
                                const ola::rdm::UID &uid);

    void DisplayLevelHandler(ola::http::HTTPResponse *response,
                             const ola::rdm::ResponseStatus &status,
                             uint8_t value);

    std::string SetDisplayLevel(const ola::http::HTTPRequest *request,
                                ola::http::HTTPResponse *response,
                                unsigned int universe_id,
                                const ola::rdm::UID &uid);

    std::string GetPanInvert(ola::http::HTTPResponse *response,
                             unsigned int universe_id,
                             const ola::rdm::UID &uid);

    std::string SetPanInvert(const ola::http::HTTPRequest *request,
                             ola::http::HTTPResponse *response,
                             unsigned int universe_id,
                             const ola::rdm::UID &uid);

    std::string GetTiltInvert(ola::http::HTTPResponse *response,
                              unsigned int universe_id,
                              const ola::rdm::UID &uid);

    std::string SetTiltInvert(const ola::http::HTTPRequest *request,
                              ola::http::HTTPResponse *response,
                              unsigned int universe_id,
                              const ola::rdm::UID &uid);

    std::string GetPanTiltSwap(ola::http::HTTPResponse *response,
                               unsigned int universe_id,
                               const ola::rdm::UID &uid);

    std::string SetPanTiltSwap(const ola::http::HTTPRequest *request,
                               ola::http::HTTPResponse *response,
                               unsigned int universe_id,
                               const ola::rdm::UID &uid);

    std::string GetClock(ola::http::HTTPResponse *response,
                         unsigned int universe_id,
                         const ola::rdm::UID &uid);

    void ClockHandler(ola::http::HTTPResponse *response,
                      const ola::rdm::ResponseStatus &status,
                      const ola::rdm::ClockValue &clock);

    std::string SyncClock(ola::http::HTTPResponse *response,
                          unsigned int universe_id,
                          const ola::rdm::UID &uid);

    std::string GetIdentifyDevice(ola::http::HTTPResponse *response,
                                  unsigned int universe_id,
                                  const ola::rdm::UID &uid);

    std::string SetIdentifyDevice(const ola::http::HTTPRequest *request,
                                  ola::http::HTTPResponse *response,
                                  unsigned int universe_id,
                                  const ola::rdm::UID &uid);

    std::string GetPowerState(ola::http::HTTPResponse *response,
                              unsigned int universe_id,
                              const ola::rdm::UID &uid);

    void PowerStateHandler(ola::http::HTTPResponse *response,
                           const ola::rdm::ResponseStatus &status,
                           uint8_t value);

    std::string SetPowerState(const ola::http::HTTPRequest *request,
                              ola::http::HTTPResponse *response,
                              unsigned int universe_id,
                              const ola::rdm::UID &uid);

    std::string GetResetDevice(ola::http::HTTPResponse *response);

    std::string SetResetDevice(const ola::http::HTTPRequest *request,
                               ola::http::HTTPResponse *response,
                               unsigned int universe_id,
                               const ola::rdm::UID &uid);

    std::string GetDnsHostname(ola::http::HTTPResponse *response,
                               unsigned int universe_id,
                               const ola::rdm::UID &uid);

    void GetDnsHostnameHandler(ola::http::HTTPResponse *response,
                               const ola::rdm::ResponseStatus &status,
                               const std::string &label);

    std::string SetDnsHostname(const ola::http::HTTPRequest *request,
                               ola::http::HTTPResponse *response,
                               unsigned int universe_id,
                               const ola::rdm::UID &uid);

    std::string GetDnsDomainName(ola::http::HTTPResponse *response,
                                 unsigned int universe_id,
                                 const ola::rdm::UID &uid);

    void GetDnsDomainNameHandler(ola::http::HTTPResponse *response,
                                 const ola::rdm::ResponseStatus &status,
                                 const std::string &label);

    std::string SetDnsDomainName(const ola::http::HTTPRequest *request,
                                 ola::http::HTTPResponse *response,
                                 unsigned int universe_id,
                                 const ola::rdm::UID &uid);

    std::string GetCurve(const ola::http::HTTPRequest *request,
                         ola::http::HTTPResponse *response,
                         unsigned int universe_id,
                         const ola::rdm::UID &uid,
                         bool include_descriptions);

    void GetCurveHandler(ola::http::HTTPResponse *response,
                            curve_info *info,
                            const ola::rdm::ResponseStatus &status,
                            uint8_t current_curve,
                            uint8_t curve_count);

    void GetNextCurveDescription(ola::http::HTTPResponse *response,
                                 curve_info *info);

    void GetCurveDescriptionHandler(ola::http::HTTPResponse *response,
                                    curve_info *info,
                                    const ola::rdm::ResponseStatus &status,
                                    uint8_t curve,
                                    const std::string &resp_description);

    void SendCurveResponse(ola::http::HTTPResponse *response,
                           curve_info *info);

    std::string SetCurve(const ola::http::HTTPRequest *request,
                         ola::http::HTTPResponse *response,
                         unsigned int universe_id,
                         const ola::rdm::UID &uid);

    std::string GetDimmerInfo(ola::http::HTTPResponse *response,
                              unsigned int universe_id,
                              const ola::rdm::UID &uid);

    void GetDimmerInfoHandler(ola::http::HTTPResponse *response,
                              const ola::rdm::ResponseStatus &status,
                              const ola::rdm::DimmerInfoDescriptor &dimmer);

    std::string GetDimmerMinimumLevels(ola::http::HTTPResponse *response,
                                       unsigned int universe_id,
                                       const ola::rdm::UID &uid);

    void GetDimmerMinimumLevelsHandler(
        ola::http::HTTPResponse *response,
        const ola::rdm::ResponseStatus &status,
        const ola::rdm::DimmerMinimumDescriptor &dimmer);

    std::string SetDimmerMinimumLevels(const ola::http::HTTPRequest *request,
                                       ola::http::HTTPResponse *response,
                                       unsigned int universe_id,
                                       const ola::rdm::UID &uid);

    std::string GetDimmerMaximumLevel(ola::http::HTTPResponse *response,
                                      unsigned int universe_id,
                                      const ola::rdm::UID &uid);

    void GetDimmerMaximumLevelHandler(ola::http::HTTPResponse *response,
                                      const ola::rdm::ResponseStatus &status,
                                      uint16_t maximum_level);

    std::string SetDimmerMaximumLevel(const ola::http::HTTPRequest *request,
                                      ola::http::HTTPResponse *response,
                                      unsigned int universe_id,
                                      const ola::rdm::UID &uid);

    // util methods
    bool CheckForInvalidId(const ola::http::HTTPRequest *request,
                           unsigned int *universe_id);

    bool CheckForInvalidUid(const ola::http::HTTPRequest *request,
                            ola::rdm::UID **uid);

    uint16_t SubDeviceOrRoot(const ola::http::HTTPRequest *request);

    void SetHandler(ola::http::HTTPResponse *response,
                    const ola::rdm::ResponseStatus &status);

    void GenericUIntHandler(ola::http::HTTPResponse *response,
                            std::string description,
                            const ola::rdm::ResponseStatus &status,
                            uint32_t value);

    void GenericUInt8BoolHandler(ola::http::HTTPResponse *response,
                                 std::string description,
                                 const ola::rdm::ResponseStatus &status,
                                 uint8_t value);
    void GenericBoolHandler(ola::http::HTTPResponse *response,
                            std::string description,
                            const ola::rdm::ResponseStatus &status,
                            bool value);

    bool CheckForRDMError(ola::http::HTTPResponse *response,
                          const ola::rdm::ResponseStatus &status);
    int RespondWithError(ola::http::HTTPResponse *response,
                         const std::string &error);
    void RespondWithSection(ola::http::HTTPResponse *response,
                            const ola::web::JsonSection &section);

    bool CheckForRDMSuccess(const ola::rdm::ResponseStatus &status);
    bool CheckForRDMSuccessWithError(const ola::rdm::ResponseStatus &status,
                                     std::string *error);

    void HandleBoolResponse(ola::http::HTTPResponse *response,
                            const std::string &error);

    void AddSection(std::vector<section_info> *sections,
                    const std::string &section_id,
                    const std::string &section_name,
                    const std::string &hint = "");

    static const uint32_t INVALID_PERSONALITY = 0xffff;
    static const char BACKEND_DISCONNECTED_ERROR[];

    static const char HINT_KEY[];
    static const char ID_KEY[];
    static const char SECTION_KEY[];
    static const char UID_KEY[];

    static const char ADDRESS_FIELD[];
    static const char DIMMER_MINIMUM_INCREASING_FIELD[];
    static const char DIMMER_MINIMUM_DECREASING_FIELD[];
    static const char DISPLAY_INVERT_FIELD[];
    static const char GENERIC_BOOL_FIELD[];
    static const char GENERIC_STRING_FIELD[];
    static const char GENERIC_UINT_FIELD[];
    static const char IDENTIFY_DEVICE_FIELD[];
    static const char LABEL_FIELD[];
    static const char LANGUAGE_FIELD[];
    static const char RECORD_SENSOR_FIELD[];
    static const char SUB_DEVICE_FIELD[];

    static const char BOOT_SOFTWARE_SECTION[];
    static const char CLOCK_SECTION[];
    static const char COMMS_STATUS_SECTION[];
    static const char CURVE_SECTION[];
    static const char DEVICE_HOURS_SECTION[];
    static const char DEVICE_INFO_SECTION[];
    static const char DEVICE_LABEL_SECTION[];
    static const char DIMMER_INFO_SECTION[];
    static const char DIMMER_MAXIMUM_SECTION[];
    static const char DIMMER_MINIMUM_SECTION[];
    static const char DISPLAY_INVERT_SECTION[];
    static const char DISPLAY_LEVEL_SECTION[];
    static const char DMX_ADDRESS_SECTION[];
    static const char DNS_DOMAIN_NAME_SECTION[];
    static const char DNS_HOSTNAME_SECTION[];
    static const char FACTORY_DEFAULTS_SECTION[];
    static const char IDENTIFY_DEVICE_SECTION[];
    static const char LAMP_HOURS_SECTION[];
    static const char LAMP_MODE_SECTION[];
    static const char LAMP_STATE_SECTION[];
    static const char LAMP_STRIKES_SECTION[];
    static const char LANGUAGE_SECTION[];
    static const char MANUFACTURER_LABEL_SECTION[];
    static const char PAN_INVERT_SECTION[];
    static const char PAN_TILT_SWAP_SECTION[];
    static const char PERSONALITY_SECTION[];
    static const char POWER_CYCLES_SECTION[];
    static const char POWER_STATE_SECTION[];
    static const char PRODUCT_DETAIL_SECTION[];
    static const char PROXIED_DEVICES_SECTION[];
    static const char RESET_DEVICE_SECTION[];
    static const char SENSOR_SECTION[];
    static const char TILT_INVERT_SECTION[];

    static const char BOOT_SOFTWARE_SECTION_NAME[];
    static const char CLOCK_SECTION_NAME[];
    static const char COMMS_STATUS_SECTION_NAME[];
    static const char CURVE_SECTION_NAME[];
    static const char DEVICE_HOURS_SECTION_NAME[];
    static const char DEVICE_INFO_SECTION_NAME[];
    static const char DEVICE_LABEL_SECTION_NAME[];
    static const char DIMMER_INFO_SECTION_NAME[];
    static const char DIMMER_MAXIMUM_SECTION_NAME[];
    static const char DIMMER_MINIMUM_SECTION_NAME[];
    static const char DISPLAY_INVERT_SECTION_NAME[];
    static const char DISPLAY_LEVEL_SECTION_NAME[];
    static const char DMX_ADDRESS_SECTION_NAME[];
    static const char DNS_DOMAIN_NAME_SECTION_NAME[];
    static const char DNS_HOSTNAME_SECTION_NAME[];
    static const char FACTORY_DEFAULTS_SECTION_NAME[];
    static const char IDENTIFY_DEVICE_SECTION_NAME[];
    static const char LAMP_HOURS_SECTION_NAME[];
    static const char LAMP_MODE_SECTION_NAME[];
    static const char LAMP_STATE_SECTION_NAME[];
    static const char LAMP_STRIKES_SECTION_NAME[];
    static const char LANGUAGE_SECTION_NAME[];
    static const char MANUFACTURER_LABEL_SECTION_NAME[];
    static const char PAN_INVERT_SECTION_NAME[];
    static const char PAN_TILT_SWAP_SECTION_NAME[];
    static const char PERSONALITY_SECTION_NAME[];
    static const char POWER_CYCLES_SECTION_NAME[];
    static const char POWER_STATE_SECTION_NAME[];
    static const char PRODUCT_DETAIL_SECTION_NAME[];
    static const char PROXIED_DEVICES_SECTION_NAME[];
    static const char RESET_DEVICE_SECTION_NAME[];
    static const char TILT_INVERT_SECTION_NAME[];

    DISALLOW_COPY_AND_ASSIGN(RDMHTTPModule);
};
}  // namespace ola
#endif  // OLAD_RDMHTTPMODULE_H_
