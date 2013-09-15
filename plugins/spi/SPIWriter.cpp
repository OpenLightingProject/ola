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
 * SPIWriter.cpp
 * This writes data to a SPI device.
 * Copyright (C) 2013 Simon Newton
 */

#include <errno.h>
#include <fcntl.h>
#include <linux/spi/spidev.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>

#include <numeric>
#include <sstream>
#include <string>
#include <vector>
#include "ola/Logging.h"
#include "ola/network/SocketCloser.h"
#include "plugins/spi/SPIWriter.h"


namespace ola {
namespace plugin {
namespace spi {

const uint8_t SPIWriter::SPI_BITS_PER_WORD = 8;
const uint8_t SPIWriter::SPI_MODE = 0;
const char SPIWriter::SPI_ERROR_VAR[] = "spi-write-errors";
const char SPIWriter::SPI_ERROR_VAR_KEY[] = "device";

SPIWriter::SPIWriter(const string &spi_device,
                     const Options &options,
                     ExportMap *export_map)
    : m_device_path(spi_device),
      m_spi_speed(options.spi_speed),
      m_cs_enable_high(options.cs_enable_high),
      m_fd(-1) {
  OLA_INFO << "Created SPI Writer " << spi_device << " with speed "
           << options.spi_speed << ", CE is " << m_cs_enable_high;
  if (export_map) {
    m_error_map_var = export_map->GetUIntMapVar(SPI_ERROR_VAR,
                                                SPI_ERROR_VAR_KEY);
    (*m_error_map_var)[m_device_path] = 0;
  }
}

SPIWriter::~SPIWriter() {
  if (m_fd >= 0)
    close(m_fd);
}

bool SPIWriter::Init() {
  int fd = open(m_device_path.c_str(), O_RDWR);
  ola::network::SocketCloser closer(fd);
  if (fd < 0) {
    OLA_WARN << "Failed to open " << m_device_path << " : " << strerror(errno);
    return false;
  }

  uint8_t spi_mode = SPI_MODE;
  if (m_cs_enable_high) {
    spi_mode |= SPI_CS_HIGH;
  }

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

bool SPIWriter::WriteSPIData(const uint8_t *data, unsigned int length) {
  struct spi_ioc_transfer spi;
  memset(&spi, 0, sizeof(spi));
  spi.tx_buf = reinterpret_cast<__u64>(data);
  spi.len = length;
  int bytes_written = ioctl(m_fd, SPI_IOC_MESSAGE(1), &spi);
  if (bytes_written != static_cast<int>(length)) {
    OLA_WARN << "Failed to write all the SPI data: " << strerror(errno);
    if (m_error_map_var) {
      (*m_error_map_var)[m_device_path]++;
    }

    return false;
  }
  return true;
}
}  // namespace spi
}  // namespace plugin
}  // namespace ola
