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
 * SpiWriter.h
 * This writes data to a SPI device.
 * Copyright (C) 2013 Simon Newton
 */

#ifndef PLUGINS_SPI_SPIWRITER_H_
#define PLUGINS_SPI_SPIWRITER_H_

#include <ola/ExportMap.h>
#include <ola/thread/Mutex.h>
#include <stdint.h>
#include <string>

namespace ola {
namespace plugin {
namespace spi {

/**
 * The interface for the SPI Writer
 */
class SpiWriterInterface {
 public:
  virtual ~SpiWriterInterface() {}

  virtual std::string DevicePath() const = 0;
  virtual bool Init() = 0;
  virtual bool WriteSpiData(const uint8_t *data, unsigned int length) = 0;
};

/**
 * The SPI Writer, this writes data to a SPI device
 */
class SpiWriter : public SpiWriterInterface {
 public:
  /**
   * SpiWriter Options
   */
  struct Options {
    uint32_t spi_speed;
    bool cs_enable_high;

    Options() : spi_speed(1000000), cs_enable_high(false) {}
  };

  SpiWriter(const std::string &spi_device, const Options &options,
            ExportMap *export_map);
  ~SpiWriter();

  std::string DevicePath() const { return m_device_path; }

  /**
   * Init the SpiWriter
   * @returns false if initialization failed.
   */
  bool Init();

  bool WriteSpiData(const uint8_t *data, unsigned int length);

 private:
  const std::string m_device_path;
  const uint32_t m_spi_speed;
  const bool m_cs_enable_high;
  int m_fd;
  UIntMap *m_error_map_var;
  UIntMap *m_write_map_var;

  static const uint8_t SPI_MODE;
  static const uint8_t SPI_BITS_PER_WORD;
  static const char SPI_DEVICE_KEY[];
  static const char SPI_ERROR_VAR[];
  static const char SPI_WRITE_VAR[];
};
}  // namespace spi
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_SPI_SPIWRITER_H_
