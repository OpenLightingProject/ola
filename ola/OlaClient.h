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

#ifndef OLA_CLIENT_H
#define OLA_CLIENT_H

#include <string>
#include <vector>

#include <ola/BaseTypes.h>
#include <ola/common.h>
#include <ola/plugin_id.h>
#include <ola/OlaDevice.h>
#include <ola/DmxBuffer.h>

namespace ola {

namespace network {
class ConnectedSocket;
}

using std::string;
using std::vector;

class OlaClientCore;

enum PatchAction {PATCH, UNPATCH};
enum RegisterAction {REGISTER, UNREGISTER};


class OlaClientObserver {
  public:
    virtual ~OlaClientObserver() {}

    virtual void NewDmx(unsigned int universe,
                        const DmxBuffer &data,
                        const string &error) {}
    virtual void Plugins(const vector <class OlaPlugin> &plugins,
                         const string &error) {}
    virtual void Devices(const vector <class OlaDevice> devices,
                         const string &error) {}
    virtual void Universes(const vector <class OlaUniverse> universes,
                           const string &error) {}
    virtual void DeviceConfig(const string &reply,
                              const string &error) {}

    virtual void PatchComplete(const string &error) {}
    virtual void UniverseNameComplete(const string &error) {}
    virtual void UniverseMergeModeComplete(const string &error) {}
    virtual void SendDmxComplete(const string &error) {}
};


/*
 * OlaClient is just a wrapper around OlaClientCore
 */
class OlaClient {
  public:
    OlaClient(ola::network::ConnectedSocket *socket);
    ~OlaClient();

    bool Setup();
    bool Stop();
    bool SetObserver(OlaClientObserver *observer);

    bool FetchPluginInfo(ola_plugin_id filter=ola::OLA_PLUGIN_ALL,
                         bool include_description=false);
    bool FetchDeviceInfo(ola_plugin_id filter=ola::OLA_PLUGIN_ALL);
    bool FetchUniverseInfo();

    // dmx methods
    bool SendDmx(unsigned int universe, const DmxBuffer &data);
    bool FetchDmx(unsigned int uni);

    // rdm methods
    // int send_rdm(int universe, uint8_t *data, int length);
    bool SetUniverseName(unsigned int uni, const string &name);
    bool SetUniverseMergeMode(unsigned int uni, OlaUniverse::merge_mode mode);

    bool RegisterUniverse(unsigned int universe, ola::RegisterAction action);

    bool Patch(unsigned int device_alias,
              unsigned int port,
              ola::PatchAction action,
              unsigned int uni);

    bool ConfigureDevice(unsigned int device_alias, const string &msg);

  private:
    OlaClient(const OlaClient&);
    OlaClient operator=(const OlaClient&);
    OlaClientCore *m_core;
};

} // ola
#endif
