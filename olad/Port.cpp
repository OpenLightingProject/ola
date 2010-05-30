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
 * Port.cpp
 * Base implementation of the Port class.
 * Copyright (C) 2005-2009 Simon Newton
 *
 * Unfortunately this file contains a lot of code duplication.
 */

#include <string>
#include "olad/Device.h"
#include "olad/Port.h"

namespace ola {

/*
 * Create a new basic input port
 */
BasicInputPort::BasicInputPort(AbstractDevice *parent,
                               unsigned int port_id,
                               const TimeStamp *wake_time):
    m_port_id(port_id),
    m_priority(DmxSource::PRIORITY_DEFAULT),
    m_priority_mode(PRIORITY_MODE_INHERIT),
    m_port_string(""),
    m_universe(NULL),
    m_device(parent),
    m_wakeup_time(wake_time) {
}


bool BasicInputPort::SetUniverse(Universe *new_universe) {
  Universe *old_universe = GetUniverse();
  if (old_universe == new_universe)
    return true;

  if (PreSetUniverse(old_universe, new_universe)) {
    m_universe = new_universe;
    PostSetUniverse(old_universe, new_universe);
    return true;
  }
  return false;
}


string BasicInputPort::UniqueId() const {
  if (m_port_string.empty()) {
    std::stringstream str;
    if (m_device)
      str << m_device->UniqueId() << "-I-" << m_port_id;
    m_port_string = str.str();
  }
  return m_port_string;
}



bool BasicInputPort::SetPriority(uint8_t priority) {
  if (priority > DmxSource::PRIORITY_MAX)
    return false;

  m_priority = priority;
  return true;
}


/*
 * Called when there is new data for this port
 */
void BasicInputPort::DmxChanged() {
  if (GetUniverse()) {
    const DmxBuffer &buffer = ReadDMX();
    uint8_t priority = (PriorityCapability() == CAPABILITY_FULL &&
                        GetPriorityMode() == PRIORITY_MODE_INHERIT ?
                        InheritedPriority() :
                        GetPriority());
    m_dmx_source.UpdateData(buffer, *m_wakeup_time, priority);
    GetUniverse()->PortDataChanged(this);
  }
}


/*
 * Create a new BasicOutputPort
 */
BasicOutputPort::BasicOutputPort(AbstractDevice *parent,
                                 unsigned int port_id):
    m_port_id(port_id),
    m_priority(DmxSource::PRIORITY_DEFAULT),
    m_priority_mode(PRIORITY_MODE_INHERIT),
    m_port_string(""),
    m_universe(NULL),
    m_device(parent) {
}


bool BasicOutputPort::SetUniverse(Universe *new_universe) {
  Universe *old_universe = GetUniverse();
  if (old_universe == new_universe)
    return true;

  if (PreSetUniverse(old_universe, new_universe)) {
    m_universe = new_universe;
    PostSetUniverse(old_universe, new_universe);
    return true;
  }
  return false;
}


string BasicOutputPort::UniqueId() const {
  if (m_port_string.empty()) {
    std::stringstream str;
    if (m_device)
      str << m_device->UniqueId() << "-O-" << m_port_id;
    m_port_string = str.str();
  }
  return m_port_string;
}


bool BasicOutputPort::SetPriority(uint8_t priority) {
  if (priority > DmxSource::PRIORITY_MAX)
    return false;

  m_priority = priority;
  return true;
}


/*
 * This allows switching based on Port type.
 */
template<class PortClass>
bool IsInputPort() {
  return true;
}

template<>
bool IsInputPort<OutputPort>() {
  return false;
}

template<>
bool IsInputPort<InputPort>() {
  return true;
}
}  // ola
