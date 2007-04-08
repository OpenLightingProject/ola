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
 * LlaDevice.h
 * Interface to the LLA Client Device class
 * Copyright (C) 2005-2006 Simon Newton
 */

#ifndef LLA_CLIENT_DEVICE_H
#define LLA_CLIENT_DEVICE_H

using namespace std;

#include <string>
#include <vector>

class LlaDevice {

  public:
    LlaDevice(int id, int count, const string &name, int pid);
    ~LlaDevice();

    int get_id() { return m_id; }
    string get_name() { return m_name; }
    int port_count() { return m_count; }
    int get_plugid() { return m_plugin; }

    int add_port(class LlaPort *prt);
    int reset_ports();
    const vector<class LlaPort *> get_ports();

  private:
    LlaDevice(const LlaDevice&);
    LlaDevice operator=(const LlaDevice&);

    int m_id;    // device id
    int m_count;  // number of ports
    string m_name;  // device name
    int m_plugin;  // parent plugin id
    vector<class LlaPort *> m_ports;
};
#endif
