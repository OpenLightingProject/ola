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
 * GPIODriver.h
 * Uses data in a DMXBuffer to drive GPIO pins.
 * Copyright (C) 2014 Simon Newton
 */

#ifndef PLUGINS_GPIO_GPIODRIVER_H_
#define PLUGINS_GPIO_GPIODRIVER_H_

#include <stdint.h>
#include <ola/DmxBuffer.h>
#include <ola/base/Macro.h>

#include <vector>

namespace ola {
namespace plugin {
namespace gpio {

/**
 * @brief Uses data in a DMXBuffer to drive GPIO pins.
 */
class GPIODriver {
 public:
  struct Options {
   public:
    Options(): start_address(1), turn_on(128), turn_off(127) {}

    std::vector<uint8_t> gpio_pins;
    uint16_t start_address;
    uint8_t turn_on;
    uint8_t turn_off;
  };

  explicit GPIODriver(const Options &options);
  ~GPIODriver();

  bool Init();

  std::vector<uint8_t> PinList() const { return m_options.gpio_pins; }

  bool SendDmx(const DmxBuffer &dmx);

 private:
  enum GPIOState {
    ON,
    OFF,
    UNDEFINED,
  };

  struct GPIOPin {
    int fd;
    GPIOState state;
    bool last_value;
  };

  typedef std::vector<GPIOPin> GPIOPins;

  const Options m_options;
  GPIOPins m_gpio_pins;

  bool SetupGPIO();
  bool UpdateGPIOPins(const DmxBuffer &dmx);
  void CloseGPIOFDs();

  static const char GPIO_BASE_DIR[];

  DISALLOW_COPY_AND_ASSIGN(GPIODriver);
};
}  // namespace gpio
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_GPIO_GPIODRIVER_H_
