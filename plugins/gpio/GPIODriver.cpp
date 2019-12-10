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
 * GPIODriver.cpp
 * Uses data in a DMXBuffer to drive GPIO pins.
 * Copyright (C) 2014 Simon Newton
 */

#include "plugins/gpio/GPIODriver.h"

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <sstream>
#include <string>
#include <vector>

#include "ola/io/IOUtils.h"
#include "ola/Logging.h"
#include "ola/thread/Mutex.h"

namespace ola {
namespace plugin {
namespace gpio {

const char GPIODriver::GPIO_BASE_DIR[] = "/sys/class/gpio/gpio";

using ola::thread::MutexLocker;
using std::string;
using std::vector;

GPIODriver::GPIODriver(const Options &options)
    : m_options(options),
      m_term(false),
      m_dmx_changed(false) {
}

GPIODriver::~GPIODriver() {
  {
    MutexLocker locker(&m_mutex);
    m_term = true;
  }
  m_cond.Signal();
  Join();

  CloseGPIOFDs();
}

bool GPIODriver::Init() {
  if (!SetupGPIO()) {
    return false;
  }

  return Start();
}

bool GPIODriver::SendDmx(const DmxBuffer &dmx) {
  {
    MutexLocker locker(&m_mutex);
    // avoid the reference counting
    m_buffer.Set(dmx);
    m_dmx_changed = true;
  }
  m_cond.Signal();
  return true;
}

void *GPIODriver::Run() {
  Clock clock;
  DmxBuffer output;

  while (true) {
    bool update_pins = false;

    TimeStamp wake_up;
    clock.CurrentTime(&wake_up);
    wake_up += TimeInterval(1, 0);

    // Wait for one of: i) termination ii) DMX changed iii) timeout
    m_mutex.Lock();
    if (!m_term && !m_dmx_changed) {
      m_cond.TimedWait(&m_mutex, wake_up);
    }

    if (m_term) {
      m_mutex.Unlock();
      break;
    } else if (m_dmx_changed) {
      output.Set(m_buffer);
      m_dmx_changed = false;
      update_pins = true;
    }
    m_mutex.Unlock();
    if (update_pins) {
      UpdateGPIOPins(output);
    }
  }
  return NULL;
}

bool GPIODriver::SetupGPIO() {
  /**
   * This relies on the pins being exported:
   *   echo N > /sys/class/gpio/export
   * That requires root access.
   */
  const string direction("out");
  bool failed = false;
  vector<uint16_t>::const_iterator iter = m_options.gpio_pins.begin();
  for (; iter != m_options.gpio_pins.end(); ++iter) {
    std::ostringstream str;
    str << GPIO_BASE_DIR << static_cast<int>(*iter) << "/value";
    int pin_fd;
    if (!ola::io::Open(str.str(), O_RDWR, &pin_fd)) {
      failed = true;
      break;
    }

    GPIOPin pin = {pin_fd, UNDEFINED, false};

    // Set dir
    str.str("");
    str << GPIO_BASE_DIR << static_cast<int>(*iter) << "/direction";
    int fd;
    if (!ola::io::Open(str.str(), O_RDWR, &fd)) {
      failed = true;
      break;
    }
    if (write(fd, direction.c_str(), direction.size()) < 0) {
      OLA_WARN << "Failed to enable output on " << str.str() << " : "
               << strerror(errno);
      failed = true;
    }
    close(fd);

    m_gpio_pins.push_back(pin);
  }

  if (failed) {
    CloseGPIOFDs();
    return false;
  }
  return true;
}

bool GPIODriver::UpdateGPIOPins(const DmxBuffer &dmx) {
  enum Action {
    TURN_ON,
    TURN_OFF,
    NO_CHANGE,
  };
  const uint16_t first_slot = m_options.start_address - 1;

  for (uint16_t i = 0;
       i < m_gpio_pins.size() && (i + first_slot < dmx.Size());
       i++) {
    Action action = NO_CHANGE;
    uint8_t slot_value = dmx.Get(i + first_slot);

    switch (m_gpio_pins[i].state) {
      case ON:
        action = (slot_value <= m_options.turn_off ? TURN_OFF : NO_CHANGE);
        break;
      case OFF:
        action = (slot_value >= m_options.turn_on ? TURN_ON : NO_CHANGE);
        break;
      case UNDEFINED:
      default:
        // If the state if undefined and the value is in the mid-range, then
        // default to turning off.
        action = (slot_value >= m_options.turn_on ? TURN_ON : TURN_OFF);
    }

    // Change the pin state if required.
    if (action != NO_CHANGE) {
      char data = (action == TURN_ON ? '1' : '0');
      if (write(m_gpio_pins[i].fd, &data, sizeof(data)) < 0) {
        OLA_WARN << "Failed to toggle GPIO pin " << i << ", fd "
                 << static_cast<int>(m_gpio_pins[i].fd) << ": "
                 << strerror(errno);
        return false;
      }
      m_gpio_pins[i].state = (action == TURN_ON ? ON : OFF);
    }
  }
  return true;
}

void GPIODriver::CloseGPIOFDs() {
  GPIOPins::iterator iter = m_gpio_pins.begin();
  for (; iter != m_gpio_pins.end(); ++iter) {
    close(iter->fd);
  }
  m_gpio_pins.clear();
}
}  // namespace gpio
}  // namespace plugin
}  // namespace ola
