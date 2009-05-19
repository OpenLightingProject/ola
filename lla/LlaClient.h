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
 * LlaClient.h
 * Interface to the LLA Client class
 * Copyright (C) 2005-2008 Simon Newton
 */

#ifndef LLA_CLIENT_H
#define LLA_CLIENT_H

#include <string>
#include <vector>

#include <lla/BaseTypes.h>
#include <lla/common.h>
#include <lla/plugin_id.h>
#include <lla/LlaDevice.h>

namespace lla {

namespace network {
class ConnectedSocket;
}

using std::string;
using std::vector;

class LlaClientCore;

enum PatchAction {PATCH, UNPATCH};
enum RegisterAction {REGISTER, UNREGISTER};


class LlaClientObserver {
  public:
    virtual ~LlaClientObserver() {}

    virtual void NewDmx(unsigned int universe,
                        unsigned int length,
                        dmx_t *data,
                        const string &error) {}
    virtual void Plugins(const vector <class LlaPlugin> &plugins,
                         const string &error) {}
    virtual void Devices(const vector <class LlaDevice> devices,
                         const string &error) {}
    virtual void Universes(const vector <class LlaUniverse> universes,
                           const string &error) {}
    virtual void DeviceConfig(const string &reply,
                              const string &error) {}

    virtual void PatchComplete(const string &error) {}
    virtual void UniverseNameComplete(const string &error) {}
    virtual void UniverseMergeModeComplete(const string &error) {}
    virtual void SendDmxComplete(const string &error) {}
};


/*
 * LlaClient is just a wrapper around LLaClientCore
 */
class LlaClient {
  public:
    LlaClient(lla::network::ConnectedSocket *socket);
    ~LlaClient();

    bool Setup();
    bool Stop();
    bool SetObserver(LlaClientObserver *observer);

    bool FetchPluginInfo(int plugin_id=-1, bool include_description=false);
    bool FetchDeviceInfo(lla_plugin_id filter=LLA_PLUGIN_ALL);
    bool FetchUniverseInfo();

    // dmx methods
    bool SendDmx(unsigned int universe, dmx_t *data, unsigned int length);
    bool FetchDmx(unsigned int uni);

    // rdm methods
    // int send_rdm(int universe, uint8_t *data, int length);
    bool SetUniverseName(unsigned int uni, const string &name);
    bool SetUniverseMergeMode(unsigned int uni, LlaUniverse::merge_mode mode);

    bool RegisterUniverse(unsigned int universe, lla::RegisterAction action);

    bool Patch(unsigned int device,
              unsigned int port,
              lla::PatchAction action,
              unsigned int uni);

    bool ConfigureDevice(unsigned int dev, const string &msg);

  private:
    LlaClient(const LlaClient&);
    LlaClient operator=(const LlaClient&);
    LlaClientCore *m_core;
};

} // lla
#endif
