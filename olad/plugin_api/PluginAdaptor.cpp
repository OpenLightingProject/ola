/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * PluginAdaptor.cpp
 * Provides a wrapper for the DeviceManager and SelectServer objects so that
 * the plugins can register devices and file handles for events
 * Copyright (C) 2005 Simon Newton
 */

#include <string>
#include "ola/Callback.h"
#include "olad/PluginAdaptor.h"
#include "olad/PortBroker.h"
#include "olad/Preferences.h"
#include "olad/plugin_api/DeviceManager.h"

namespace ola {

using ola::io::SelectServerInterface;
using ola::thread::timeout_id;
using std::string;

PluginAdaptor::PluginAdaptor(DeviceManager *device_manager,
                             SelectServerInterface *select_server,
                             ExportMap *export_map,
                             PreferencesFactory *preferences_factory,
                             PortBrokerInterface *port_broker,
                             const std::string *instance_name):
  m_device_manager(device_manager),
  m_ss(select_server),
  m_export_map(export_map),
  m_preferences_factory(preferences_factory),
  m_port_broker(port_broker),
  m_instance_name(instance_name) {
}

bool PluginAdaptor::AddReadDescriptor(
    ola::io::ReadFileDescriptor *descriptor) {
  return m_ss->AddReadDescriptor(descriptor);
}

bool PluginAdaptor::AddReadDescriptor(
    ola::io::ConnectedDescriptor *descriptor,
    bool delete_on_close) {
  return m_ss->AddReadDescriptor(descriptor, delete_on_close);
}

void PluginAdaptor::RemoveReadDescriptor(
    ola::io::ReadFileDescriptor *descriptor) {
  m_ss->RemoveReadDescriptor(descriptor);
}

void PluginAdaptor::RemoveReadDescriptor(
    ola::io::ConnectedDescriptor *descriptor) {
  m_ss->RemoveReadDescriptor(descriptor);
}

bool PluginAdaptor::AddWriteDescriptor(
    ola::io::WriteFileDescriptor *descriptor) {
  return m_ss->AddWriteDescriptor(descriptor);
}

void PluginAdaptor::RemoveWriteDescriptor(
    ola::io::WriteFileDescriptor *descriptor) {
  m_ss->RemoveWriteDescriptor(descriptor);
}

timeout_id PluginAdaptor::RegisterRepeatingTimeout(
    unsigned int ms,
    Callback0<bool> *closure) {
  return m_ss->RegisterRepeatingTimeout(ms, closure);
}

timeout_id PluginAdaptor::RegisterRepeatingTimeout(
    const TimeInterval &interval,
    Callback0<bool> *closure) {
  return m_ss->RegisterRepeatingTimeout(interval, closure);
}

timeout_id PluginAdaptor::RegisterSingleTimeout(
    unsigned int ms,
    SingleUseCallback0<void> *closure) {
  return m_ss->RegisterSingleTimeout(ms, closure);
}

timeout_id PluginAdaptor::RegisterSingleTimeout(
    const TimeInterval &interval,
    SingleUseCallback0<void> *closure) {
  return m_ss->RegisterSingleTimeout(interval, closure);
}

void PluginAdaptor::RemoveTimeout(timeout_id id) {
  m_ss->RemoveTimeout(id);
}

void PluginAdaptor::Execute(ola::BaseCallback0<void> *closure) {
  m_ss->Execute(closure);
}

void PluginAdaptor::DrainCallbacks() {
  m_ss->DrainCallbacks();
}

bool PluginAdaptor::RegisterDevice(AbstractDevice *device) const {
  return m_device_manager->RegisterDevice(device);
}

bool PluginAdaptor::UnregisterDevice(AbstractDevice *device) const {
  return m_device_manager->UnregisterDevice(device);
}

Preferences *PluginAdaptor::NewPreference(const string &name) const {
  return m_preferences_factory->NewPreference(name);
}

const TimeStamp *PluginAdaptor::WakeUpTime() const {
  return m_ss->WakeUpTime();
}

const std::string PluginAdaptor::InstanceName() const {
  if (m_instance_name) {
    return *m_instance_name;
  } else {
    return "";
  }
}
}  // namespace ola
