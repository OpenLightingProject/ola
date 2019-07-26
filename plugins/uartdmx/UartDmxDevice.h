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
 * UartDmxDevice.h
 * The DMX through a UART plugin for ola
 * Copyright (C) 2011 Rui Barreiros
 * Copyright (C) 2014 Richard Ash
 */

#ifndef PLUGINS_UARTDMX_UARTDMXDEVICE_H_
#define PLUGINS_UARTDMX_UARTDMXDEVICE_H_

#include <string>
#include <sstream>
#include <memory>
#include "ola/DmxBuffer.h"
#include "olad/Device.h"
#include "olad/Preferences.h"
#include "plugins/uartdmx/UartWidget.h"

namespace ola {
namespace plugin {
namespace uartdmx {

class UartDmxDevice : public Device {
 public:
  UartDmxDevice(AbstractPlugin *owner,
                class Preferences *preferences,
                const std::string &name,
                const std::string &path);
  ~UartDmxDevice();

  std::string DeviceId() const { return m_path; }
  UartWidget* GetWidget() { return m_widget.get(); }

 protected:
  bool StartHook();

 private:
  // Per device options
  std::string DeviceBreakKey() const;
  std::string DeviceMalfKey() const;
  void SetDefaults();

  std::auto_ptr<UartWidget> m_widget;
  class Preferences *m_preferences;
  const std::string m_name;
  const std::string m_path;
  unsigned int m_breakt;
  unsigned int m_malft;

  static const unsigned int DEFAULT_MALF;
  static const char K_MALF[];
  static const unsigned int DEFAULT_BREAK;
  static const char K_BREAK[];

  DISALLOW_COPY_AND_ASSIGN(UartDmxDevice);
};
}  // namespace uartdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_UARTDMX_UARTDMXDEVICE_H_
