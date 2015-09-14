#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <string>

#include "plugins/ovdmx/OVDmxDevice.h"
#include "plugins/ovdmx/OVDmxPort.h"

namespace ola {
namespace plugin {
namespace ovdmx {

using ola::Device;
using std::string;


/*
 * Create a new device
 * @param owner
 * @param name
 * @param path to device
 */
OVDmxDevice::OVDmxDevice(AbstractPlugin *owner,
                             const string &name,
                             const string &path,
                             unsigned int device_id)
    : Device(owner, name),
      m_path(path) {
  std::ostringstream str;
  str << device_id;
  m_device_id = str.str();
}



/*
 * Start this device
 */
bool OVDmxDevice::StartHook() {
  AddPort(new OVDmxOutputPort(this, 0, m_path));
  return true;
}
}  // namespace ovdmx
}  // namespace plugin
}  // namespace ola
