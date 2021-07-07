#ifndef PLUGINS_OVDMX_OVDMXDEVICE_H_
#define PLUGINS_OVDMX_OVDMXDEVICE_H_

#include <string>
#include "olad/Device.h"

namespace ola {
namespace plugin {
namespace ovdmx {

class OVDmxDevice: public ola::Device {
 public:
    OVDmxDevice(ola::AbstractPlugin *owner,
                  const std::string &name,
                  const std::string &path,
                  unsigned int device_id);

    // we only support one widget for now
    std::string DeviceId() const { return m_device_id; }

 protected:
    bool StartHook();

 private:
    std::string m_path;
    std::string m_device_id;
};
}  // namespace ovdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_OVDMX_OVDMXDEVICE_H_
