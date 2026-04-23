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
 * MilInstDevice.h
 * Interface for the milinst device
 * Copyright (C) 2013 Peter Newman
 */

#ifndef PLUGINS_MILINST_MILINSTDEVICE_H_
#define PLUGINS_MILINST_MILINSTDEVICE_H_

#include <memory>
#include <string>

#include "olad/Device.h"

namespace ola {

class AbstractPlugin;

namespace plugin {
namespace milinst {

class MilInstDevice: public ola::Device {
 public:
  MilInstDevice(AbstractPlugin *owner,
                class Preferences *preferences,
                const std::string &dev_path);
  ~MilInstDevice();

  std::string DeviceId() const { return m_path; }
  ola::io::ConnectedDescriptor *GetSocket() const;

 protected:
  bool StartHook();
  void PrePortStop();

 private:
  std::string m_path;
  class Preferences *m_preferences;
  std::unique_ptr<class MilInstWidget> m_widget;

  static const char MILINST_DEVICE_NAME[];

  // Per device options
  std::string DeviceTypeKey() const;

  void SetDeviceDefaults();

  static const char TYPE_1463[];
  static const char TYPE_1553[];
};
}  // namespace milinst
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_MILINST_MILINSTDEVICE_H_
