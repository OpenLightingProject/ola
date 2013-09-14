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
 * The backend for SPI output. These are the classes which write the data to
 * the SPI bus.
 * Copyright (C) 2013 Simon Newton
 */

#ifndef PLUGINS_SPI_SPIBACKEND_H_
#define PLUGINS_SPI_SPIBACKEND_H_

#include <stdint.h>
#include <string>
#include <vector>

namespace ola {
namespace plugin {
namespace spi {

using std::string;
using std::vector;

/**
 * The base class for all SPI Backends.
 */
class SPIBackend {
  public:
    /**
     * SPIBackend Options
     */
    struct Options {
      uint32_t spi_speed;
      bool cs_enable_high;

      Options() : spi_speed(1000000), cs_enable_high(false) {}
    };

    SPIBackend(const string &spi_device, const Options &options);

    virtual ~SPIBackend();

    /**
     * Init the SPI backend
     * @returns false if initialization failed.
     */
    bool Init();

    string DevicePath() const { return m_device_path; }

    /**
     * Write data for a single output (device / universe) to the backend
     */
    virtual bool Write(uint8_t output, const uint8_t *data,
                       unsigned int length, unsigned int latch_bytes) = 0;

  protected:
    bool WriteSPIData(const uint8_t *data, unsigned int length);

    virtual bool InitHook() { return true; }

  private:
    const string m_device_path;
    const uint32_t m_spi_speed;
    const bool m_cs_enable_high;
    int m_fd;

    static const uint8_t SPI_MODE;
    static const uint8_t SPI_BITS_PER_WORD;
};

/**
 * A SPIBackend which uses a hardware multiplexier. This uses the GPIO pins to
 * control the multiplexer and route the SPI signal to the correct pixel
 * string.
 */
class HardwareBackend : public SPIBackend {
  public:
    struct Options : public SPIBackend::Options {
      // Which GPIO bits to use to select the output. The number of outputs
      // will be 2 ** gpio_pins.size();
      vector<uint8_t> gpio_pins;
    };

    HardwareBackend(const string &spi_device, const Options &options);
    ~HardwareBackend();

    bool Write(uint8_t output, const uint8_t *data, unsigned int length,
               unsigned int latch_bytes);

  protected:
    bool InitHook();

  private:
    typedef vector<int> GPIOFds;

    const uint8_t m_output_count;
    const vector<uint8_t> m_gpio_pins;
    GPIOFds m_gpio_fds;
    vector<bool> m_gpio_pin_state;

    void CloseGPIOFDs();
};


/**
 * An SPI Backend which uses a software multipliexer. This accumulates all data
 * into a single buffer and then writes it to the SPI bus.
 */
class SoftwareBackend : public SPIBackend {
  public:
    struct Options : public SPIBackend::Options {
      /**
       * The number of outputs.
       */
      uint8_t outputs;
      /**
       * Controls if we designate one of the outputs as the 'sync' output.
       * If set >= 0, it denotes the output which triggers the SPI write.
       * If set to -1, we perform an SPI write on each update.
       */
      int16_t sync_output;

      explicit Options() : outputs(1), sync_output(0) {}
    };

    SoftwareBackend(const string &spi_device, const Options &options);
    ~SoftwareBackend();

    bool Write(uint8_t output, const uint8_t *data, unsigned int length,
               unsigned int latch_bytes);

  private:
    const int16_t m_sync_output;
    vector<unsigned int> m_output_sizes;
    vector<unsigned int> m_latch_bytes;
    uint8_t *m_output;
    unsigned int m_length;
};
}  // namespace spi
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_SPI_SPIBACKEND_H_
