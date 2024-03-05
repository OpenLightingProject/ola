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
 * SPIPlugin.cpp
 * The SPI plugin for ola
 * Copyright (C) 2013 Simon Newton
 */

#include <memory>
#include <string>
#include <vector>
#include "ola/Logging.h"
#include "ola/StringUtils.h"
#include "ola/file/Util.h"
#include "ola/rdm/UID.h"
#include "ola/rdm/UIDAllocator.h"
#include "olad/PluginAdaptor.h"
#include "olad/Preferences.h"
#include "plugins/spi/SPIDevice.h"
#include "plugins/spi/SPIPlugin.h"


namespace ola {
namespace plugin {
namespace spi {

using ola::rdm::UID;
using std::auto_ptr;
using std::string;
using std::vector;

const char SPIPlugin::DEFAULT_BASE_UID[] = "7a70:00000100";
const char SPIPlugin::DEFAULT_SPI_DEVICE_PREFIX[] = "spidev";
const char SPIPlugin::PLUGIN_NAME[] = "SPI";
const char SPIPlugin::PLUGIN_PREFIX[] = "spi";
const char SPIPlugin::SPI_BASE_UID_KEY[] = "base_uid";
const char SPIPlugin::SPI_DEVICE_PREFIX_KEY[] = "device_prefix";

/*
 * Start the plugin
 * For now we just have one device.
 */
bool SPIPlugin::StartHook() {
  const string uid_str = m_preferences->GetValue(SPI_BASE_UID_KEY);
  auto_ptr<UID> base_uid(UID::FromString(uid_str));
  if (!base_uid.get()) {
    OLA_WARN << "Invalid UID " << uid_str << ", defaulting to "
             << DEFAULT_BASE_UID;
    base_uid.reset(UID::FromString(DEFAULT_BASE_UID));
    if (!base_uid.get()) {
      OLA_WARN << "Invalid UID " << DEFAULT_BASE_UID;
      return false;
    }
  }

  vector<string> spi_files;
  vector<string> spi_prefixes = m_preferences->GetMultipleValue(
      SPI_DEVICE_PREFIX_KEY);
  if (!ola::file::FindMatchingFiles("/dev", spi_prefixes, &spi_files)) {
    return false;
  }

  ola::rdm::UIDAllocator uid_allocator(*base_uid);
  vector<string>::const_iterator iter = spi_files.begin();
  for (; iter != spi_files.end(); ++iter) {
    SPIDevice *device = new SPIDevice(this, m_preferences, m_plugin_adaptor,
                                      *iter, &uid_allocator);

    if (!device) {
      continue;
    }

    if (!device->Start()) {
      delete device;
      continue;
    }
    m_devices.push_back(device);
    m_plugin_adaptor->RegisterDevice(device);
  }
  return true;
}


/*
 * Stop the plugin
 * @return true on success, false on failure
 */
bool SPIPlugin::StopHook() {
  vector<SPIDevice*>::iterator iter = m_devices.begin();
  bool ok = true;
  for (; iter != m_devices.end(); ++iter) {
    m_plugin_adaptor->UnregisterDevice(*iter);
    ok &= (*iter)->Stop();
    delete *iter;
  }
  return ok;
}


/*
 * Return the description for this plugin
 */
string SPIPlugin::Description() const {
  return
"SPI Plugin\n"
"----------------------------\n"
"\n"
"This plugin enables control of LED pixel strings using SPI. Each SPI output\n"
"is represented as an OLA Device. Devices can have multiple Ports, each of\n"
"which controls a pixel string. Each Port can use a different\n"
"personality (pixel type) and DMX start address, this allows a combination\n"
"of various strings lengths & pixel hardware types. The start address and\n"
"personality settings are controllable via RDM (each Port appears as a RDM\n"
"responder).\n"
"\n"
"To support multiple ports per SPI output, we use an SPI-Backend. Two\n"
"backends are supported right now, a software backend which concatenates\n"
"all the pixel data into a single buffer and a hardware multiplexer backend\n"
"which uses the GPIO pins to control an off-host multiplexer. It's\n"
"recommended to use the hardware multiplexer.\n"
"\n"
"--- Config file : ola-spi.conf ---\n"
"\n"
"base_uid = <string>\n"
"The starting UID to use for the SPI RDM , e.g. 7a70:00000100.\n"
"\n"
"device_prefix = <string>\n"
"The prefix of files to match in /dev. Usually set to 'spidev'. Each match\n"
"will instantiate a Device.\n"
"\n"
"--- Per Device Settings ---\n"
"<device>-spi-speed = <int>\n"
"The speed of the SPI bus, range is 0 - 32000000 Hz.\n"
"\n"
"<device>-ce-high = <bool>\n"
"The mode of the CE pin. Set to false this pulls the CE pin low when writing\n"
"data. Set to true this will pull the pin high when writing.\n"
"\n"
"<device>-backend = [software | hardware]\n"
"The backend to use to multiplex the SPI data.\n"
"\n"
"<device>-gpio-pin = <int>\n"
"The GPIO pins to use for the hardware multiplexer. Add one line for each\n"
"pin. The number of ports will be 2 ** (# of pins).\n"
"\n"
"<device>-ports = <int>\n"
"If the software backend is used, this defines the number of ports which\n"
"will be created.\n"
"\n"
"<device>-sync-port = <int>\n"
"Controls which port triggers a flush (write) of the SPI data. If set to -1\n"
"the SPI data is written when any port changes. This can result in a lot of\n"
"data writes (slow) and partial frames. If set to -2, the last port is used.\n"
"\n"
"--- Per Port Settings ---\n"
"Ports are indexed from 0.\n"
"\n"
"<device>-<port>-dmx-address = <int>\n"
"The DMX address to use. e.g. spidev0.1-0-dmx-address = 1\n"
"\n"
"<device>-<port>-device-label = <string>\n"
"The RDM device label to use.\n"
"\n"
"<device>-<port>-personality = <int>\n"
"The RDM personality to use.\n"
"\n"
"<device>-<port>-pixel-count = <int>\n"
"The number of pixels for this port. e.g. spidev0.1-1-pixel-count = 20.\n"
"\n";
}


/*
 * Load the plugin prefs and default to sensible values
 */
bool SPIPlugin::SetDefaultPreferences() {
  bool save = false;

  if (!m_preferences) {
    return false;
  }

  save |= m_preferences->SetDefaultValue(SPI_DEVICE_PREFIX_KEY,
                                         StringValidator(),
                                         DEFAULT_SPI_DEVICE_PREFIX);
  save |= m_preferences->SetDefaultValue(SPI_BASE_UID_KEY,
                                         StringValidator(),
                                         DEFAULT_BASE_UID);
  if (save) {
    m_preferences->Save();
  }

  if (m_preferences->GetValue(SPI_DEVICE_PREFIX_KEY).empty()) {
    return false;
  }

  return true;
}
}  // namespace spi
}  // namespace plugin
}  // namespace ola
