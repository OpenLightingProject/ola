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
 * UartDmxPlugin.cpp
 * The DMX through a UART plugin for ola
 * Copyright (C) 2011 Rui Barreiros
 * Copyright (C) 2014 Richard Ash
 */

#include <fcntl.h>
#include <errno.h>

#include <memory>
#include <string>
#include <vector>

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

const char UartDmxPlugin::PLUGIN_NAME[] = "UART native DMX";
const char UartDmxPlugin::PLUGIN_PREFIX[] = "uartdmx";
const char UartDmxPlugin::K_DEVICE[] = "device";
const char UartDmxPlugin::DEFAULT_DEVICE[] = "/dev/ttyACM0";

/*
 * Start the plug-in, using only the configured device(s) (we cannot sensibly
 * scan for UARTs!). Stolen from the opendmx plugin.
 */
bool UartDmxPlugin::StartHook() {
  vector<string> devices = m_preferences->GetMultipleValue(K_DEVICE);
  vector<string>::const_iterator iter;  // iterate over devices

  // start counting device ids from 0

  for (iter = devices.begin(); iter != devices.end(); ++iter) {
    // first check if device configured
    if (iter->empty()) {
      OLA_DEBUG << "No path configured for device, please set one in "
                << "ola-uartdmx.conf";
      continue;
    }

    OLA_DEBUG << "Trying to open UART device " << *iter;
    int fd;
    if (!ola::io::Open(*iter, O_WRONLY, &fd)) {
      OLA_WARN << "Could not open " << *iter << " " << strerror(errno);
      continue;
    }

    // can open device, so shut the temporary file descriptor
    close(fd);
    std::auto_ptr<UartDmxDevice> device(new UartDmxDevice(
        this, m_preferences, PLUGIN_NAME, *iter));

    // got a device, now lets see if we can configure it before we announce
    // it to the world
    if (!device->GetWidget()->SetupOutput()) {
      OLA_WARN << "Unable to setup device for output, device ignored "
               << device->DeviceId();
      continue;
    }
    // OK, device is good to go
    if (!device->Start()) {
      OLA_WARN << "Failed to start UartDmxDevice for " << *iter;
      continue;
    }

    OLA_DEBUG << "Started UartDmxDevice " << *iter;
    m_plugin_adaptor->RegisterDevice(device.get());
    m_devices.push_back(device.release());
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
    delete *iter;
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
"create the DMX stream itself as there is no external microcontroller.\n"
"This is tested with the on-board UART of the Raspberry Pi.\n"
"See here for a possible schematic:\n"
"http://eastertrail.blogspot.co.uk/2014/04/command-and-control-ii.html\n"
"\n"
"--- Config file : ola-uartdmx.conf ---\n"
"\n"
"enabled = true\n"
"Enable this plugin (DISABLED by default).\n"
"device = /dev/ttyAMA0\n"
"The device to use for DMX output (optional). Multiple devices are supported "
"if the hardware exists. On later software it may also be /dev/serial0. Using "
"USB-serial adapters is not supported (try the ftdidmx plugin instead).\n"
"--- Per Device Settings (using above device name) ---\n"
"<device>-break = 100\n"
"The DMX break time in microseconds for this device (optional).\n"
"<device>-malf = 100\n"
"The Mark After Last Frame time in microseconds for this device (optional).\n"
"\n";
}


/**
 * Set the default preferences
 */
bool UartDmxPlugin::SetDefaultPreferences() {
  if (!m_preferences) {
    return false;
  }

  // only insert default device name, no others at this stage
  bool save = m_preferences->SetDefaultValue(K_DEVICE, StringValidator(),
                                             DEFAULT_DEVICE);
  if (save) {
    m_preferences->Save();
  }

  // Just check key exists, as we've set it to ""
  if (!m_preferences->HasKey(K_DEVICE)) {
    return false;
  }
  return true;
}
}  // namespace uartdmx
}  // namespace plugin
}  // namespace ola
