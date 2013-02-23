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
 * SPIPlugin.cpp
 * The SPI plugin for ola
 * Copyright (C) 2013 Simon Newton
 */

#include <dirent.h>
#include <errno.h>
#include <string>
#include <vector>
#include "ola/StringUtils.h"
#include "ola/Logging.h"
#include "olad/PluginAdaptor.h"
#include "olad/Preferences.h"
#include "plugins/spi/SPIDevice.h"
#include "plugins/spi/SPIPlugin.h"


namespace ola {
namespace plugin {
namespace spi {

const char SPIPlugin::PLUGIN_NAME[] = "SPI";
const char SPIPlugin::PLUGIN_PREFIX[] = "spi";


/*
 * Start the plugin
 * For now we just have one device.
 */
bool SPIPlugin::StartHook() {
  vector<string> spi_files;
  vector<string> spi_prefixes = m_preferences->GetMultipleValue(
      SPIDevice::SPI_DEVICE_PREFIX_KEY);
  FindMatchingFiles("/dev", spi_prefixes, &spi_files);
  vector<string>::const_iterator iter = spi_files.begin();
  for (; iter != spi_files.end(); ++iter) {
    SPIDevice *device = new SPIDevice(this, m_preferences, m_plugin_adaptor,
                                      *iter);

    if (!device)
      continue;

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
"This plugin allows you to control LED strings using SPI.\n"
"\n"
"--- Config file : ola-pathport.conf ---\n"
"\n"
"device_prefix = <string>\n"
"The prefix of files to match in /dev. Usually set to 'spidev'\n"
"\n";
}


/*
 * Load the plugin prefs and default to sensible values
 */
bool SPIPlugin::SetDefaultPreferences() {
  bool save = false;

  if (!m_preferences)
    return false;

  save |= m_preferences->SetDefaultValue(SPIDevice::SPI_DEVICE_PREFIX_KEY,
                                         StringValidator(),
                                         SPIDevice::DEFAULT_SPI_DEVICE_PREFIX);

  if (save)
    m_preferences->Save();

  if (m_preferences->GetValue(SPIDevice::SPI_DEVICE_PREFIX_KEY).empty())
    return false;

  return true;
}


/*
 * Return files in a given directory that match a set of prefixes. Each file
 * must match at least one prefix to be returned, An empty prefix list will
 * return no files.
 * @param directory the path of the directory to search
 * @param prefixes the list of prefixes to match files against
 * @param files a pointer to the vector in which to return the absoluate path
 *   file names.
 * TODO(simon): move this to the common code something.
 */
void SPIPlugin::FindMatchingFiles(const string &directory,
                                  const vector<string> &prefixes,
                                  vector<string> *files) {
  if (directory.empty() || prefixes.empty())
    return;

  DIR *dp;
  struct dirent dir_ent;
  struct dirent *dir_ent_p;
  if ((dp  = opendir(directory.data())) == NULL) {
    OLA_WARN << "Could not open " << directory << ":" << strerror(errno);
    return;
  }

  readdir_r(dp, &dir_ent, &dir_ent_p);
  while (dir_ent_p != NULL) {
    vector<string>::const_iterator iter;
    for (iter = prefixes.begin(); iter != prefixes.end(); ++iter) {
      if (!strncmp(dir_ent_p->d_name, iter->data(), iter->size())) {
        stringstream str;
        str << directory << "/" << dir_ent_p->d_name;
        files->push_back(str.str());
      }
    }
    readdir_r(dp, &dir_ent, &dir_ent_p);
  }
  closedir(dp);
}
}  // spi
}  // plugin
}  // ola
