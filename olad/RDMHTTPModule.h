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
 * RDMHTTPModule.h
 * This module acts as the http -> olad gateway for RDM commands.
 * Copyright (C) 2010 Simon Newton
 */

#ifndef OLAD_RDMHTTPMODULE_H_
#define OLAD_RDMHTTPMODULE_H_

#include <map>
#include <queue>
#include <string>
#include <utility>
#include <vector>
#include "ola/OlaCallbackClient.h"
#include "ola/rdm/RDMAPI.h"
#include "ola/rdm/UID.h"
#include "ola/web/JsonSections.h"
#include "ola/http/HTTPServer.h"

namespace ola {

using ola::http::HTTPRequest;
using ola::http::HTTPServer;
using ola::http::HTTPResponse;
using ola::rdm::UID;
using std::string;


/*
 * The module that deals with RDM requests.
 */
class RDMHTTPModule {
  public:
    RDMHTTPModule(HTTPServer *http_server,
                  class OlaCallbackClient *client);
    ~RDMHTTPModule();

    int RunRDMDiscovery(const HTTPRequest *request, HTTPResponse *response);

    int JsonUIDs(const HTTPRequest *request, HTTPResponse *response);

    // these are used by the RDM Patcher
    int JsonUIDInfo(const HTTPRequest *request, HTTPResponse *response);
    int JsonUIDIdentifyMode(const HTTPRequest *request,
                            HTTPResponse *response);
    int JsonUIDPersonalities(const HTTPRequest *request,
                             HTTPResponse *response);

    // these are used by the RDM Attributes Panel
    int JsonSupportedPIDs(const HTTPRequest *request, HTTPResponse *response);
    int JsonSupportedSections(const HTTPRequest *request,
                              HTTPResponse *response);
    int JsonSectionInfo(const HTTPRequest *request,
                        HTTPResponse *response);
    int JsonSaveSectionInfo(const HTTPRequest *request,
                            HTTPResponse *response);

    void PruneUniverseList(const vector<OlaUniverse> &universes);

    string SubDevice(const HTTPRequest *request);

  private:
    typedef struct {
      string manufacturer;
      string device;
      bool active;
    } resolved_uid;

    typedef enum {
      RESOLVE_MANUFACTURER,
      RESOLVE_DEVICE,
    } uid_resolve_action;

    typedef struct {
      map<UID, resolved_uid> resolved_uids;
      std::queue<std::pair<UID, uid_resolve_action> > pending_uids;
      bool uid_resolution_running;
      bool active;
    } uid_resolution_state;

    HTTPServer *m_server;
    class OlaCallbackClient *m_client;
    ola::rdm::RDMAPI m_rdm_api;
    map<unsigned int, uid_resolution_state*> m_universe_uids;

    typedef struct {
      string id;
      string name;
      string hint;
    } section_info;

    struct lt_section_info {
      bool operator()(const section_info &left, const section_info &right) {
        return left.name < right.name;
      }
    };

    typedef struct {
      unsigned int universe_id;
      const UID uid;
      string hint;
      string device_model;
      string software_version;
    } device_info;

    typedef struct {
      unsigned int universe_id;
      const UID *uid;
      bool include_descriptions;
      bool return_as_section;
      unsigned int active;
      unsigned int next;
      unsigned int total;
      vector<std::pair<uint32_t, string> > personalities;
    } personality_info;

    RDMHTTPModule(const RDMHTTPModule&);
    RDMHTTPModule& operator=(const RDMHTTPModule&);

    // uid resolution methods
    void HandleUIDList(HTTPResponse *response,
                       unsigned int universe_id,
                       const ola::rdm::UIDSet &uids,
                       const string &error);

    void ResolveNextUID(unsigned int universe_id);

    void UpdateUIDManufacturerLabel(unsigned int universe,
                                    UID uid,
                                    const ola::rdm::ResponseStatus &status,
                                    const string &device_label);

    void UpdateUIDDeviceLabel(unsigned int universe,
                              UID uid,
                              const ola::rdm::ResponseStatus &status,
                              const string &device_label);

    uid_resolution_state *GetUniverseUids(unsigned int universe);
    uid_resolution_state *GetUniverseUidsOrCreate(unsigned int universe);

    // uid info handler
    void UIDInfoHandler(HTTPResponse *response,
                        const ola::rdm::ResponseStatus &status,
                        const ola::rdm::DeviceDescriptor &device);

    // uid identify handler
    void UIDIdentifyHandler(HTTPResponse *response,
                            const ola::rdm::ResponseStatus &status,
                            bool value);

    // personality handler
    void SendPersonalityResponse(HTTPResponse *response,
                                 personality_info *info);


    // supported params / sections
    void SupportedParamsHandler(HTTPResponse *response,
                                const ola::rdm::ResponseStatus &status,
                                const vector<uint16_t> &pids);
    void SupportedSectionsHandler(HTTPResponse *response,
                                  unsigned int universe,
                                  UID uid,
                                  const ola::rdm::ResponseStatus &status,
                                  const vector<uint16_t> &pids);
    void SupportedSectionsDeviceInfoHandler(
        HTTPResponse *response,
        const vector<uint16_t> pids,
        const ola::rdm::ResponseStatus &status,
        const ola::rdm::DeviceDescriptor &device);

    // section methods
    string GetCommStatus(HTTPResponse *response,
                         unsigned int universe_id,
                         const UID &uid);

    void CommStatusHandler(HTTPResponse *response,
                           const ola::rdm::ResponseStatus &status,
                           uint16_t short_messages,
                           uint16_t length_mismatch,
                           uint16_t checksum_fail);

    string ClearCommsCounters(HTTPResponse *response,
                              unsigned int universe_id,
                              const UID &uid);

    string GetProxiedDevices(HTTPResponse *response,
                             unsigned int universe_id,
                             const UID &uid);


    void ProxiedDevicesHandler(HTTPResponse *response,
                               unsigned int universe_id,
                               const ola::rdm::ResponseStatus &status,
                               const vector<UID> &uids);

    string GetDeviceInfo(const HTTPRequest *request,
                         HTTPResponse *response,
                         unsigned int universe_id,
                         const UID &uid);

    void GetSoftwareVersionHandler(HTTPResponse *response,
                                   device_info dev_info,
                                   const ola::rdm::ResponseStatus &status,
                                   const string &software_version);

    void GetDeviceModelHandler(HTTPResponse *response,
                               device_info dev_info,
                               const ola::rdm::ResponseStatus &status,
                               const string &device_model);

    void GetDeviceInfoHandler(HTTPResponse *response,
                              device_info dev_info,
                              const ola::rdm::ResponseStatus &status,
                              const ola::rdm::DeviceDescriptor &device);

    string GetProductIds(const HTTPRequest *request,
                         HTTPResponse *response,
                         unsigned int universe_id,
                         const UID &uid);

    void GetProductIdsHandler(HTTPResponse *response,
                              const ola::rdm::ResponseStatus &status,
                              const vector<uint16_t> &ids);

    string GetManufacturerLabel(const HTTPRequest *request,
                                HTTPResponse *response,
                                unsigned int universe_id,
                                const UID &uid);

    void GetManufacturerLabelHandler(HTTPResponse *response,
                                     unsigned int universe_id,
                                     const UID uid,
                                     const ola::rdm::ResponseStatus &status,
                                     const string &label);

    string GetDeviceLabel(const HTTPRequest *request,
                          HTTPResponse *response,
                          unsigned int universe_id,
                          const UID &uid);

    void GetDeviceLabelHandler(HTTPResponse *response,
                               unsigned int universe_id,
                               const UID uid,
                               const ola::rdm::ResponseStatus &status,
                               const string &label);

    string SetDeviceLabel(const HTTPRequest *request,
                          HTTPResponse *response,
                          unsigned int universe_id,
                          const UID &uid);

    string GetFactoryDefaults(HTTPResponse *response,
                              unsigned int universe_id,
                              const UID &uid);

    void FactoryDefaultsHandler(HTTPResponse *response,
                                const ola::rdm::ResponseStatus &status,
                                bool defaults);

    string SetFactoryDefault(HTTPResponse *response,
                             unsigned int universe_id,
                             const UID &uid);

    string GetLanguage(HTTPResponse *response,
                       unsigned int universe_id,
                       const UID &uid);

    void GetSupportedLanguagesHandler(HTTPResponse *response,
                                      unsigned int universe_id,
                                      const UID uid,
                                      const ola::rdm::ResponseStatus &status,
                                      const vector<string> &languages);

    void GetLanguageHandler(HTTPResponse *response,
                            vector<string> languages,
                            const ola::rdm::ResponseStatus &status,
                            const string &language);

    string SetLanguage(const HTTPRequest *request,
                       HTTPResponse *response,
                       unsigned int universe_id,
                       const UID &uid);

    string GetBootSoftware(HTTPResponse *response,
                           unsigned int universe_id,
                           const UID &uid);

    void GetBootSoftwareLabelHandler(HTTPResponse *response,
                                     unsigned int universe_id,
                                     const UID uid,
                                     const ola::rdm::ResponseStatus &status,
                                     const string &label);

    void GetBootSoftwareVersionHandler(
        HTTPResponse *response,
        string label,
        const ola::rdm::ResponseStatus &status,
        uint32_t version);

    string GetPersonalities(const HTTPRequest *request,
                            HTTPResponse *response,
                            unsigned int universe_id,
                            const UID &uid,
                            bool return_as_section,
                            bool include_description = false);

    void GetPersonalityHandler(
        HTTPResponse *response,
        personality_info *info,
        const ola::rdm::ResponseStatus &status,
        uint8_t current,
        uint8_t total);

    void GetNextPersonalityDescription(HTTPResponse *response,
                                       personality_info *info);

    void GetPersonalityLabelHandler(
        HTTPResponse *response,
        personality_info *info,
        const ola::rdm::ResponseStatus &status,
        uint8_t personality,
        uint16_t slot_count,
        const string &label);

    void SendSectionPersonalityResponse(HTTPResponse *response,
                                        personality_info *info);

    string SetPersonality(const HTTPRequest *request,
                          HTTPResponse *response,
                          unsigned int universe_id,
                          const UID &uid);

    string GetStartAddress(const HTTPRequest *request,
                           HTTPResponse *response,
                           unsigned int universe_id,
                           const UID &uid);

    void GetStartAddressHandler(HTTPResponse *response,
                                const ola::rdm::ResponseStatus &status,
                                uint16_t address);

    string SetStartAddress(const HTTPRequest *request,
                           HTTPResponse *response,
                           unsigned int universe_id,
                           const UID &uid);

    string GetSensor(const HTTPRequest *request,
                     HTTPResponse *response,
                     unsigned int universe_id,
                     const UID &uid);

    void SensorDefinitionHandler(HTTPResponse *response,
                                 unsigned int universe_id,
                                 const UID uid,
                                 uint8_t sensor_id,
                                 const ola::rdm::ResponseStatus &status,
                                 const ola::rdm::SensorDescriptor &definition);

    void SensorValueHandler(HTTPResponse *response,
                            ola::rdm::SensorDescriptor *definition,
                            const ola::rdm::ResponseStatus &status,
                            const ola::rdm::SensorValueDescriptor &value);

    string RecordSensor(const HTTPRequest *request,
                        HTTPResponse *response,
                        unsigned int universe_id,
                        const UID &uid);

    string GetDeviceHours(const HTTPRequest *request,
                          HTTPResponse *response,
                          unsigned int universe_id,
                          const UID &uid);

    string SetDeviceHours(const HTTPRequest *request,
                          HTTPResponse *response,
                          unsigned int universe_id,
                          const UID &uid);

    string GetLampHours(const HTTPRequest *request,
                        HTTPResponse *response,
                        unsigned int universe_id,
                        const UID &uid);

    string SetLampHours(const HTTPRequest *request,
                        HTTPResponse *response,
                        unsigned int universe_id,
                        const UID &uid);

    string GetLampStrikes(const HTTPRequest *request,
                          HTTPResponse *response,
                          unsigned int universe_id,
                          const UID &uid);

    string SetLampStrikes(const HTTPRequest *request,
                          HTTPResponse *response,
                          unsigned int universe_id,
                          const UID &uid);

    string GetLampState(const HTTPRequest *request,
                        HTTPResponse *response,
                        unsigned int universe_id,
                        const UID &uid);

    void LampStateHandler(HTTPResponse *response,
                          const ola::rdm::ResponseStatus &status,
                          uint8_t state);

    string SetLampState(const HTTPRequest *request,
                        HTTPResponse *response,
                        unsigned int universe_id,
                        const UID &uid);

    string GetLampMode(const HTTPRequest *request,
                        HTTPResponse *response,
                        unsigned int universe_id,
                        const UID &uid);

    void LampModeHandler(HTTPResponse *response,
                          const ola::rdm::ResponseStatus &status,
                          uint8_t mode);

    string SetLampMode(const HTTPRequest *request,
                        HTTPResponse *response,
                        unsigned int universe_id,
                        const UID &uid);

    string GetPowerCycles(const HTTPRequest *request,
                          HTTPResponse *response,
                          unsigned int universe_id,
                          const UID &uid);

    string SetPowerCycles(const HTTPRequest *request,
                          HTTPResponse *response,
                          unsigned int universe_id,
                          const UID &uid);

    string GetDisplayInvert(HTTPResponse *response,
                            unsigned int universe_id,
                            const UID &uid);

    void DisplayInvertHandler(HTTPResponse *response,
                              const ola::rdm::ResponseStatus &status,
                              uint8_t value);

    string SetDisplayInvert(const HTTPRequest *request,
                            HTTPResponse *response,
                            unsigned int universe_id,
                            const UID &uid);

    string GetDisplayLevel(HTTPResponse *response,
                           unsigned int universe_id,
                           const UID &uid);

    void DisplayLevelHandler(HTTPResponse *response,
                             const ola::rdm::ResponseStatus &status,
                             uint8_t value);

    string SetDisplayLevel(const HTTPRequest *request,
                           HTTPResponse *response,
                           unsigned int universe_id,
                           const UID &uid);

    string GetPanInvert(HTTPResponse *response,
                        unsigned int universe_id,
                        const UID &uid);

    string SetPanInvert(const HTTPRequest *request,
                        HTTPResponse *response,
                        unsigned int universe_id,
                        const UID &uid);

    string GetTiltInvert(HTTPResponse *response,
                         unsigned int universe_id,
                         const UID &uid);

    string SetTiltInvert(const HTTPRequest *request,
                         HTTPResponse *response,
                         unsigned int universe_id,
                         const UID &uid);

    string GetPanTiltSwap(HTTPResponse *response,
                          unsigned int universe_id,
                          const UID &uid);

    string SetPanTiltSwap(const HTTPRequest *request,
                          HTTPResponse *response,
                          unsigned int universe_id,
                          const UID &uid);

    string GetClock(HTTPResponse *response,
                    unsigned int universe_id,
                    const UID &uid);

    void ClockHandler(HTTPResponse *response,
                      const ola::rdm::ResponseStatus &status,
                      const ola::rdm::ClockValue &clock);

    string SyncClock(HTTPResponse *response,
                     unsigned int universe_id,
                     const UID &uid);

    string GetIdentifyMode(HTTPResponse *response,
                           unsigned int universe_id,
                           const UID &uid);

    string SetIdentifyMode(const HTTPRequest *request,
                           HTTPResponse *response,
                           unsigned int universe_id,
                           const UID &uid);

    string GetPowerState(HTTPResponse *response,
                         unsigned int universe_id,
                         const UID &uid);

    void PowerStateHandler(HTTPResponse *response,
                           const ola::rdm::ResponseStatus &status,
                           uint8_t value);

    string SetPowerState(const HTTPRequest *request,
                         HTTPResponse *response,
                         unsigned int universe_id,
                         const UID &uid);

    // util methods
    bool CheckForInvalidId(const HTTPRequest *request,
                           unsigned int *universe_id);

    bool CheckForInvalidUid(const HTTPRequest *request, UID **uid);

    void SetHandler(HTTPResponse *response,
                    const ola::rdm::ResponseStatus &status);

    void GenericUIntHandler(HTTPResponse *response,
                            string description,
                            const ola::rdm::ResponseStatus &status,
                            uint32_t value);

    void GenericUInt8BoolHandler(HTTPResponse *response,
                                 string description,
                                 const ola::rdm::ResponseStatus &status,
                                 uint8_t value);
    void GenericBoolHandler(HTTPResponse *response,
                            string description,
                            const ola::rdm::ResponseStatus &status,
                            bool value);

    bool CheckForRDMError(HTTPResponse *response,
                          const ola::rdm::ResponseStatus &status);
    int RespondWithError(HTTPResponse *response, const string &error);
    void RespondWithSection(HTTPResponse *response,
                            const ola::web::JsonSection &section);

    bool CheckForRDMSuccess(const ola::rdm::ResponseStatus &status);
    bool CheckForRDMSuccessWithError(const ola::rdm::ResponseStatus &status,
                                     string *error);

    void HandleBoolResponse(HTTPResponse *response, const string &error);

    void AddSection(vector<section_info> *sections,
                    const string &section_id,
                    const string &section_name,
                    const string &hint="");

    static const uint32_t INVALID_PERSONALITY = 0xffff;
    static const char BACKEND_DISCONNECTED_ERROR[];

    static const char HINT_KEY[];
    static const char ID_KEY[];
    static const char SECTION_KEY[];
    static const char UID_KEY[];

    static const char ADDRESS_FIELD[];
    static const char SUB_DEVICE_FIELD[];
    static const char DISPLAY_INVERT_FIELD[];
    static const char GENERIC_BOOL_FIELD[];
    static const char GENERIC_STRING_FIELD[];
    static const char GENERIC_UINT_FIELD[];
    static const char IDENTIFY_FIELD[];
    static const char LABEL_FIELD[];
    static const char LANGUAGE_FIELD[];
    static const char RECORD_SENSOR_FIELD[];

    static const char BOOT_SOFTWARE_SECTION[];
    static const char COMMS_STATUS_SECTION[];
    static const char CLOCK_SECTION[];
    static const char DEVICE_HOURS_SECTION[];
    static const char DEVICE_INFO_SECTION[];
    static const char DEVICE_LABEL_SECTION[];
    static const char DISPLAY_INVERT_SECTION[];
    static const char DISPLAY_LEVEL_SECTION[];
    static const char DMX_ADDRESS_SECTION[];
    static const char FACTORY_DEFAULTS_SECTION[];
    static const char IDENTIFY_SECTION[];
    static const char LAMP_HOURS_SECTION[];
    static const char LAMP_MODE_SECTION[];
    static const char LAMP_STATE_SECTION[];
    static const char LAMP_STRIKES_SECITON[];
    static const char LANGUAGE_SECTION[];
    static const char MANUFACTURER_LABEL_SECTION[];
    static const char PAN_INVERT_SECTION[];
    static const char PAN_TILT_SWAP_SECTION[];
    static const char PERSONALITY_SECTION[];
    static const char POWER_CYCLES_SECTION[];
    static const char POWER_STATE_SECTION[];
    static const char PRODUCT_DETAIL_SECTION[];
    static const char PROXIED_DEVICES_SECTION[];
    static const char SENSOR_SECTION[];
    static const char TILT_INVERT_SECTION[];
};
}  // ola
#endif  // OLAD_RDMHTTPMODULE_H_
