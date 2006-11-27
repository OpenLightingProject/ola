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
 * LlaDevice.cpp
 * Implementation of the LLA Client Device class
 * Copyright (C) 2005-2006 Simon Newton
 */

#include "LlaDevice.h"
#include "LlaPort.h"

/*
 *
 */
LlaDevice::LlaDevice(int id, int count, const string &name, int pid) :
	m_id(id),
	m_count(count),
	m_name(name),
	m_plugin(pid) {

}

LlaDevice::~LlaDevice() {
	reset_ports();
}


int LlaDevice::add_port(LlaPort *prt) {
	m_ports.push_back(prt);
	return 0;
}

const vector<LlaPort *> LlaDevice::get_ports() {
	return m_ports;
}

int LlaDevice::reset_ports() {
	vector<LlaPort *>::iterator iter;

	for(iter = m_ports.begin(); iter != m_ports.end(); ++iter) {
		delete (*iter);
	}

	m_ports.clear();
	return 0;
}
