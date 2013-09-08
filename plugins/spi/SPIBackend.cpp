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
#include <string.h>
#include <sys/ioctl.h>

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

MultiplexedSPIBackend::MultiplexedSPIBackend(const string &spi_device,
                                             const Options &options)
    : SPIBackend(spi_device, options),
      m_output_count(1 << options.gpio_pins.size()),
      m_gpio_pins(options.gpio_pins) {
}


MultiplexedSPIBackend::~MultiplexedSPIBackend() {
  GPIOFds::iterator iter = m_gpio_fds.begin();
  for (; iter != m_gpio_fds.end(); ++iter) {
    close(*iter);
  }
}

bool MultiplexedSPIBackend::Write(uint8_t output, const uint8_t *data,
                                  unsigned int length) {
  if (output < m_output_count) {
    return false;
  }

  // TODO(simon): Select GPIO output here
  for (unsigned int i = 0; i < m_gpio_fds.size(); i++) {
    uint8_t on = output & (1 << i);
    OLA_INFO << "Pin " << i << " is " << (int) on;
    // Write m_gpio_fds[i] = on;
  }

  return WriteSPIData(data, length);
}

bool MultiplexedSPIBackend::InitHook() {
  // open each pin.
  vector<uint8_t>::const_iterator iter = m_gpio_pins.begin();
  for (; iter != m_gpio_pins.end(); ++iter) {
    // open GPIO fd here
  }
  return true;
}

ChainedSPIBackend::ChainedSPIBackend(const string &spi_device,
                                     const Options &options)
    : SPIBackend(spi_device, options),
      m_output_sizes(options.outputs, 0) {
}

bool ChainedSPIBackend::Write(uint8_t output, const uint8_t *data,
                              unsigned int length) {
  if (output >= m_output_sizes.size()) {
    return false;
  }

  if (length != m_output_sizes[output]) {
    // need to resize here
  }

  // copy to buffer

  return WriteSPIData(data, length);
}
}  // namespace spi
}  // namespace plugin
}  // namespace ola
