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
 * RDMHttpModule.h
 * This module acts as the http -> olad gateway for RDM commands.
 * Copyright (C) 2010 Simon Newton
 */

#ifndef OLAD_RDMHTTPMODULE_H_
#define OLAD_RDMHTTPMODULE_H_

#include <map>
#include <queue>
#include <string>
#include <vector>
#include "ola/OlaCallbackClient.h"
#include "ola/rdm/RDMAPI.h"
#include "ola/rdm/UID.h"
#include "ola/web/JsonSections.h"
#include "olad/HttpModule.h"
#include "olad/HttpServer.h"

namespace ola {

using std::string;
using ola::rdm::UID;


/*
 * The module that deals with RDM requests.
 */
class RDMHttpModule: public HttpModule {
  public:
    RDMHttpModule(HttpServer *http_server,
                  class OlaCallbackClient *client);
    ~RDMHttpModule();

    int RunRDMDiscovery(const HttpRequest *request, HttpResponse *response);

    int JsonUIDs(const HttpRequest *request, HttpResponse *response);
    int JsonSupportedPIDs(const HttpRequest *request, HttpResponse *response);
    int JsonSupportedSections(const HttpRequest *request,
                              HttpResponse *response);
    int JsonSectionInfo(const HttpRequest *request,
                        HttpResponse *response);
    int JsonSaveSectionInfo(const HttpRequest *request,
                            HttpResponse *response);

    void PruneUniverseList(const vector<OlaUniverse> &universes);

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

    HttpServer *m_server;
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
      unsigned int active;
      unsigned int next;
      unsigned int total;
      vector<std::pair<uint32_t, string> > personalities;
    } personality_info;

    RDMHttpModule(const RDMHttpModule&);
    RDMHttpModule& operator=(const RDMHttpModule&);

    // uid resolution methods
    void HandleUIDList(HttpResponse *response,
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

    // supported params / sections
    void SupportedParamsHandler(HttpResponse *response,
                                const ola::rdm::ResponseStatus &status,
                                const vector<uint16_t> &pids);
    void SupportedSectionsHandler(HttpResponse *response,
                                  unsigned int universe,
                                  UID uid,
                                  const ola::rdm::ResponseStatus &status,
                                  const vector<uint16_t> &pids);
    void SupportedSectionsDeviceInfoHandler(
        HttpResponse *response,
        const vector<uint16_t> pids,
        const ola::rdm::ResponseStatus &status,
        const ola::rdm::DeviceDescriptor &device);

    // section methods
    string GetProxiedDevices(HttpResponse *response,
                             unsigned int universe_id,
                             const UID &uid);


    void ProxiedDevicesHandler(HttpResponse *response,
                               unsigned int universe_id,
                               const ola::rdm::ResponseStatus &status,
                               const vector<UID> &uids);

    string GetDeviceInfo(const HttpRequest *request,
                         HttpResponse *response,
                         unsigned int universe_id,
                         const UID &uid);

    void GetSoftwareVersionHandler(HttpResponse *response,
                                   device_info dev_info,
                                   const ola::rdm::ResponseStatus &status,
                                   const string &software_version);

    void GetDeviceModelHandler(HttpResponse *response,
                               device_info dev_info,
                               const ola::rdm::ResponseStatus &status,
                               const string &device_model);

    void GetDeviceInfoHandler(HttpResponse *response,
                              device_info dev_info,
                              const ola::rdm::ResponseStatus &status,
                              const ola::rdm::DeviceDescriptor &device);

    string GetProductIds(const HttpRequest *request,
                         HttpResponse *response,
                         unsigned int universe_id,
                         const UID &uid);

    void GetProductIdsHandler(HttpResponse *response,
                              const ola::rdm::ResponseStatus &status,
                              const vector<uint16_t> &ids);

    string GetManufacturerLabel(const HttpRequest *request,
                                HttpResponse *response,
                                unsigned int universe_id,
                                const UID &uid);

    void GetManufacturerLabelHandler(HttpResponse *response,
                                     unsigned int universe_id,
                                     const UID uid,
                                     const ola::rdm::ResponseStatus &status,
                                     const string &label);

    string GetDeviceLabel(const HttpRequest *request,
                          HttpResponse *response,
                          unsigned int universe_id,
                          const UID &uid);

    void GetDeviceLabelHandler(HttpResponse *response,
                               unsigned int universe_id,
                               const UID uid,
                               const ola::rdm::ResponseStatus &status,
                               const string &label);

    string SetDeviceLabel(const HttpRequest *request,
                          HttpResponse *response,
                          unsigned int universe_id,
                          const UID &uid);

    string GetFactoryDefaults(HttpResponse *response,
                              unsigned int universe_id,
                              const UID &uid);

    void FactoryDefaultsHandler(HttpResponse *response,
                                const ola::rdm::ResponseStatus &status,
                                bool defaults);

    string SetFactoryDefault(HttpResponse *response,
                             unsigned int universe_id,
                             const UID &uid);

    string GetLanguage(HttpResponse *response,
                       unsigned int universe_id,
                       const UID &uid);

    void GetSupportedLanguagesHandler(HttpResponse *response,
                                      unsigned int universe_id,
                                      const UID uid,
                                      const ola::rdm::ResponseStatus &status,
                                      const vector<string> &languages);

    void GetLanguageHandler(HttpResponse *response,
                            vector<string> languages,
                            const ola::rdm::ResponseStatus &status,
                            const string &language);

    string SetLanguage(const HttpRequest *request,
                       HttpResponse *response,
                       unsigned int universe_id,
                       const UID &uid);

    string GetBootSoftware(HttpResponse *response,
                           unsigned int universe_id,
                           const UID &uid);

    void GetBootSoftwareLabelHandler(HttpResponse *response,
                                     unsigned int universe_id,
                                     const UID uid,
                                     const ola::rdm::ResponseStatus &status,
                                     const string &label);

    void GetBootSoftwareVersionHandler(
        HttpResponse *response,
        string label,
        const ola::rdm::ResponseStatus &status,
        uint32_t version);

    string GetPersonalities(const HttpRequest *request,
                            HttpResponse *response,
                            unsigned int universe_id,
                            const UID &uid);

    void GetPersonalityHandler(
        HttpResponse *response,
        personality_info *info,
        const ola::rdm::ResponseStatus &status,
        uint8_t current,
        uint8_t total);

    void GetNextPersonalityDescription(HttpResponse *response,
                                       personality_info *info);

    void GetPersonalityLabelHandler(
        HttpResponse *response,
        personality_info *info,
        const ola::rdm::ResponseStatus &status,
        uint8_t personality,
        uint16_t slot_count,
        const string &label);

    void SendPersonalityResponse(HttpResponse *response,
                                 personality_info *info);

    string SetPersonality(const HttpRequest *request,
                          HttpResponse *response,
                          unsigned int universe_id,
                          const UID &uid);

    string GetStartAddress(const HttpRequest *request,
                           HttpResponse *response,
                           unsigned int universe_id,
                           const UID &uid);

    void GetStartAddressHandler(HttpResponse *response,
                                const ola::rdm::ResponseStatus &status,
                                uint16_t address);

    string SetStartAddress(const HttpRequest *request,
                           HttpResponse *response,
                           unsigned int universe_id,
                           const UID &uid);

    string GetSensor(const HttpRequest *request,
                     HttpResponse *response,
                     unsigned int universe_id,
                     const UID &uid);

    void SensorDefinitionHandler(HttpResponse *response,
                                 unsigned int universe_id,
                                 const UID uid,
                                 uint8_t sensor_id,
                                 const ola::rdm::ResponseStatus &status,
                                 const ola::rdm::SensorDescriptor &definition);

    void SensorValueHandler(HttpResponse *response,
                            ola::rdm::SensorDescriptor *definition,
                            const ola::rdm::ResponseStatus &status,
                            const ola::rdm::SensorValueDescriptor &value);

    string RecordSensor(const HttpRequest *request,
                        HttpResponse *response,
                        unsigned int universe_id,
                        const UID &uid);

    string GetDeviceHours(const HttpRequest *request,
                          HttpResponse *response,
                          unsigned int universe_id,
                          const UID &uid);

    string SetDeviceHours(const HttpRequest *request,
                          HttpResponse *response,
                          unsigned int universe_id,
                          const UID &uid);

    string GetLampHours(const HttpRequest *request,
                        HttpResponse *response,
                        unsigned int universe_id,
                        const UID &uid);

    string SetLampHours(const HttpRequest *request,
                        HttpResponse *response,
                        unsigned int universe_id,
                        const UID &uid);

    string GetLampStrikes(const HttpRequest *request,
                          HttpResponse *response,
                          unsigned int universe_id,
                          const UID &uid);

    string SetLampStrikes(const HttpRequest *request,
                          HttpResponse *response,
                          unsigned int universe_id,
                          const UID &uid);

    string GetLampState(const HttpRequest *request,
                        HttpResponse *response,
                        unsigned int universe_id,
                        const UID &uid);

    void LampStateHandler(HttpResponse *response,
                          const ola::rdm::ResponseStatus &status,
                          uint8_t state);

    string SetLampState(const HttpRequest *request,
                        HttpResponse *response,
                        unsigned int universe_id,
                        const UID &uid);

    string GetLampMode(const HttpRequest *request,
                        HttpResponse *response,
                        unsigned int universe_id,
                        const UID &uid);

    void LampModeHandler(HttpResponse *response,
                          const ola::rdm::ResponseStatus &status,
                          uint8_t mode);

    string SetLampMode(const HttpRequest *request,
                        HttpResponse *response,
                        unsigned int universe_id,
                        const UID &uid);

    string GetPowerCycles(const HttpRequest *request,
                          HttpResponse *response,
                          unsigned int universe_id,
                          const UID &uid);

    string SetPowerCycles(const HttpRequest *request,
                          HttpResponse *response,
                          unsigned int universe_id,
                          const UID &uid);

    string GetDisplayInvert(HttpResponse *response,
                            unsigned int universe_id,
                            const UID &uid);

    void DisplayInvertHandler(HttpResponse *response,
                              const ola::rdm::ResponseStatus &status,
                              uint8_t value);

    string SetDisplayInvert(const HttpRequest *request,
                            HttpResponse *response,
                            unsigned int universe_id,
                            const UID &uid);

    string GetDisplayLevel(HttpResponse *response,
                           unsigned int universe_id,
                           const UID &uid);

    void DisplayLevelHandler(HttpResponse *response,
                             const ola::rdm::ResponseStatus &status,
                             uint8_t value);

    string SetDisplayLevel(const HttpRequest *request,
                           HttpResponse *response,
                           unsigned int universe_id,
                           const UID &uid);

    string GetPanInvert(HttpResponse *response,
                        unsigned int universe_id,
                        const UID &uid);

    string SetPanInvert(const HttpRequest *request,
                        HttpResponse *response,
                        unsigned int universe_id,
                        const UID &uid);

    string GetTiltInvert(HttpResponse *response,
                         unsigned int universe_id,
                         const UID &uid);

    string SetTiltInvert(const HttpRequest *request,
                         HttpResponse *response,
                         unsigned int universe_id,
                         const UID &uid);

    string GetPanTiltSwap(HttpResponse *response,
                          unsigned int universe_id,
                          const UID &uid);

    string SetPanTiltSwap(const HttpRequest *request,
                          HttpResponse *response,
                          unsigned int universe_id,
                          const UID &uid);

    string GetClock(HttpResponse *response,
                    unsigned int universe_id,
                    const UID &uid);

    void ClockHandler(HttpResponse *response,
                      const ola::rdm::ResponseStatus &status,
                      const ola::rdm::ClockValue &clock);

    string GetIdentifyMode(HttpResponse *response,
                           unsigned int universe_id,
                           const UID &uid);

    string SetIdentifyMode(const HttpRequest *request,
                           HttpResponse *response,
                           unsigned int universe_id,
                           const UID &uid);

    string GetPowerState(HttpResponse *response,
                         unsigned int universe_id,
                         const UID &uid);

    void PowerStateHandler(HttpResponse *response,
                           const ola::rdm::ResponseStatus &status,
                           uint8_t value);

    string SetPowerState(const HttpRequest *request,
                         HttpResponse *response,
                         unsigned int universe_id,
                         const UID &uid);

    // util methods
    bool CheckForInvalidId(const HttpRequest *request,
                           unsigned int *universe_id);

    bool CheckForInvalidUid(const HttpRequest *request, UID **uid);

    void SetHandler(HttpResponse *response,
                    const ola::rdm::ResponseStatus &status);

    void GenericUIntHandler(HttpResponse *response,
                            string description,
                            const ola::rdm::ResponseStatus &status,
                            uint32_t value);

    void GenericUInt8BoolHandler(HttpResponse *response,
                                 string description,
                                 const ola::rdm::ResponseStatus &status,
                                 uint8_t value);
    void GenericBoolHandler(HttpResponse *response,
                            string description,
                            const ola::rdm::ResponseStatus &status,
                            bool value);

    bool CheckForRDMError(HttpResponse *response,
                          const ola::rdm::ResponseStatus &status);
    int RespondWithError(HttpResponse *response, const string &error);
    void RespondWithSection(HttpResponse *response,
                            const ola::web::JsonSection &section);

    bool CheckForRDMSuccess(const ola::rdm::ResponseStatus &status);
    bool CheckForRDMSuccessWithError(const ola::rdm::ResponseStatus &status,
                                     string *error);

    void HandleBoolResponse(HttpResponse *response, const string &error);

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
    static const char DISPLAY_INVERT_FIELD[];
    static const char GENERIC_BOOL_FIELD[];
    static const char GENERIC_STRING_FIELD[];
    static const char GENERIC_UINT_FIELD[];
    static const char IDENTIFY_FIELD[];
    static const char LABEL_FIELD[];
    static const char LANGUAGE_FIELD[];
    static const char RECORD_SENSOR_FIELD[];

    static const char BOOT_SOFTWARE_SECTION[];
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
