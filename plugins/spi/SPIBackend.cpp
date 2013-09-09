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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * SPIBackend.cpp
 * The backend for SPI output. These are the classes which write the data to
 * the SPI bus.
 * Copyright (C) 2013 Simon Newton
 */

#include <errno.h>
#include <fcntl.h>
#include <linux/spi/spidev.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>

#include <sstream>
#include <string>
#include <vector>
#include "ola/Logging.h"
#include "ola/network/SocketCloser.h"
#include "plugins/spi/SPIBackend.h"


namespace ola {
namespace plugin {
namespace spi {

const uint8_t SPIBackend::SPI_BITS_PER_WORD = 8;
const uint8_t SPIBackend::SPI_MODE = 0;

SPIBackend::SPIBackend(const string &spi_device, const Options &options)
    : m_device_path(spi_device),
      m_spi_speed(options.spi_speed),
      m_fd(-1) {
}

SPIBackend::~SPIBackend() {
  if (m_fd >= 0)
    close(m_fd);
}

bool SPIBackend::Init() {
  bool ok = InitHook();
  if (!ok)
    return false;

  int fd = open(m_device_path.c_str(), O_RDWR);
  ola::network::SocketCloser closer(fd);
  if (fd < 0) {
    OLA_WARN << "Failed to open " << m_device_path << " : " << strerror(errno);
    return false;
  }

  uint8_t spi_mode = SPI_MODE;
  if (ioctl(fd, SPI_IOC_WR_MODE, &spi_mode) < 0) {
    OLA_WARN << "Failed to set SPI_IOC_WR_MODE for " << m_device_path;
    return false;
  }

  uint8_t spi_bits_per_word = SPI_BITS_PER_WORD;
  if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &spi_bits_per_word) < 0) {
    OLA_WARN << "Failed to set SPI_IOC_WR_BITS_PER_WORD for " << m_device_path;
    return false;
  }

  if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &m_spi_speed) < 0) {
    OLA_WARN << "Failed to set SPI_IOC_WR_MAX_SPEED_HZ for " << m_device_path;
    return false;
  }
  m_fd = closer.Release();
  return true;
}

bool SPIBackend::WriteSPIData(const uint8_t *data, unsigned int length) {
  struct spi_ioc_transfer spi;
  memset(&spi, 0, sizeof(spi));
  spi.tx_buf = reinterpret_cast<__u64>(data);
  spi.len = length;
  int bytes_written = ioctl(m_fd, SPI_IOC_MESSAGE(1), &spi);
  if (bytes_written != static_cast<int>(length)) {
    OLA_WARN << "Failed to write all the SPI data: " << strerror(errno);
    return false;
  }
  return true;
}

HardwareBackend::HardwareBackend(const string &spi_device,
                                       const Options &options)
    : SPIBackend(spi_device, options),
      m_output_count(1 << options.gpio_pins.size()),
      m_gpio_pins(options.gpio_pins) {
}


HardwareBackend::~HardwareBackend() {
  CloseGPIOFDs();
}

bool HardwareBackend::Write(uint8_t output, const uint8_t *data,
                            unsigned int length) {
  if (output >= m_output_count) {
    return false;
  }

  const string on("1");
  const string off("0");

  for (unsigned int i = 0; i < m_gpio_fds.size(); i++) {
    uint8_t pin = output & (1 << i);

    if (i >= m_gpio_pin_state.size()) {
      m_gpio_pin_state.push_back(!pin);
    }

    if (m_gpio_pin_state[i] != pin) {
      const string &data = pin ? on : off;
      if (write(m_gpio_fds[i], data.c_str(), data.size()) < 0) {
        OLA_WARN << "Failed to toggle SPI GPIO pin "
                 << static_cast<int>(m_gpio_pins[i]) << ": "
                 << strerror(errno);
        return false;
      }
      m_gpio_pin_state[i] = pin;
    }
  }

  return WriteSPIData(data, length);
}

bool HardwareBackend::InitHook() {
  /**
   * This relies on the pins being exported:
   *   echo N > /sys/class/gpio/export
   * That requires root access.
   */
  const string direction("out");
  bool failed = false;
  vector<uint8_t>::const_iterator iter = m_gpio_pins.begin();
  for (; iter != m_gpio_pins.end(); ++iter) {
    std::ostringstream str;
    str << "/sys/class/gpio/gpio" << static_cast<int>(*iter) << "/value";
    int fd = open(str.str().c_str(), O_RDWR);
    if (fd < 0) {
      OLA_WARN << "Failed to open " << str.str() << " : " << strerror(errno);
      failed = true;
      break;
    } else {
      m_gpio_fds.push_back(fd);
    }

    // Set dir
    str.str("");
    str << "/sys/class/gpio/gpio" << static_cast<int>(*iter) << "/direction";
    fd = open(str.str().c_str(), O_RDWR);
    if (fd < 0) {
      OLA_WARN << "Failed to open " << str.str() << " : " << strerror(errno);
      failed = true;
      break;
    }
    if (write(fd, direction.c_str(), direction.size()) < 0) {
      OLA_WARN << "Failed to enable output on " << str.str() << " : "
               << strerror(errno);
      failed = true;
    }
    close(fd);
  }

  if (failed) {
    CloseGPIOFDs();
    return false;
  }
  return true;
}

void HardwareBackend::CloseGPIOFDs() {
  GPIOFds::iterator iter = m_gpio_fds.begin();
  for (; iter != m_gpio_fds.end(); ++iter) {
    close(*iter);
  }
  m_gpio_fds.clear();
}

SoftwareBackend::SoftwareBackend(const string &spi_device,
                                 const Options &options)
    : SPIBackend(spi_device, options),
      m_sync_output(options.sync_output),
      m_output_sizes(options.outputs, 0),
      m_output(NULL),
      m_length(0) {
}

SoftwareBackend::~SoftwareBackend() {
  free(m_output);
}

bool SoftwareBackend::Write(uint8_t output, const uint8_t *data,
                            unsigned int length) {
  if (output >= m_output_sizes.size()) {
    OLA_WARN << "Invalid SPI output " << static_cast<int>(output);
    return false;
  }

  unsigned int leading = 0;
  unsigned int trailing = 0;
  for (uint8_t i = 0; i < m_output_sizes.size(); i++) {
    if (i < output) {
      leading += m_output_sizes[i];
    } else if (i > output) {
      trailing += m_output_sizes[i];
    }
  }
  const unsigned int required_size = leading + length + trailing;

  // Check if the current buffer is large enough to hold our data.
  if (required_size != m_length) {
    // The length changed
    uint8_t *new_output = reinterpret_cast<uint8_t*>(malloc(required_size));
    memcpy(new_output, m_output, leading);
    memcpy(new_output + leading, data, length);
    memcpy(new_output + leading + length, m_output + leading, trailing);
    free(m_output);
    m_output = new_output;
    m_length = required_size;
    m_output_sizes[output] = length;
  } else {
    // This is just an update
    memcpy(m_output + leading, data, length);
  }

  if (m_sync_output < 0 || output == m_sync_output) {
    return WriteSPIData(m_output, m_length);
  } else {
    return true;
  }
}
}  // namespace spi
}  // namespace plugin
}  // namespace ola
