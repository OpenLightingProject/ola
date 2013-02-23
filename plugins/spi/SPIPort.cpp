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
 * SPIPort.h
 * The SPI plugin for ola
 * Copyright (C) 2013 Simon Newton
 */

#include <linux/spi/spidev.h>
#include <sstream>
#include <string>
#include "ola/Logging.h"
#include "ola/BaseTypes.h"
#include "ola/network/SocketCloser.h"

#include "plugins/spi/SPIPort.h"

namespace ola {
namespace plugin {
namespace spi {

SPIOutputPort::SPIOutputPort(SPIDevice *parent, const string &spi_device)
    : BasicOutputPort(parent, 0),
      m_device_path(spi_device),
      m_spi_device_name(spi_device),
      m_pixel_count(25),
      m_fd(-1),
      m_output_data(NULL);
      m_spi_mode(0),
      m_spi_bits_per_word(8),
      m_spi_delay(0),
      m_spi_speed(1000000) {
  size_t pos = spi_device.find_last_of("/");
  if (pos != string::npos)
    m_spi_device_name = spi_device.substr(pos + 1);
}


SPIOutputPort::~SPIOutputPort() {
  if (m_fd >= 0)
    close(m_fd);
  if (m_output_data)
    delete[] m_output_data;
}


/**
 * Open the SPI device
 */
bool SPIOutputPort::Init() {
  int fd = open(m_device_path.c_str(), O_RDWR);
  ola::network::SocketCloser closer(fd);
  if (fd < 0) {
    OLA_ERROR << "Failed to open " << m_device_path << " : " << strerror(errno);
    return false;
  }

  if (ioctl(fd, SPI_IOC_WR_MODE, &m_spi_mode) < 0) {
    return false;
  }

  if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &m_spi_bits_per_word) < 0) {
    return false;
  }

  if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &m_spi_speed) < 0) {
    return false;
  }
  m_fd = closer.Release();
  return true;
}


/*
 * Send DMX data over SPI.
 */
bool SPIOutputPort::WriteDMX(const DmxBuffer &buffer, uint8_t) {
  if (m_fd < 0)
    return false;

  if (!m_output_data)
    m_output_data = new uint8_t[m_pixel_count * 3];

  buffer.Get(m_output_data, m_pixel_count * 3);

  struct spi_ioc_transfer spi;
  spi.tx_buf = m_output_data;
  spi.len = m_pixel_count * 3;
  int bytes_written = ioctl(fd, SPI_IOC_MESSAGE(1), &spi);
  if (bytes_written != m_pixel_count * 3) {
    OLA_WARN << "Failed to write all the SPI data";
    return false;
  }
  return true;
}
}  // spi
}  // plugin
}  // ola
