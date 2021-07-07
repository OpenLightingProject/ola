#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

#include "ola/Logging.h"
#include "ola/io/IOUtils.h"
#include "olad/PluginAdaptor.h"
#include "olad/Preferences.h"
#include "plugins/ovdmx/OVDmxDevice.h"
#include "plugins/ovdmx/OVDmxPlugin.h"

namespace ola {
namespace plugin {
namespace ovdmx {

using ola::PluginAdaptor;
using std::string;
using std::vector;

const char OVDmxPlugin::OVDMX_DEVICE_PATH[] = "/dev/ttyACM0";
const char OVDmxPlugin::OVDMX_DEVICE_NAME[] = "OVDmx USB Device";
const char OVDmxPlugin::PLUGIN_NAME[] = "Omega Verksted DMX";
const char OVDmxPlugin::PLUGIN_PREFIX[] = "ovdmx";
const char OVDmxPlugin::DEVICE_KEY[] = "device";


/*
 * Start the plugin
 * TODO: scan /dev for devices?
 */
bool OVDmxPlugin::StartHook() {
  vector<string> devices = m_preferences->GetMultipleValue(DEVICE_KEY);
  vector<string>::const_iterator iter = devices.begin();

  // start counting device ids from 0
  unsigned int device_id = 0;

  for (; iter != devices.end(); ++iter) {
    // first check if it's there
    int fd;
    if (ola::io::Open(*iter, O_WRONLY, &fd)) {
      close(fd);
      OVDmxDevice *device = new OVDmxDevice(
          this,
          OVDMX_DEVICE_NAME,
          *iter,
          device_id++);
      if (device->Start()) {
        m_devices.push_back(device);
        m_plugin_adaptor->RegisterDevice(device);
      } else {
        OLA_WARN << "Failed to start OVDmxDevice for " << *iter;
        delete device;
      }
    } else {
      OLA_WARN << "Could not open " << *iter << " " << strerror(errno);
    }
  }
  return true;
}


/*
 * Stop the plugin
 * @return true on success, false on failure
 */
bool OVDmxPlugin::StopHook() {
  bool ret = true;
  DeviceList::iterator iter = m_devices.begin();
  for (; iter != m_devices.end(); ++iter) {
    m_plugin_adaptor->UnregisterDevice(*iter);
    ret &= (*iter)->Stop();
    delete *iter;
  }
  m_devices.clear();
  return ret;
}


/*
 * Return the description for this plugin
 */
string OVDmxPlugin::Description() const {
    return "OVDMX is awesome";
}


/*
 * Set default preferences.
 */
bool OVDmxPlugin::SetDefaultPreferences() {
  if (!m_preferences) {
    return false;
  }

  if (m_preferences->SetDefaultValue(DEVICE_KEY, StringValidator(),
                                     OVDMX_DEVICE_PATH)) {
    m_preferences->Save();
  }

  // check if this save correctly
  // we don't want to use it if null
  if (m_preferences->GetValue(DEVICE_KEY).empty()) {
    return false;
  }

  return true;
}
}  // namespace ovdmx
}  // namespace plugin
}  // namespace ola
