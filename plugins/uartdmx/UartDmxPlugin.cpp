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

#include <fcntl.h>
#include <errno.h>

#include <vector>
#include <string>

#include "ola/StringUtils.h"
#include "ola/io/IOUtils.h"
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

unsigned int UartDmxPlugin::DEFAULT_BREAK[] = "100";
const char UartDmxPlugin::K_BREAK[] = "break";
unsigned int UartDmxPlugin::DEFAULT_MALF[] = "100";
const char UartDmxPlugin::K_MALF[] = "malf";
const char UartDmxPlugin::PLUGIN_NAME[] = "UART native DMX";
const char UartDmxPlugin::PLUGIN_PREFIX[] = "uartdmx";
const char UartDmxPlugin::K_DEVICE[] = "device";


/**
 * Start the plug-in, using only the configured device (we cannot sensibly scan for
 * UARTs!) Stolen from the opendmx plugin
 */
bool UartDmxPlugin::StartHook() {
  vector<string> devices = m_preferences->GetMultipleValue(K_DEVICE);
  vector<string>::const_iterator iter = devices.begin();

  // start counting device ids from 0
  unsigned int device_id = 0;

  for (; iter != devices.end(); ++iter) {
    // first check if it's there
    int fd;
    if (ola::io::Open(*iter, O_WRONLY, &fd)) {
      // can open device, so shut the temporary file descriptor
      close(fd);
      UartDmxDevice *device = new UartDmxDevice(
          this,
          PLUGIN_NAME,
          *iter,
          device_id++,
          GetBreak(),
          GetMalf());
      // got a device, now lets see if we can configure it before we announce
      // it to the world
      if (device->GetWidget()->SetupOutput() == false) {
        // that failed, but other devices may succeed
        OLA_WARN << "Unable to setup device for output, device ignored "
                 << device->Description();
        delete device;
        continue;
      }
      // OK, device is good to go
      if (device->Start()) {
        m_devices.push_back(device);
        m_plugin_adaptor->RegisterDevice(device);
      } else {
        OLA_WARN << "Failed to start UartDevice for " << *iter;
        delete device;
      }
    } else {
      OLA_WARN << "Could not open " << *iter << " " << strerror(errno);
    }
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
"enabled = true\n"
"Enable this plugin (DISABLED by default).\n"
"device = /dev/ttyACM0\n"
"The device to use for DMX output (optional).\n"
"break = 100\n"
"The DMX break time in microseconds (optional).\n"
"malf = 100\n"
"The Mark After Last Frame time in microseconds (optional).\n"
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
                                     UIntValidator(8, 1000000)
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
