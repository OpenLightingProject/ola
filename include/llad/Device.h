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

#ifndef LLA_DEVICE_H
#define LLA_DEVICE_H

#include <stdint.h>
#include <vector>
#include <string>

namespace google {
namespace protobuf {
  class Closure;
  class RpcController;
}
}

namespace lla {

class AbstractPlugin;
class AbstractPort;

using std::vector;
using std::string;

class AbstractDevice {
  public:
    AbstractDevice() {}
    virtual ~AbstractDevice() {}

    virtual const string Name() const = 0;
    virtual AbstractPlugin *Owner() const = 0;
    virtual unsigned int DeviceId() const = 0;
    virtual void SetDeviceId(unsigned int device_id) = 0;

    // for the subclasses
    virtual bool Stop() = 0;
    virtual void Configure(google::protobuf::RpcController *controller,
                           const string &request,
                           string *response,
                           google::protobuf::Closure *done) = 0;
    virtual int AddPort(AbstractPort *port) = 0;
    virtual const vector<AbstractPort*> Ports() const = 0;
    virtual AbstractPort *GetPort(unsigned int port_id) const = 0;
};


class Device: public AbstractDevice {
  public:
    Device(AbstractPlugin *owner, const string &name);
    virtual ~Device();

    const string Name() const { return m_name; }
    AbstractPlugin *Owner() const { return m_owner; }
    void SetDeviceId(unsigned int device_id) { m_device_id = device_id; }
    unsigned int DeviceId() const { return m_device_id; }

    // for the subclasses
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
    Device(const Device&);
    Device& operator=(const Device&);
    AbstractPlugin *m_owner; // which plugin owns this device
    string m_name; // device name
    unsigned int m_device_id;
    vector<lla::AbstractPort*> m_ports; // ports on the device
};

} //lla
#endif
