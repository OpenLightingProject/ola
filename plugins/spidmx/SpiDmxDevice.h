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
 * SpiDmxDevice.h
 * This represents a device and manages thread, widget and input/output ports.
 * Copyright (C) 2017 Florian Edelmann
 */

#ifndef PLUGINS_SPIDMX_SPIDMXDEVICE_H_
#define PLUGINS_SPIDMX_SPIDMXDEVICE_H_

#include <string>
#include <sstream>
#include <memory>
#include "ola/DmxBuffer.h"
#include "olad/Device.h"
#include "olad/Preferences.h"
#include "plugins/spidmx/SpiDmxWidget.h"
#include "plugins/spidmx/SpiDmxThread.h"

namespace ola {
namespace plugin {
namespace spidmx {

class SpiDmxDevice : public Device {
 public:
  SpiDmxDevice(AbstractPlugin *owner,
               class Preferences *preferences,
               PluginAdaptor *plugin_adaptor,
               const std::string &name,
               const std::string &path);
  ~SpiDmxDevice();

  std::string DeviceId() const { return m_path; }
  SpiDmxWidget* GetWidget() { return m_widget.get(); }

 protected:
  bool StartHook();

 private:
  // Per device options
  std::string DeviceBlocklength() const;
  void SetDefaults();


  std::auto_ptr<SpiDmxWidget> m_widget;
  std::auto_ptr<SpiDmxThread> m_thread;
  class Preferences *m_preferences;
  PluginAdaptor *m_plugin_adaptor;
  const std::string m_name;
  const std::string m_path;
  unsigned int m_blocklength;

  static const unsigned int PREF_BLOCKLENGTH_DEFAULT;
  static const char PREF_BLOCKLENGTH_KEY[];

  DISALLOW_COPY_AND_ASSIGN(SpiDmxDevice);
};

}  // namespace spidmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_SPIDMX_SPIDMXDEVICE_H_
