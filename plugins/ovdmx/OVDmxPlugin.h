#ifndef PLUGINS_OVDMX_OVDMXPLUGIN_H_
#define PLUGINS_OVDMX_OVDMXPLUGIN_H_

#include <string>
#include <vector>
#include "olad/Plugin.h"
#include "ola/plugin_id.h"

namespace ola {
namespace plugin {
namespace ovdmx {

class OVDmxDevice;

class OVDmxPlugin: public Plugin {
 public:
    explicit OVDmxPlugin(PluginAdaptor *plugin_adaptor):
      Plugin(plugin_adaptor) {
    }

    std::string Name() const { return PLUGIN_NAME; }
    std::string Description() const;
    ola_plugin_id Id() const { return OLA_PLUGIN_OVDMX; }
    std::string PluginPrefix() const { return PLUGIN_PREFIX; }

 private:
    bool StartHook();
    bool StopHook();
    bool SetDefaultPreferences();

    typedef std::vector<OVDmxDevice*> DeviceList;
    DeviceList m_devices;
    static const char PLUGIN_NAME[];
    static const char PLUGIN_PREFIX[];
    static const char OVDMX_DEVICE_PATH[];
    static const char OVDMX_DEVICE_NAME[];
    static const char DEVICE_KEY[];
};
}  // namespace ovdmx
}  // namespace plugin
}  // namespace ola

#endif  // PLUGINS_OVDMX_OVDMXPLUGIN_H_
