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
 * OlaClient.h
 * Interface to the OLA Client class
 * Copyright (C) 2005-2008 Simon Newton
 */

#ifndef OLA_OLACLIENT_H_
#define OLA_OLACLIENT_H_

#include <ola/BaseTypes.h>
#include <ola/DmxBuffer.h>
#include <ola/OlaDevice.h>
#include <ola/common.h>
#include <ola/plugin_id.h>
#include <ola/rdm/RDMAPIImplInterface.h>
#include <ola/rdm/UIDSet.h>
#include <olad/PortConstants.h>
#include <string>
#include <vector>

namespace ola {

namespace network {
class ConnectedSocket;
}

using std::string;
using std::vector;
using ola::rdm::UID;

class OlaClientCore;

enum PatchAction {PATCH, UNPATCH};
enum RegisterAction {REGISTER, UNREGISTER};


class OlaClientObserver {
  public:
    virtual ~OlaClientObserver() {}

    virtual void NewDmx(unsigned int universe,
                        const DmxBuffer &data,
                        const string &error) {
      (void) universe;
      (void) data;
      (void) error;
    }
    virtual void Plugins(const vector <class OlaPlugin> &plugins,
                         const string &error) {
      (void) plugins;
      (void) error;
    }
    virtual void Devices(const vector <class OlaDevice> devices,
                         const string &error) {
      (void) devices;
      (void) error;
    }
    virtual void Universes(const vector <class OlaUniverse> universes,
                           const string &error) {
      (void) universes;
      (void) error;
    }
    virtual void DeviceConfig(const string &reply,
                              const string &error) {
      (void) reply;
      (void) error;
    }

    virtual void PatchComplete(const string &error) {
      (void) error;
    }
    virtual void UniverseNameComplete(const string &error) {
      (void) error;
    }
    virtual void UniverseMergeModeComplete(const string &error) {
      (void) error;
    }
    virtual void SendDmxComplete(const string &error) {
      (void) error;
    }
    virtual void SetPortPriorityComplete(const string &error) {
      (void) error;
    }

    // RDM methods
    virtual void UIDList(unsigned int universe,
                         const ola::rdm::UIDSet &uids,
                         const string &error) {
      (void) universe;
      (void) uids;
      (void) error;
    }
    virtual void ForceRDMDiscoveryComplete(unsigned int universe,
                                           const string &error) {
      (void) universe;
      (void) error;
    }
};


/*
 * OlaClient is just a wrapper around OlaClientCore
 */
class OlaClient: public ola::rdm::RDMAPIImplInterface {
  public:
    explicit OlaClient(ola::network::ConnectedSocket *socket);
    ~OlaClient();

    bool Setup();
    bool Stop();
    bool SetObserver(OlaClientObserver *observer);

    bool FetchPluginInfo(ola_plugin_id filter = ola::OLA_PLUGIN_ALL,
                         bool include_description = false);
    bool FetchDeviceInfo(ola_plugin_id filter = ola::OLA_PLUGIN_ALL);
    bool FetchUniverseInfo();

    // dmx methods
    bool SendDmx(unsigned int universe, const DmxBuffer &data);
    bool FetchDmx(unsigned int uni);

    // rdm methods
    bool FetchUIDList(unsigned int universe);
    bool ForceDiscovery(unsigned int universe);
    bool SetSourceUID(const UID &uid,
                      ola::SingleUseCallback1<void, const string &> *callback);

    bool RDMGet(rdm_callback *callback,
                unsigned int universe,
                const UID &uid,
                uint16_t sub_device,
                uint16_t pid,
                const uint8_t *data = NULL,
                unsigned int data_length = 0);
    bool RDMSet(rdm_callback *callback,
                unsigned int universe,
                const UID &uid,
                uint16_t sub_device,
                uint16_t pid,
                const uint8_t *data,
                unsigned int data_length);

    bool SetUniverseName(unsigned int uni, const string &name);
    bool SetUniverseMergeMode(unsigned int uni, OlaUniverse::merge_mode mode);

    bool RegisterUniverse(unsigned int universe, ola::RegisterAction action);

    bool Patch(unsigned int device_alias,
               unsigned int port,
               bool is_output,
               ola::PatchAction action,
               unsigned int uni);

    bool SetPortPriorityInherit(unsigned int device_alias,
                                unsigned int port,
                                bool is_output);
    bool SetPortPriorityOverride(unsigned int device_alias,
                                 unsigned int port,
                                 bool is_output,
                                 uint8_t value);

    bool ConfigureDevice(unsigned int device_alias, const string &msg);

  private:
    OlaClient(const OlaClient&);
    OlaClient operator=(const OlaClient&);
    OlaClientCore *m_core;
};
}  // ola
#endif  // OLA_OLACLIENT_H_
