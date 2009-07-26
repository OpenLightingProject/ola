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
 * Device.h
 * Header file for the Device class
 * Copyright (C) 2005  Simon Newton
 */

#ifndef OLA_DEVICE_H
#define OLA_DEVICE_H

#include <stdint.h>
#include <vector>
#include <string>

namespace google {
namespace protobuf {
  class Closure;
  class RpcController;
}
}

namespace ola {

class AbstractPlugin;
class AbstractPort;

using std::vector;
using std::string;

/*
 * The interface for a Device
 */
class AbstractDevice {
  public:
    AbstractDevice() {}
    virtual ~AbstractDevice() {}

    // return the name of this device
    virtual const string Name() const = 0;
    // return the plugin that owns this device
    virtual AbstractPlugin *Owner() const = 0;
    // return the a unique id of this device, this is guaranteed to be unique
    // and persist across restarts.
    virtual string UniqueId() const = 0;

    // stop the device
    virtual bool Stop() = 0;
    // configure this device
    virtual void Configure(google::protobuf::RpcController *controller,
                           const string &request,
                           string *response,
                           google::protobuf::Closure *done) = 0;
    // Fetch a list of all ports in this device
    virtual const vector<AbstractPort*> Ports() const = 0;
    // Lookup a particular port in this device
    virtual AbstractPort *GetPort(unsigned int port_id) const = 0;
};


/*
 * A partial implementation of a Device.
 */
class Device: public AbstractDevice {
  public:
    Device(AbstractPlugin *owner, const string &name);
    virtual ~Device();

    const string Name() const { return m_name; }
    AbstractPlugin *Owner() const { return m_owner; }
    string UniqueId() const;

    // Returns an id which is unique within the plugin
    virtual string DeviceId() const = 0;

    virtual bool Stop() { m_enabled = false; return true; }
    virtual void Configure(class google::protobuf::RpcController *controller,
                           const string &request,
                           string *response,
                           google::protobuf::Closure *done);
    virtual int AddPort(AbstractPort *port);
    virtual const vector<AbstractPort*> Ports() const;
    virtual AbstractPort *GetPort(unsigned int port_id) const;
    virtual void DeleteAllPorts();

  protected:
    bool m_enabled;

  private:
    AbstractPlugin *m_owner; // which plugin owns this device
    string m_name; // device name
    mutable string m_unique_id; // device id
    vector<ola::AbstractPort*> m_ports; // ports on the device

    Device(const Device&);
    Device& operator=(const Device&);
};

} //ola
#endif
