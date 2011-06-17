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
 * E133Node.cpp
 * Copyright (C) 2011 Simon Newton
 */

#include "plugins/e131/e131/E131Includes.h"  //  NOLINT, this has to be first
#include <ola/Callback.h>
#include <ola/Logging.h>
#include <ola/network/InterfacePicker.h>

#include <string>

#include "plugins/e131/e131/CID.h"

#include "E133Component.h"
#include "E133Node.h"

using std::string;


E133Node::E133Node(const string &preferred_ip,
                   uint16_t port)
    : m_preferred_ip(preferred_ip),
      m_timeout_event(NULL),
      m_cid(ola::plugin::e131::CID::Generate()),
      m_transport(port),
      m_root_layer(&m_transport, m_cid),
      m_e133_layer(&m_root_layer),
      m_dmp_inflator(&m_e133_layer) {
}


E133Node::~E133Node() {
  if (m_timeout_event)
    m_ss.RemoveTimeout(m_timeout_event);

  m_dmp_inflator.RemoveRDMManagementHandler();
}


bool E133Node::Init() {
  ola::network::Interface interface;
  const ola::network::InterfacePicker *picker =
    ola::network::InterfacePicker::NewPicker();
  if (!picker->ChooseInterface(&interface, m_preferred_ip)) {
    OLA_INFO << "Failed to find an interface";
    delete picker;
    return false;
  }
  delete picker;

  if (!m_transport.Init(interface)) {
    return false;
  }

  ola::network::UdpSocket *socket = m_transport.GetSocket();
  m_ss.AddSocket(socket);
  m_e133_layer.SetInflator(&m_dmp_inflator);

  m_timeout_event = m_ss.RegisterRepeatingTimeout(
      500,
      ola::NewCallback(this, &E133Node::CheckForStaleRequests));

  m_dmp_inflator.SetRDMManagementHandler(
      ola::NewCallback(this, &E133Node::HandleManagementPacket));
  return true;
}


/**
 * Register a E133UniverseComponent
 * @param component E133UniverseComponent to register
 * @return true if the registration succeeded, false otherwise.
 */
bool E133Node::RegisterComponent(E133Component *component) {
  component_map::iterator iter = m_component_map.find(
      component->Universe());
  if (iter == m_component_map.end()) {
    m_component_map[component->Universe()] = component;
    component->SetE133Layer(&m_e133_layer);
    m_dmp_inflator.SetRDMHandler(
        component->Universe(),
        ola::NewCallback(component, &E133Component::HandlePacket));
    return true;
  }
  return false;
}


/**
 * Unregister a E133UniverseComponent
 * @param component E133UniverseComponent to register
 */
void E133Node::UnRegisterComponent(E133Component *component) {
  component_map::iterator iter = m_component_map.find(
      component->Universe());
  if (iter != m_component_map.end()) {
    component->SetE133Layer(NULL);
    m_dmp_inflator.RemoveRDMHandler(component->Universe());
    // TODO(simon): timeout all existing requests at this point
    m_component_map.erase(iter);
  }
}


/**
 * Handle management packets
 */
void E133Node::HandleManagementPacket(
    const ola::plugin::e131::TransportHeader &transport_header,
    const ola::plugin::e131::E133Header &e133_header,
    const string &request) {

  OLA_INFO << "Got management packet from " << transport_header.SourceIP();
  (void) e133_header;
  (void) request;
}


/**
 * Check for any requests that have timed out
 */
bool E133Node::CheckForStaleRequests() {
  const ola::TimeStamp *now = m_ss.WakeUpTime();
  component_map::iterator iter = m_component_map.begin();
  for (; iter != m_component_map.end(); ++iter) {
    iter->second->CheckForStaleRequests(now);
  }
  return true;
}
