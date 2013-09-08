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
 * SPIBackend.h
 * The backend for SPI output. These are the classes which write the data to the
 * SPI bus.
 * Copyright (C) 2013 Simon Newton
 */

#ifndef PLUGINS_SPI_SPIBACKEND_H_
#define PLUGINS_SPI_SPIBACKEND_H_

#include <string>
#include <vector>

namespace ola {
namespace plugin {
namespace spi {

/**
 * The base class for all SPI Backends
 */
class SPIBackend {
  public:
    struct Options {
      uint32_t spi_speed;

      Options() : spi_speed(1000000) {}
    };

    SPIBackend(const string &spi_device, const Options &options);
    virtual ~SPIBackend();

    bool Init();
    string DevicePath() const {
      return m_device_path;
    }

    virtual bool Write(uint8_t output, const uint8_t *data,
                       unsigned int length) = 0;

  protected:
    bool WriteSPIData(const uint8_t *data, unsigned int length);

  private:
    const string m_device_path;
    uint32_t m_spi_speed;
    int m_fd;
};

/**
 * A SPIBackend which uses a hardware multiplexier.
 */
class MultiplexedSPIBackend : public SPIBackend {
  public:
    struct Options : public SPIBackend::Options {
      // which GPIO bits to use to select the output. The number of outputs
      // will be 2 ** gpio_pins.size();
      vector<uint8_t> gpio_pins;
    };

    MultiplexedSPIBackend(const string &spi_device, const Options &options);


    bool Write(uint8_t output, const uint8_t *data, unsigned int length)

  private:
    const uint8_t m_output_count;
}


/**
 * An SPI Backend which uses a software multipliexer. This accumulates all data
 * into a single buffer and then writes it to the SPI bus.
 */
class ChainedSPIBackend : public SPIBackend {
  public:
    struct Options : public SPIBackend::Options {
      uint8_t outputs;
    };

    ChainedSPIBackend(const string &spi_device, const Options &options);

    bool Write(uint8_t output, const uint8_t *data, unsigned int length)

  private:
    vector<unsigned int> m_output_sizes;
}
}  // namespace spi
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_SPI_SPIBACKEND_H_
