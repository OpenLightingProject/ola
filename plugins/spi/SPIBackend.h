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

/**
 * The interface for all SPI Backends.
 */
class SPIBackendInterface {
 public:
  virtual ~SPIBackendInterface() {}

  virtual uint8_t *Checkout(uint8_t output, unsigned int length) = 0;
  virtual uint8_t *Checkout(uint8_t output,
                            unsigned int length,
                            unsigned int latch_bytes) = 0;
  virtual void Commit(uint8_t output) = 0;

  virtual std::string DevicePath() const = 0;

  virtual bool Init() = 0;

 protected:
  static const char SPI_DROP_VAR[];
  static const char SPI_DROP_VAR_KEY[];
};


/**
 * A HardwareBackend which uses GPIO pins and an external de-multiplexer
 */
class HardwareBackend : public ola::thread::Thread,
                        public SPIBackendInterface {
 public:
  struct Options {
    // Which GPIO bits to use to select the output. The number of outputs
    // will be 2 ** gpio_pins.size();
    std::vector<uint16_t> gpio_pins;
  };

  HardwareBackend(const Options &options,
                  SPIWriterInterface *writer,
                  ExportMap *export_map);
  ~HardwareBackend();

  bool Init();

  uint8_t *Checkout(uint8_t output, unsigned int length) {
    return Checkout(output, length, 0);
  }

  uint8_t *Checkout(uint8_t output,
                    unsigned int length,
                    unsigned int latch_bytes);
  void Commit(uint8_t output);

  std::string DevicePath() const { return m_spi_writer->DevicePath(); }

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

    ~OutputData() { delete[] m_data; }

    uint8_t *Resize(unsigned int length);
    void SetLatchBytes(unsigned int latch_bytes);
    void SetPending();
    bool IsPending() const { return m_write_pending; }
    void ResetPending() { m_write_pending = false; }
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

  typedef std::vector<int> GPIOFds;
  typedef std::vector<OutputData*> Outputs;

  SPIWriterInterface *m_spi_writer;
  UIntMap *m_drop_map;
  const uint8_t m_output_count;
  ola::thread::Mutex m_mutex;
  ola::thread::ConditionVariable m_cond_var;
  bool m_exit;

  Outputs m_output_data;

  // GPIO members
  GPIOFds m_gpio_fds;
  const std::vector<uint16_t> m_gpio_pins;
  std::vector<bool> m_gpio_pin_state;

  void SetupOutputs(Outputs *outputs);
  void WriteOutput(uint8_t output_id, OutputData *output);
  bool SetupGPIO();
  void CloseGPIOFDs();
};


/**
 * An SPI Backend which uses a software multipliexer. This accumulates all data
 * into a single buffer and then writes it to the SPI bus.
 */
class SoftwareBackend : public SPIBackendInterface,
                        public ola::thread::Thread {
 public:
  struct Options {
    /*
     * The number of outputs.
     */
    uint8_t outputs;
    /*
     * Controls if we designate one of the outputs as the 'sync' output.
     * If set >= 0, it denotes the output which triggers the SPI write.
     * If set to -1, we perform an SPI write on each update.
     */
    int16_t sync_output;

    Options() : outputs(1), sync_output(0) {}
  };

  SoftwareBackend(const Options &options,
                  SPIWriterInterface *writer,
                  ExportMap *export_map);
  ~SoftwareBackend();

  bool Init();

  uint8_t *Checkout(uint8_t output, unsigned int length) {
    return Checkout(output, length, 0);
  }

  uint8_t *Checkout(uint8_t output,
                    unsigned int length,
                    unsigned int latch_bytes);
  void Commit(uint8_t output);

  std::string DevicePath() const { return m_spi_writer->DevicePath(); }

 protected:
  void* Run();

 private:
  SPIWriterInterface *m_spi_writer;
  UIntMap *m_drop_map;
  ola::thread::Mutex m_mutex;
  ola::thread::ConditionVariable m_cond_var;
  bool m_write_pending;
  bool m_exit;

  const int16_t m_sync_output;
  std::vector<unsigned int> m_output_sizes;
  std::vector<unsigned int> m_latch_bytes;
  uint8_t *m_output;
  unsigned int m_length;
};


/**
 * A fake backend used for testing. If we had gmock this would be much
 * easier...
 */
class FakeSPIBackend : public SPIBackendInterface {
 public:
  explicit FakeSPIBackend(unsigned int outputs);
  ~FakeSPIBackend();

  uint8_t *Checkout(uint8_t output, unsigned int length) {
    return Checkout(output, length, 0);
  }

  uint8_t *Checkout(uint8_t output,
                    unsigned int length,
                    unsigned int latch_bytes);

  void Commit(uint8_t output);
  const uint8_t *GetData(uint8_t output, unsigned int *length);

  std::string DevicePath() const { return "/dev/test"; }

  bool Init() { return true; }

  unsigned int Writes(uint8_t output) const;

 private:
  class Output {
   public:
    Output() : data(NULL), length(0), writes(0) {}
    ~Output() { delete[] data; }

    uint8_t *data;
    unsigned int length;
    unsigned int writes;
  };

  typedef std::vector<Output*> Outputs;
  Outputs m_outputs;
};
}  // namespace spi
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_SPI_SPIBACKEND_H_
