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
 *  LlaConfigurator.h
 *  Makes configuring devices easy
 *  Copyright (C) 2005-2009 Simon Newton
 *
 * The Configurator makes it easy to use the device specific ConfigureDevice()
 * rpc. For each device type you want to manage, subclass the Configurator and
 * implement the SendConfigRequest() and HandleConfigResponse() methods.
 * Briefly, upon calling Setup() the Configurator will send a DeviceInfo
 * request to check that the device type matches the plugin_id given in the
 * constructor. On successfull match, it will call SendConfigRequest() which
 * the subclass uses to send the desired request.
 *
 * Once the response is returned, HandleConfigResponse() is called in the
 * subclass.
 */

#include <string>
#include <google/protobuf/message.h>

#include <lla/LlaClient.h>
#include <lla/LlaDevice.h>
#include <lla/SimpleClient.h>
#include <lla/network/SelectServer.h>

using namespace std;

class LlaConfigurator;

/*
 * The observer class which repsonds to events
 */
class Observer: public lla::LlaClientObserver {
  public:
    Observer(LlaConfigurator *configurator): m_configurator(configurator) {}
    void Devices(const vector <lla::LlaDevice> devices, const string &error);
    void DeviceConfig(const string &reply, const string &error);
  private:
    LlaConfigurator *m_configurator;
};


/*
 * Inherit from this and implement HandleResponse()
 */
class LlaConfigurator {
  public:
    /*
     * @param device_id the device id to configure
     * @param plugin_id the expected plugin id for this device
     */
    LlaConfigurator(unsigned int device_id, lla_plugin_id plugin_id):
      m_alias(device_id),
      m_plugin_id(plugin_id),
      m_simple_client(NULL),
      m_client(NULL),
      m_ss(NULL),
      m_observer(NULL) {}
    virtual ~LlaConfigurator();

    /*
     * Setup the configurator
     */
    bool Setup();
    void Run() { m_ss->Run(); }
    void Terminate() { m_ss->Terminate(); }
    void HandleDevices(const vector <lla::LlaDevice> devices,
                       const string &error);
    bool SendMessage(google::protobuf::Message &message);

    // Subclasses implement this
    virtual void HandleConfigResponse(const string &reply,
                                      const string &error) = 0;
    virtual void SendConfigRequest() = 0;

  protected:
    unsigned int m_alias;
    lla_plugin_id m_plugin_id;
  private:
    lla::SimpleClient *m_simple_client;
    lla::LlaClient *m_client;
    lla::network::SelectServer *m_ss;
    Observer *m_observer;
};
