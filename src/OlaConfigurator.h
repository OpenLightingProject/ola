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
 *  OlaConfigurator.h
 *  Makes configuring devices easy
 *  Copyright (C) 2005-2009 Simon Newton
 *
 * The Configurator makes it easy to use the device specific ConfigureDevice()
 * rpc. For each device type you want to manage, subclass the Configurator and
 * implement the SendConfigRequest() and HandleConfigResponse() methods.
 * Upon calling Setup() the Configurator will send a DeviceInfo
 * request to check that the device type matches the plugin_id given in the
 * constructor. On successfull match, it will call SendConfigRequest() which
 * the subclass uses to send the desired request.
 *
 * Once the response is returned, HandleConfigResponse() is called in the
 * subclass.
 */

#include <google/protobuf/message.h>
#include <ola/OlaClient.h>
#include <ola/OlaDevice.h>
#include <ola/SimpleClient.h>
#include <ola/network/SelectServer.h>

#include <string>
#include <vector>

using std::string;
using std::vector;

#ifndef SRC_OLACONFIGURATOR_H_
#define SRC_OLACONFIGURATOR_H_

class OlaConfigurator;

/*
 * The observer class which repsonds to events
 */
class Observer: public ola::OlaClientObserver {
  public:
    explicit Observer(OlaConfigurator *configurator):
      m_configurator(configurator) {}
    void Devices(const vector <ola::OlaDevice> &devices, const string &error);
    void DeviceConfig(unsigned int device_alias,
                      const string &reply,
                      const string &error);
  private:
    OlaConfigurator *m_configurator;
};


/*
 * Inherit from this and implement HandleResponse()
 */
class OlaConfigurator {
  public:
    /*
     * @param device_id the device id to configure
     * @param plugin_id the expected plugin id for this device
     */
    OlaConfigurator(unsigned int device_id, ola::ola_plugin_id plugin_id):
      m_alias(device_id),
      m_plugin_id(plugin_id),
      m_simple_client(NULL),
      m_client(NULL),
      m_ss(NULL),
      m_observer(NULL) {}
    virtual ~OlaConfigurator();

    /*
     * Setup the configurator
     */
    bool Setup();
    void Run() { m_ss->Run(); }
    void Terminate() { m_ss->Terminate(); }
    void HandleDevices(const vector <ola::OlaDevice> &devices,
                       const string &error);
    bool SendMessage(const google::protobuf::Message &message);

    // Subclasses implement this
    virtual void HandleConfigResponse(const string &reply,
                                      const string &error) = 0;
    virtual void SendConfigRequest() = 0;

  protected:
    unsigned int m_alias;
    ola::ola_plugin_id m_plugin_id;

  private:
    ola::SimpleClient *m_simple_client;
    ola::OlaClient *m_client;
    ola::network::SelectServer *m_ss;
    Observer *m_observer;
};
#endif  // SRC_OLACONFIGURATOR_H_
