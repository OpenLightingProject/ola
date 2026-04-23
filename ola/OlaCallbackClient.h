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
 * OlaCallbackClient.h
 * The legacy callback client.
 * Copyright (C) 2010 Simon Newton
 */

#ifndef OLA_OLACALLBACKCLIENT_H_
#define OLA_OLACALLBACKCLIENT_H_

#include <ola/Callback.h>
#include <ola/DmxBuffer.h>
#include <ola/OlaDevice.h>
#include <ola/client/Result.h>
#include <ola/base/Macro.h>
#include <ola/network/Socket.h>
#include <ola/plugin_id.h>
#include <ola/rdm/RDMAPIImplInterface.h>
#include <ola/rdm/RDMCommand.h>
#include <ola/rdm/UID.h>
#include <ola/rdm/UIDSet.h>
#include <ola/timecode/TimeCode.h>

#include <memory>
#include <string>
#include <vector>

/**
 * @file
 * @brief
 * @deprecated Include <ola/client/OlaClient.h> instead.
 */

namespace ola {

namespace client {
class OlaClientCore;
}

enum PatchAction {PATCH, UNPATCH};
enum RegisterAction {REGISTER, UNREGISTER};
enum PortDirection {INPUT_PORT, OUTPUT_PORT};

/**
 * @brief The legacy callback client.
 * @deprecated Use ola::client::OlaClient instead.
 */
class OlaCallbackClient: public ola::rdm::RDMAPIImplInterface {
 public:
    struct PluginState {
      std::string name;
      bool enabled;
      bool active;
      std::string preferences_source;
      std::vector<OlaPlugin> conflicting_plugins;
    };

    typedef SingleUseCallback2<void, const PluginState&, const std::string&>
      PluginStateCallback;

    typedef Callback3<void, unsigned int, const DmxBuffer&, const std::string&>
        DMXCallback;

    typedef Callback4<void, unsigned int, uint8_t, const DmxBuffer&,
                      const std::string&> DMXCallbackWithPriority;

    explicit OlaCallbackClient(ola::io::ConnectedDescriptor *descriptor);
    ~OlaCallbackClient();

    bool Setup();
    bool Stop();

    void SetCloseHandler(ola::SingleUseCallback0<void> *callback);

    // plugin methods
    bool FetchPluginList(
        SingleUseCallback2<void,
                           const std::vector<OlaPlugin>&,
                           const std::string&> *callback);

    bool FetchPluginDescription(
        ola_plugin_id plugin_id,
        SingleUseCallback2<void,
                           const std::string&,
                           const std::string&> *callback);

    bool FetchPluginState(ola_plugin_id plugin_id,
                          PluginStateCallback *callback);

    // device methods
    bool FetchDeviceInfo(
        ola_plugin_id filter,
        SingleUseCallback2<void,
                           const std::vector<OlaDevice>&,
                           const std::string&> *callback);

    bool FetchCandidatePorts(
        unsigned int universe_id,
        SingleUseCallback2<void,
                           const std::vector<OlaDevice>&,
                           const std::string&> *callback);

    bool FetchCandidatePorts(
        SingleUseCallback2<void,
                           const std::vector<OlaDevice>&,
                           const std::string&> *callback);

    bool ConfigureDevice(
        unsigned int device_alias,
        const std::string &msg,
        SingleUseCallback2<void, const std::string&,
                           const std::string&> *callback);

    // port methods
    bool SetPortPriorityInherit(
        unsigned int device_alias,
        unsigned int port,
        PortDirection port_direction,
        SingleUseCallback1<void, const std::string&> *callback);
    bool SetPortPriorityOverride(
        unsigned int device_alias,
        unsigned int port,
        PortDirection port_direction,
        uint8_t value,
        SingleUseCallback1<void, const std::string&> *callback);

    // universe methods
    bool FetchUniverseList(
        SingleUseCallback2<void,
                           const std::vector<OlaUniverse>&,
                           const std::string &> *callback);
    bool FetchUniverseInfo(
        unsigned int universe,
        SingleUseCallback2<void, OlaUniverse&, const std::string &> *callback);
    bool SetUniverseName(
        unsigned int uni,
        const std::string &name,
        SingleUseCallback1<void, const std::string&> *callback);
    bool SetUniverseMergeMode(
        unsigned int uni,
        OlaUniverse::merge_mode mode,
        SingleUseCallback1<void, const std::string&> *callback);

    // patching
    bool Patch(
        unsigned int device_alias,
        unsigned int port,
        ola::PortDirection port_direction,
        ola::PatchAction action,
        unsigned int uni,
        SingleUseCallback1<void, const std::string&> *callback);

    // DMX methods
    void SetDmxCallback(
        Callback3<void,
                  unsigned int,
                  const DmxBuffer&, const std::string&> *callback);

    // Similar to above, but this also passes the source priority.
    void SetDmxCallback(
        Callback4<void,
                  unsigned int,
                  uint8_t,
                  const DmxBuffer&, const std::string&> *callback);

    bool RegisterUniverse(
        unsigned int universe,
        ola::RegisterAction register_action,
        SingleUseCallback1<void, const std::string&> *callback);
    bool SendDmx(
        unsigned int universe,
        const DmxBuffer &data,
        SingleUseCallback1<void, const std::string&> *callback);
    bool SendDmx(
        unsigned int universe,
        const DmxBuffer &data,
        Callback1<void, const std::string&> *callback);
    // A version of SendDmx that doesn't wait for confirmation
    bool SendDmx(unsigned int universe, const DmxBuffer &data);

    bool FetchDmx(
        unsigned int universe,
        SingleUseCallback2<void, const DmxBuffer&,
                           const std::string&> *callback);

    // rdm methods
    bool FetchUIDList(
        unsigned int universe,
        SingleUseCallback2<void,
                           const ola::rdm::UIDSet&,
                           const std::string&> *callback);
    bool RunDiscovery(
        unsigned int universe,
        bool full,
        ola::SingleUseCallback2<void,
                                const ola::rdm::UIDSet&,
                                const std::string&> *callback);
    bool SetSourceUID(const ola::rdm::UID &uid,
                      ola::SingleUseCallback1<void,
                                              const std::string&> *callback);

    bool RDMGet(ola::rdm::RDMAPIImplInterface::rdm_callback *callback,
                unsigned int universe,
                const ola::rdm::UID &uid,
                uint16_t sub_device,
                uint16_t pid,
                const uint8_t *data,
                unsigned int data_length);
    bool RDMGet(ola::rdm::RDMAPIImplInterface::rdm_pid_callback *callback,
                unsigned int universe,
                const ola::rdm::UID &uid,
                uint16_t sub_device,
                uint16_t pid,
                const uint8_t *data,
                unsigned int data_length);
    bool RDMSet(ola::rdm::RDMAPIImplInterface::rdm_callback *callback,
                unsigned int universe,
                const ola::rdm::UID &uid,
                uint16_t sub_device,
                uint16_t pid,
                const uint8_t *data,
                unsigned int data_length);
    bool RDMSet(ola::rdm::RDMAPIImplInterface::rdm_pid_callback *callback,
                unsigned int universe,
                const ola::rdm::UID &uid,
                uint16_t sub_device,
                uint16_t pid,
                const uint8_t *data,
                unsigned int data_length);

    // timecode
    bool SendTimeCode(ola::SingleUseCallback1<void,
                                              const std::string&> *callback,
                      const ola::timecode::TimeCode &timecode);

 private:
    std::unique_ptr<client::OlaClientCore> m_core;
    std::unique_ptr<DMXCallback> m_dmx_callback;
    std::unique_ptr<DMXCallbackWithPriority> m_priority_dmx_callback;

    void HandlePluginList(
        SingleUseCallback2<void, const std::vector<OlaPlugin>&,
                           const std::string&> *callback,
        const client::Result &result,
        const std::vector<OlaPlugin> &plugins);

    void HandlePluginDescription(
        SingleUseCallback2<void,
                           const std::string&,
                           const std::string&> *callback,
        const client::Result &result,
        const std::string &description);

    void HandlePluginState(
        PluginStateCallback *callback,
        const client::Result &result,
        const client::PluginState &state);

    void HandleDeviceInfo(
        SingleUseCallback2<void,
                           const std::vector<OlaDevice>&,
                           const std::string&> *callback,
        const client::Result &result,
        const std::vector<OlaDevice> &devices);

    void HandleConfigureDevice(
        SingleUseCallback2<void,
                           const std::string&,
                           const std::string&> *callback,
        const client::Result &result,
        const std::string &reply);

    void HandleUniverseList(
        SingleUseCallback2<void, const std::vector<OlaUniverse>&,
                           const std::string &> *callback,
        const client::Result &result,
        const std::vector<OlaUniverse> &universes);

    void HandleUniverseInfo(
        SingleUseCallback2<void, OlaUniverse&, const std::string&> *callback,
        const client::Result &result,
        const OlaUniverse &universe);

    void HandleFetchDmx(
        SingleUseCallback2<void,
                           const DmxBuffer&,
                           const std::string&> *callback,
        const client::Result &result,
        const client::DMXMetadata &metadata,
        const DmxBuffer &data);

    void HandleDiscovery(
        ola::SingleUseCallback2<void,
                                const ola::rdm::UIDSet&,
                                const std::string&> *callback,
        const client::Result &result,
        const ola::rdm::UIDSet &uids);

    void HandleSetCallback(
        ola::SingleUseCallback1<void, const std::string&> *callback,
        const client::Result &result);

    void HandleRepeatableSetCallback(
        ola::Callback1<void, const std::string&> *callback,
        const client::Result &result);

    void HandleDMX(const client::DMXMetadata &metadata,
                   const DmxBuffer &data);

    void HandleRDMResponse(
        ola::rdm::RDMAPIImplInterface::rdm_callback *callback,
        const client::Result &result,
        const client::RDMMetadata &metadata,
        const ola::rdm::RDMResponse *response);

    void HandleRDMResponseWithPid(
        ola::rdm::RDMAPIImplInterface::rdm_pid_callback *callback,
        const client::Result &result,
        const client::RDMMetadata &metadata,
        const ola::rdm::RDMResponse *response);

    void GetResponseStatusAndData(
        const client::Result &result,
        ola::rdm::rdm_response_code response_code,
        const ola::rdm::RDMResponse *response,
        rdm::ResponseStatus *response_status,
        std::string *data);

    void GetParamFromReply(
        const std::string &message_type,
        const ola::rdm::RDMResponse *response,
        ola::rdm::ResponseStatus *new_status);

    DISALLOW_COPY_AND_ASSIGN(OlaCallbackClient);
};
}  // namespace ola
#endif  // OLA_OLACALLBACKCLIENT_H_
