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
 * GPIOPlugin.cpp
 * The General Purpose digital I/O plugin.
 * Copyright (C) 2014 Simon Newton
 */

#include "plugins/gpio/GPIOPlugin.h"

#include <memory>
#include <string>
#include <vector>
#include "olad/Preferences.h"
#include "ola/Logging.h"
#include "ola/StringUtils.h"
#include "plugins/gpio/GPIODevice.h"
#include "plugins/gpio/GPIODriver.h"

namespace ola {
namespace plugin {
namespace gpio {

using std::auto_ptr;
using std::string;
using std::vector;

const char GPIOPlugin::GPIO_PINS_KEY[] = "gpio_pins";
const char GPIOPlugin::GPIO_SLOT_OFFSET_KEY[] = "gpio_slot_offset";
const char GPIOPlugin::GPIO_TURN_OFF_KEY[] = "gpio_turn_off";
const char GPIOPlugin::GPIO_TURN_ON_KEY[] = "gpio_turn_on";
const char GPIOPlugin::PLUGIN_NAME[] = "GPIO";
const char GPIOPlugin::PLUGIN_PREFIX[] = "gpio";

bool GPIOPlugin::StartHook() {
  GPIODriver::Options options;
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

  std::auto_ptr<GPIODevice> device(new GPIODevice(this, options));
  if (!device->Start()) {
    return false;
  }

  m_plugin_adaptor->RegisterDevice(device.get());
  m_device = device.release();
  return true;
}

bool GPIOPlugin::StopHook() {
  if (m_device) {
    m_plugin_adaptor->UnregisterDevice(m_device);
    m_device->Stop();
    delete m_device;
    m_device = NULL;
  }
  return true;
}

string GPIOPlugin::Description() const {
  return
"General Purpose I/O Plugin\n"
"----------------------------\n"
"\n"
"This plugin controls the General Purpose Digital I/O (GPIO) pins on devices\n"
"like a Raspberry Pi. It creates a single device, with a single output port.\n"
"The offset (start address) of the GPIO pins is configurable.\n"
"\n"
"--- Config file : ola-gpio.conf ---\n"
"\n"
"gpio_pins = [int]\n"
"The list of GPIO pins to control, each pin is mapped to a DMX512 slot.\n"
"\n"
"gpio_slot_offset = <int>\n"
"The DMX512 slot for the first pin. Slots are indexed from 1.\n"
"\n"
"gpio_turn_on = <int>\n"
"The DMX512 value above which a GPIO pin will be turned on.\n"
"\n"
"gpio_turn_off = <int>\n"
"The DMX512 value below which a GPIO pin will be turned off.\n"
"\n";
}

bool GPIOPlugin::SetDefaultPreferences() {
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
