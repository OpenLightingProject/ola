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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * UartDmxPlugin.cpp
 * The DMX through a UART plugin for ola
 * Copyright (C) 2011 Rui Barreiros
 * Copyright (C) 2014 Richard Ash
 */

#include <vector>
#include <string>

#include "ola/StringUtils.h"
#include "olad/Preferences.h"
#include "olad/PluginAdaptor.h"
#include "plugins/uartdmx/UartDmxPlugin.h"
#include "plugins/uartdmx/UartDmxDevice.h"
#include "plugins/uartdmx/UartWidget.h"

namespace ola {
namespace plugin {
namespace uartdmx {

using std::string;
using std::vector;

const char UartDmxPlugin::DEFAULT_BREAK[] = "100";
const char UartDmxPlugin::K_BREAK[] = "break";
const char UartDmxPlugin::DEFAULT_MALF[] = "100";
const char UartDmxPlugin::K_MALF[] = "malf";
const char UartDmxPlugin::PLUGIN_NAME[] = "UART native DMX";
const char UartDmxPlugin::PLUGIN_PREFIX[] = "uartdmx";

/**
 * Attempt to start a device and, if successfull, register it
 * Ownership of the UartDmxDevice is transfered to us here.
 */
void UartDmxPlugin::AddDevice(UartDmxDevice *device) {
  // Check if device is working before adding
  if (device->GetDevice()->SetupOutput() == false) {
    OLA_WARN << "Unable to setup device for output, device ignored "
             << device->Description();
    delete device;
    return;
  }

  if (device->Start()) {
      m_devices.push_back(device);
      m_plugin_adaptor->RegisterDevice(device);
  } else {
    OLA_WARN << "Failed to start UART " << device->Description();
    delete device;
  }
}


/**
 * Fetch a list of all UARTS and create a new device for each of them.
 */
bool UartDmxPlugin::StartHook() {
  typedef vector<UartWidgetInfo> UartInfoVector;
  UartWidgetInfoVector widgets;
  UartWidget::Widgets(&widgets);

  UartWidgetInfoVector::const_iterator iter;
  for (iter = widgets.begin(); iter != widgets.end(); ++iter) {
    AddDevice(new UartDmxDevice(this, *iter, GetFrequency()));
  }
  return true;
}


/**
 * Stop all the devices.
 */
bool UartDmxPlugin::StopHook() {
  UartDeviceVector::iterator iter;
  for (iter = m_devices.begin(); iter != m_devices.end(); ++iter) {
    m_plugin_adaptor->UnregisterDevice(*iter);
    (*iter)->Stop();
  }
  m_devices.clear();
  return true;
}


/**
 * Return a description for this plugin.
 */
string UartDmxPlugin::Description() const {
  return
"Native UART DMX Plugin\n"
"----------------------\n"
"\n"
"This plugin drives a supported POSIX UART (plus extensions)\n"
"to produce a direct DMX output stream. The host needs to\n"
"create the DMX stream itself as there is no external micro\n"
"This is tested with the on-board UART of the Raspberry Pi.\n"
"\n"
"--- Config file : ola-uartdmx.conf ---\n"
"\n"
"break = 100\n"
"The DMX break time in microseconds.\n"
"malf = 100\n"
"The Mark After Last Frame time in microseconds.\n"
"\n";
}


/**
 * Set the default preferences
 */
bool UartDmxPlugin::SetDefaultPreferences() {
  if (!m_preferences)
    return false;

  if (m_preferences->SetDefaultValue(UartDmxPlugin::K_BREAK,
                                     UIntValidator(88, 1000000),
                                     DEFAULT_BREAK))
  if (m_preferences->SetDefaultValue(UartDmxPlugin::K_MALF,
                                     UIntValidator(8, 1000000),
                                     DEFAULT_MALF))

    m_preferences->Save();

  if (m_preferences->GetValue(UartDmxPlugin::K_BREAK).empty())
    return false;
  if (m_preferences->GetValue(UartDmxPlugin::K_MALF).empty())
    return false;

  return true;
}


/**
 * Return the break time (in microseconds) as specified in the config file.
 */
int unsigned UartDmxPlugin::GetBreak() {
  unsigned int breakt;

  if (!StringToInt(m_preferences->GetValue(K_BREAK), &breakt))
    StringToInt(DEFAULT_BREAK, &breakt);
  return breakt;
}

/**
 * Return the malf time (in microseconds) as specified in the config file.
 */
int unsigned UartDmxPlugin::GetMalf() {
  unsigned int malft;

  if (!StringToInt(m_preferences->GetValue(K_MALF), &malft))
    StringToInt(DEFAULT_MALF, &malft);
  return malft;
}


}  // namespace uartdmx
}  // namespace plugin
}  // namespace ola
