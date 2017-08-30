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
 * GpioPlugin.cpp
 * The General Purpose digital I/O plugin.
 * Copyright (C) 2014 Simon Newton
 */

#include "plugins/gpio/GpioPlugin.h"

#include <memory>
#include <string>
#include <vector>
#include "olad/Preferences.h"
#include "ola/Logging.h"
#include "ola/StringUtils.h"
#include "plugins/gpio/GpioDevice.h"
#include "plugins/gpio/GpioDriver.h"
#include "plugins/gpio/GpioPluginDescription.h"

namespace ola {
namespace plugin {
namespace gpio {

using std::auto_ptr;
using std::string;
using std::vector;

const char GpioPlugin::GPIO_PINS_KEY[] = "gpio_pins";
const char GpioPlugin::GPIO_SLOT_OFFSET_KEY[] = "gpio_slot_offset";
const char GpioPlugin::GPIO_TURN_OFF_KEY[] = "gpio_turn_off";
const char GpioPlugin::GPIO_TURN_ON_KEY[] = "gpio_turn_on";
const char GpioPlugin::PLUGIN_NAME[] = "GPIO";
const char GpioPlugin::PLUGIN_PREFIX[] = "gpio";

bool GpioPlugin::StartHook() {
  GpioDriver::Options options;
  if (!StringToInt(m_preferences->GetValue(GPIO_TURN_ON_KEY),
                   &options.turn_on)) {
    OLA_WARN << "Invalid value for " << GPIO_TURN_ON_KEY;
    return false;
  }

  if (!StringToInt(m_preferences->GetValue(GPIO_TURN_OFF_KEY),
                   &options.turn_off)) {
    OLA_WARN << "Invalid value for " << GPIO_TURN_OFF_KEY;
    return false;
  }

  if (!StringToInt(m_preferences->GetValue(GPIO_SLOT_OFFSET_KEY),
                   &options.start_address)) {
    OLA_WARN << "Invalid value for " << GPIO_SLOT_OFFSET_KEY;
    return false;
  }

  if (options.turn_off >= options.turn_on) {
    OLA_WARN << GPIO_TURN_OFF_KEY << " must be strictly less than "
             << GPIO_TURN_ON_KEY;
    return false;
  }

  vector<string> pin_list;
  StringSplit(m_preferences->GetValue(GPIO_PINS_KEY), &pin_list, ",");
  vector<string>::const_iterator iter = pin_list.begin();
  for (; iter != pin_list.end(); ++iter) {
    if (iter->empty()) {
      continue;
    }

    uint16_t pin;
    if (!StringToInt(*iter, &pin)) {
      OLA_WARN << "Invalid value for GPIO pin: " << *iter;
      return false;
    }
    options.gpio_pins.push_back(pin);
  }

  if (options.gpio_pins.empty()) {
    return true;
  }

  std::auto_ptr<GpioDevice> device(new GpioDevice(this, options));
  if (!device->Start()) {
    return false;
  }

  m_plugin_adaptor->RegisterDevice(device.get());
  m_device = device.release();
  return true;
}

bool GpioPlugin::StopHook() {
  if (m_device) {
    m_plugin_adaptor->UnregisterDevice(m_device);
    m_device->Stop();
    delete m_device;
    m_device = NULL;
  }
  return true;
}

string GpioPlugin::Description() const {
  return plugin_description;
}

bool GpioPlugin::SetDefaultPreferences() {
  bool save = false;

  if (!m_preferences)
    return false;

  save |= m_preferences->SetDefaultValue(GPIO_PINS_KEY,
                                         StringValidator(),
                                         "");
  save |= m_preferences->SetDefaultValue(GPIO_SLOT_OFFSET_KEY,
                                         UIntValidator(1, DMX_UNIVERSE_SIZE),
                                         "1");
  save |= m_preferences->SetDefaultValue(
      GPIO_TURN_ON_KEY,
      UIntValidator(DMX_MIN_SLOT_VALUE + 1, DMX_MAX_SLOT_VALUE),
      "128");
  save |= m_preferences->SetDefaultValue(
      GPIO_TURN_OFF_KEY,
      UIntValidator(DMX_MIN_SLOT_VALUE, DMX_MAX_SLOT_VALUE - 1),
      "127");

  if (save) {
    m_preferences->Save();
  }

  if (m_preferences->GetValue(GPIO_SLOT_OFFSET_KEY).empty()) {
    return false;
  }

  return true;
}
}  // namespace gpio
}  // namespace plugin
}  // namespace ola
