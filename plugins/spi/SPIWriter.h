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
 * SPIWriter.h
 * This writes data to a SPI device.
 * Copyright (C) 2013 Simon Newton
 */

#ifndef PLUGINS_SPI_SPIWRITER_H_
#define PLUGINS_SPI_SPIWRITER_H_

#include <stdint.h>
#include <string>

namespace ola {
namespace plugin {
namespace spi {

using std::string;

/**
 * The SPI Writer, this writes data to a SPI device
 */
class SPIWriter {
  public:
    /**
     * SPIWriter Options
     */
    struct Options {
      uint32_t spi_speed;
      bool cs_enable_high;

      Options() : spi_speed(1000000), cs_enable_high(false) {}
    };

    SPIWriter(const string &spi_device, const Options &options);
    ~SPIWriter();

    string DevicePath() const { return m_device_path; }

    /**
     * Init the SPIWriter
     * @returns false if initialization failed.
     */
    bool Init();

    bool WriteSPIData(const uint8_t *data, unsigned int length);

  private:
    const string m_device_path;
    const uint32_t m_spi_speed;
    const bool m_cs_enable_high;
    int m_fd;

    static const uint8_t SPI_MODE;
    static const uint8_t SPI_BITS_PER_WORD;
};

}  // namespace spi
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_SPI_SPIWRITER_H_
