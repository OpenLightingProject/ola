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
 * MilInstDevice.cpp
 * MilInst device
 * Copyright (C) 2013 Peter Newman
 */

#include <set>
#include <string>

#include "ola/Logging.h"
#include "olad/Preferences.h"
#include "plugins/milinst/MilInstPort.h"
#include "plugins/milinst/MilInstWidget1463.h"
#include "plugins/milinst/MilInstWidget1553.h"

namespace ola {
namespace plugin {
namespace milinst {

using ola::AbstractPlugin;
using ola::io::ConnectedDescriptor;
using std::set;
using std::string;

const char MilInstDevice::MILINST_DEVICE_NAME[] = "Milford Instruments Device";

const char MilInstDevice::TYPE_1463[] = "1-463";
const char MilInstDevice::TYPE_1553[] = "1-553";


/*
 * Create a new device
 *
 * @param owner  the plugin that owns this device
 * @param name  the device name
 * @param dev_path  path to the pro widget
 */
MilInstDevice::MilInstDevice(AbstractPlugin *owner,
                             Preferences *preferences,
                             const string &dev_path)
    : Device(owner, MILINST_DEVICE_NAME),
      m_path(dev_path),
      m_preferences(preferences) {

  SetDeviceDefaults();

  string type = m_preferences->GetValue(DeviceTypeKey());
  OLA_DEBUG << "Got type " << type;

  if (type.compare(TYPE_1553) == 0) {
    m_widget.reset(new MilInstWidget1553(m_path, m_preferences));
  } else {
    m_widget.reset(new MilInstWidget1463(m_path));
  }
}


/*
 * Destroy this device
 */
MilInstDevice::~MilInstDevice() {
  // Stub destructor for compatibility with MilInstWidget subclasses
}


/*
 * Start this device
 */
bool MilInstDevice::StartHook() {
  if (!m_widget.get()) {
    return false;
  }

  if (!m_widget->Connect()) {
    OLA_WARN << "Failed to connect to " << m_path;
    return false;
  }

  if (!m_widget->DetectDevice()) {
    OLA_WARN << "No device found at " << m_path;
    return false;
  }

  MilInstOutputPort *port = new MilInstOutputPort(this, 0, m_widget.get());
  AddPort(port);
  return true;
}


/*
 * Stop this device
 */
void MilInstDevice::PrePortStop() {
  // disconnect from widget
  m_widget->Disconnect();
}


/*
 * return the sd for this device
 */
ConnectedDescriptor *MilInstDevice::GetSocket() const {
  return m_widget->GetSocket();
}

string MilInstDevice::DeviceTypeKey() const {
  return m_path + "-type";
}

void MilInstDevice::SetDeviceDefaults() {
  bool save = false;

  set<string> valid_types;
  valid_types.insert(TYPE_1463);
  valid_types.insert(TYPE_1553);

  // Set generic device options
  save |= m_preferences->SetDefaultValue(DeviceTypeKey(),
                                         SetValidator<string>(valid_types),
                                         TYPE_1463);

  if (save) {
    m_preferences->Save();
  }
}
}  // namespace milinst
}  // namespace plugin
}  // namespace ola
