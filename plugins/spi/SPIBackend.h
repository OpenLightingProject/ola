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
#include <ola/thread/Mutex.h>
#include <ola/thread/Thread.h>
#include <string>
#include <vector>

#include "plugins/spi/SPIWriter.h"

namespace ola {
namespace plugin {
namespace spi {

using std::string;
using std::vector;

/**
 * The interface for all SPI Backends.
 */
class SPIBackendInterface {
  public:
    virtual ~SPIBackendInterface() {}

    virtual uint8_t *Checkout(uint8_t output, unsigned int length) = 0;
    virtual void Commit(uint8_t output, unsigned int length,
                        unsigned int latch_bytes) = 0;

    virtual string DevicePath() const = 0;

    virtual bool Init() = 0;
};


/**
 * A HardwareBackend which uses GPIO pins and an external de-multiplexer
 */
class HardwareBackend : public ola::thread::Thread,
                        public SPIBackendInterface {
  public:
    struct Options : public SPIWriter::Options {
      // Which GPIO bits to use to select the output. The number of outputs
      // will be 2 ** gpio_pins.size();
      vector<uint8_t> gpio_pins;
    };

    HardwareBackend(const string &spi_device, const Options &options);

    ~HardwareBackend();

    bool Init();

    uint8_t *Checkout(uint8_t output, unsigned int length);

    void Commit(uint8_t output, unsigned int length, unsigned int latch_bytes);

    string DevicePath() const { return m_spi_writer.DevicePath(); }

  protected:
    void* Run();

  private:
    class OutputData {
      public:
        OutputData()
            : m_data(NULL),
              m_write_pending(false),
              m_size(0),
              m_actual_size(0),
              m_latch_bytes(0) {
        }

        ~OutputData() { delete m_data; }

        uint8_t *Resize(unsigned int length);
        void SetPending(unsigned int length, unsigned int latch_bytes);
        bool IsPending() const { return m_write_pending; }
        const uint8_t *GetData() const { return m_data; }
        unsigned int Size() const { return m_size; }

        OutputData& operator=(const OutputData &other);

      private:
        uint8_t *m_data;
        bool m_write_pending;
        unsigned int m_size;
        unsigned int m_actual_size;
        unsigned int m_latch_bytes;

        OutputData(const OutputData&);
    };

    typedef vector<int> GPIOFds;
    typedef vector<OutputData*> Outputs;

    SPIWriter m_spi_writer;
    const uint8_t m_output_count;
    ola::thread::Mutex m_mutex;
    ola::thread::ConditionVariable m_cond_var;
    bool m_exit;

    Outputs m_output_data;

    // GPIO members
    GPIOFds m_gpio_fds;
    const vector<uint8_t> m_gpio_pins;
    vector<bool> m_gpio_pin_state;

    void SetupOutputs(Outputs *outputs);
    void WriteOutput(uint8_t output_id, OutputData *output);
    bool SetupGPIO();
    void CloseGPIOFDs();
};


/**
 * An SPI Backend which uses a software multipliexer. This accumulates all data
 * into a single buffer and then writes it to the SPI bus.
class SoftwareBackend : public SPIBackendInterface {
  public:
    struct Options : public SPIWriter::Options {
       * The number of outputs.
      uint8_t outputs;
       * Controls if we designate one of the outputs as the 'sync' output.
       * If set >= 0, it denotes the output which triggers the SPI write.
       * If set to -1, we perform an SPI write on each update.
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
*/
}  // namespace spi
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_SPI_SPIBACKEND_H_
