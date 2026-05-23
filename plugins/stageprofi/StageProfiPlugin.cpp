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
 * StageProfiPlugin.cpp
 * The StageProfi plugin for ola
 * Copyright (C) 2006 Simon Newton
 */

#include "plugins/stageprofi/StageProfiPlugin.h"

#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <vector>

#include "ola/Logging.h"
#include "ola/stl/STLUtils.h"
#include "olad/PluginAdaptor.h"
#include "ola/network/IPV4Address.h"
#include "ola/network/TCPSocket.h"
#include "olad/Preferences.h"
#include "plugins/stageprofi/StageProfiDetector.h"
#include "plugins/stageprofi/StageProfiDevice.h"
#include "plugins/stageprofi/StageProfiPluginDescription.h"
#include "plugins/stageprofi/StageProfiWidget.h"

namespace ola {
namespace plugin {
namespace stageprofi {

using ola::io::ConnectedDescriptor;
using std::unique_ptr;
using std::string;
using std::vector;

const char StageProfiPlugin::STAGEPROFI_DEVICE_PATH[] = "/dev/ttyUSB0";
const char StageProfiPlugin::STAGEPROFI_DEVICE_NAME[] = "StageProfi Device";
const char StageProfiPlugin::PLUGIN_NAME[] = "StageProfi";
const char StageProfiPlugin::PLUGIN_PREFIX[] = "stageprofi";
const char StageProfiPlugin::DEVICE_KEY[] = "device";

namespace {

void DeleteStageProfiDevice(StageProfiDevice *device) {
  delete device;
}
}  // namespace

StageProfiPlugin::~StageProfiPlugin() {
}

bool StageProfiPlugin::StartHook() {
  vector<string> device_names = m_preferences->GetMultipleValue(DEVICE_KEY);
  m_detector.reset(new StageProfiDetector(
      m_plugin_adaptor, device_names,
      NewCallback(this, &StageProfiPlugin::NewWidget)));
  m_detector->Start();
  return true;
}

bool StageProfiPlugin::StopHook() {
  m_detector->Stop();

  DeviceMap::iterator iter = m_devices.begin();
  for (; iter != m_devices.end(); ++iter) {
    DeleteDevice(iter->second);
  }
  m_devices.clear();
  return true;
}

string StageProfiPlugin::Description() const {
    return plugin_description;
}

bool StageProfiPlugin::SetDefaultPreferences() {
  if (!m_preferences) {
    return false;
  }

  bool save = false;

  save |= m_preferences->SetDefaultValue(DEVICE_KEY, StringValidator(),
                                         STAGEPROFI_DEVICE_PATH);

  if (save) {
    m_preferences->Save();
  }

  if (m_preferences->GetValue(DEVICE_KEY).empty()) {
    return false;
  }
  return true;
}

void StageProfiPlugin::NewWidget(const std::string &widget_path,
                                 ConnectedDescriptor *descriptor) {
  OLA_INFO << "New StageProfiWidget: " << widget_path;

  DeviceMap::iterator iter = STLLookupOrInsertNull(&m_devices, widget_path);
  if (iter->second) {
    OLA_WARN << "Pre-existing StageProfiDevice for " << widget_path;
    return;
  }

  unique_ptr<StageProfiDevice> device(new StageProfiDevice(
      this,
      new StageProfiWidget(
          m_plugin_adaptor, descriptor, widget_path,
          NewSingleCallback(this, &StageProfiPlugin::DeviceRemoved,
                            widget_path)),
      STAGEPROFI_DEVICE_NAME));

  if (!device->Start()) {
    OLA_INFO << "Failed to start StageProfiDevice";
    return;
  }

  m_plugin_adaptor->RegisterDevice(device.get());
  iter->second = device.release();
}

void StageProfiPlugin::DeviceRemoved(std::string widget_path) {
  OLA_INFO << "StageProfi device " << widget_path << " was removed";
  StageProfiDevice *device = STLReplacePtr(&m_devices, widget_path, NULL);
  if (device) {
    // Since this is called within the call stack of the StageProfiWidget
    // itself, we need to schedule deletion for later.
    m_plugin_adaptor->UnregisterDevice(device);
    device->Stop();
    m_plugin_adaptor->Execute(
        NewSingleCallback(DeleteStageProfiDevice, device));
  }
  m_detector->ReleaseWidget(widget_path);
}

void StageProfiPlugin::DeleteDevice(StageProfiDevice *device) {
  if (device) {
    m_plugin_adaptor->UnregisterDevice(device);
    device->Stop();
    delete device;
  }
}
}  // namespace stageprofi
}  // namespace plugin
}  // namespace ola
