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

#ifndef INCLUDE_OLAD_DEVICE_H_
#define INCLUDE_OLAD_DEVICE_H_

#include <stdint.h>
#include <olad/Port.h>
#include <map>
#include <string>
#include <vector>

namespace google {
namespace protobuf {
  class Closure;
  class RpcController;
}
}

namespace ola {

class AbstractPlugin;

using std::map;
using std::pair;
using std::string;
using std::vector;

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

    // allow input and output ports to be patched to the same univese
    virtual bool AllowLooping() const = 0;

    // allow multiple ports of the same type to be patched to the same
    // universe.
    virtual bool AllowMultiPortPatching() const = 0;

    // Fetch a list of all ports in this device
    virtual void InputPorts(vector<InputPort*> *ports) const = 0;
    virtual void OutputPorts(vector<OutputPort*> *ports) const = 0;

    // Lookup a particular port in this device
    virtual InputPort *GetInputPort(unsigned int port_id) const = 0;
    virtual OutputPort *GetOutputPort(unsigned int port_id) const = 0;

    // configure this device
    virtual void Configure(google::protobuf::RpcController *controller,
                           const string &request,
                           string *response,
                           google::protobuf::Closure *done) = 0;
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

    virtual bool Stop() {
      m_enabled = false;
      return true;
    }

    bool AddPort(InputPort *port);
    bool AddPort(OutputPort *port);
    void InputPorts(vector<InputPort*> *ports) const;
    void OutputPorts(vector<OutputPort*> *ports) const;

    InputPort *GetInputPort(unsigned int port_id) const;
    OutputPort *GetOutputPort(unsigned int port_id) const;

    // Free all ports
    void DeleteAllPorts();

    // Handle a Configure request
    virtual void Configure(class google::protobuf::RpcController *controller,
                           const string &request,
                           string *response,
                           google::protobuf::Closure *done);

  protected:
    bool m_enabled;

  private:
    typedef map<unsigned int, InputPort*> input_port_map;
    typedef map<unsigned int, OutputPort*> output_port_map;

    AbstractPlugin *m_owner;  // which plugin owns this device
    string m_name;  // device name
    mutable string m_unique_id;  // device id
    input_port_map m_input_ports;
    output_port_map m_output_ports;

    Device(const Device&);
    Device& operator=(const Device&);

    template<class PortClass>
    bool GenericAddPort(PortClass *port,
                        map<unsigned int, PortClass*> *ports);

    template<class PortClass>
    void GenericFetchPortsVector(
        vector<PortClass*> *ports,
        const map<unsigned int, PortClass*> &port_map) const;

    template <class PortClass>
    void GenericDeletePort(PortClass *p);
};
}  // ola
#endif  // INCLUDE_OLAD_DEVICE_H_
