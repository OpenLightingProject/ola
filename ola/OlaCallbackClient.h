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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
#include <ola/api/Result.h>
#include <ola/base/Macro.h>
#include <ola/common.h>
#include <ola/network/Socket.h>
#include <ola/plugin_id.h>
#include <ola/rdm/RDMAPIImplInterface.h>
#include <ola/rdm/UID.h>
#include <ola/rdm/UIDSet.h>
#include <ola/timecode/TimeCode.h>

#include <string>
#include <vector>

/**
 * @file
 * @brief
 * @deprecated Include <ola/api/OlaClient.h> instead.
 */

namespace ola {

namespace api {
class OlaClientCore;
}

using std::string;

enum PatchAction {PATCH, UNPATCH};
enum RegisterAction {REGISTER, UNREGISTER};
enum PortDirection {INPUT_PORT, OUTPUT_PORT};

/**
 * @brief The legacy callback client.
 * @deprecated Use ola::api::OlaClient instead.
 */
class OlaCallbackClient: public ola::rdm::RDMAPIImplInterface {
  public:
    struct PluginState {
      string name;
      bool enabled;
      bool active;
      string preferences_source;
      std::vector<OlaPlugin> conflicting_plugins;
    };

    typedef SingleUseCallback2<void, const PluginState&, const string&>
      PluginStateCallback;

    typedef Callback3<void, unsigned int, const DmxBuffer&, const string&>
        DMXCallback;

    typedef Callback4<void, unsigned int, uint8_t, const DmxBuffer&,
                      const string&> DMXCallbackWithPriority;

    explicit OlaCallbackClient(ola::io::ConnectedDescriptor *descriptor);
    ~OlaCallbackClient();

    bool Setup();
    bool Stop();

    void SetCloseHandler(ola::SingleUseCallback0<void> *callback);

    // plugin methods
    bool FetchPluginList(
        SingleUseCallback2<void,
                           const std::vector<OlaPlugin>&,
                           const string&> *callback);

    bool FetchPluginDescription(
        ola_plugin_id plugin_id,
        SingleUseCallback2<void, const string&, const string&> *callback);

    bool FetchPluginState(ola_plugin_id plugin_id,
                          PluginStateCallback *callback);

    // device methods
    bool FetchDeviceInfo(
        ola_plugin_id filter,
        SingleUseCallback2<void,
                           const std::vector<OlaDevice>&,
                           const string&> *callback);

    bool FetchCandidatePorts(
        unsigned int universe_id,
        SingleUseCallback2<void,
                           const std::vector<OlaDevice>&,
                           const string&> *callback);

    bool FetchCandidatePorts(
        SingleUseCallback2<void,
                           const std::vector<OlaDevice>&,
                           const string&> *callback);

    bool ConfigureDevice(
        unsigned int device_alias,
        const string &msg,
        SingleUseCallback2<void, const string&, const string&> *callback);

    // port methods
    bool SetPortPriorityInherit(
        unsigned int device_alias,
        unsigned int port,
        PortDirection port_direction,
        SingleUseCallback1<void, const string&> *callback);
    bool SetPortPriorityOverride(
        unsigned int device_alias,
        unsigned int port,
        PortDirection port_direction,
        uint8_t value,
        SingleUseCallback1<void, const string&> *callback);

    // universe methods
    bool FetchUniverseList(
        SingleUseCallback2<void,
                           const std::vector<OlaUniverse>&,
                           const string &> *callback);
    bool FetchUniverseInfo(
        unsigned int universe,
        SingleUseCallback2<void, OlaUniverse&, const string &> *callback);
    bool SetUniverseName(
        unsigned int uni,
        const string &name,
        SingleUseCallback1<void, const string&> *callback);
    bool SetUniverseMergeMode(
        unsigned int uni,
        OlaUniverse::merge_mode mode,
        SingleUseCallback1<void, const string&> *callback);

    // patching
    bool Patch(
        unsigned int device_alias,
        unsigned int port,
        ola::PortDirection port_direction,
        ola::PatchAction action,
        unsigned int uni,
        SingleUseCallback1<void, const string&> *callback);

    // DMX methods
    void SetDmxCallback(
        Callback3<void,
                  unsigned int,
                  const DmxBuffer&, const string&> *callback);

    // Similar to above, but this also passes the source priority.
    void SetDmxCallback(
        Callback4<void,
                  unsigned int,
                  uint8_t,
                  const DmxBuffer&, const string&> *callback);

    bool RegisterUniverse(
        unsigned int universe,
        ola::RegisterAction register_action,
        SingleUseCallback1<void, const string&> *callback);
    bool SendDmx(
        unsigned int universe,
        const DmxBuffer &data,
        SingleUseCallback1<void, const string&> *callback);
    bool SendDmx(
        unsigned int universe,
        const DmxBuffer &data,
        Callback1<void, const string&> *callback);
    // A version of SendDmx that doesn't wait for confirmation
    bool SendDmx(unsigned int universe, const DmxBuffer &data);

    bool FetchDmx(
        unsigned int universe,
        SingleUseCallback2<void, const DmxBuffer&, const string&> *callback);

    // rdm methods
    bool FetchUIDList(
        unsigned int universe,
        SingleUseCallback2<void,
                           const ola::rdm::UIDSet&,
                           const string&> *callback);
    bool RunDiscovery(
        unsigned int universe,
        bool full,
        ola::SingleUseCallback2<void,
                                const ola::rdm::UIDSet&,
                                const string&> *callback);
    bool SetSourceUID(const ola::rdm::UID &uid,
                      ola::SingleUseCallback1<void, const string&> *callback);

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
    bool SendTimeCode(ola::SingleUseCallback1<void, const string&> *callback,
                      const ola::timecode::TimeCode &timecode);

  private:
    std::auto_ptr<api::OlaClientCore> m_core;
    std::auto_ptr<DMXCallback> m_dmx_callback;
    std::auto_ptr<DMXCallbackWithPriority> m_priority_dmx_callback;

    void HandlePluginList(
        SingleUseCallback2<void, const std::vector<OlaPlugin>&,
                           const string&> *callback,
        const api::Result &result,
        const std::vector<OlaPlugin> &plugins);

    void HandlePluginDescription(
        SingleUseCallback2<void, const string&, const string&> *callback,
        const api::Result &result,
        const std::string &description);

    void HandlePluginState(
        PluginStateCallback *callback,
        const api::Result &result,
        const api::PluginState &state);

    void HandleDeviceInfo(
        SingleUseCallback2<void, const std::vector<OlaDevice>&, const string&>
            *callback,
        const api::Result &result,
        const std::vector<OlaDevice> &devices);

    void HandleConfigureDevice(
        SingleUseCallback2<void, const string&, const string&> *callback,
        const api::Result &result,
        const std::string &reply);

    void HandleUniverseList(
        SingleUseCallback2<void, const std::vector<OlaUniverse>&,
                           const string &> *callback,
        const api::Result &result,
        const std::vector<OlaUniverse> &universes);

    void HandleUniverseInfo(
        SingleUseCallback2<void, OlaUniverse&, const string&> *callback,
        const api::Result &result,
        const OlaUniverse &universe);

    void HandleFetchDmx(
        SingleUseCallback2<void, const DmxBuffer&, const string&> *callback,
        const api::Result &result,
        const api::DMXMetadata &metadata,
        const DmxBuffer &data);

    void HandleDiscovery(
        ola::SingleUseCallback2<void,
                                const ola::rdm::UIDSet&,
                                const string&> *callback,
        const api::Result &result,
        const ola::rdm::UIDSet &uids);

    void HandleSetCallback(
        ola::SingleUseCallback1<void, const string&> *callback,
        const api::Result &result);

    void HandleRepeatableSetCallback(
        ola::Callback1<void, const string&> *callback,
        const api::Result &result);

    void HandleDMX(const api::DMXMetadata &metadata,
                   const DmxBuffer &data);

    DISALLOW_COPY_AND_ASSIGN(OlaCallbackClient);
};
}  // namespace ola
#endif  // OLA_OLACALLBACKCLIENT_H_
