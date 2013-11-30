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

#include <numeric>
#include <sstream>
#include <string>
#include <vector>

#include "ola/Logging.h"
#include "ola/io/IOUtils.h"
#include "ola/network/SocketCloser.h"
#include "ola/stl/STLUtils.h"
#include "plugins/spi/SPIBackend.h"

namespace ola {
namespace plugin {
namespace spi {

using ola::thread::MutexLocker;

const char SPIBackendInterface::SPI_DROP_VAR[] = "spi-drops";
const char SPIBackendInterface::SPI_DROP_VAR_KEY[] = "device";

uint8_t *HardwareBackend::OutputData::Resize(unsigned int length) {
  if (length < m_size) {
    m_size = length;
    return m_data;
  } else if (length == m_size) {
    return m_data;
  }

  if (length <= m_actual_size) {
    m_size = length;
    return m_data;
  }

  delete[] m_data;
  m_data = new uint8_t[length];
  if (m_data) {
    m_size = m_data ? length : 0;
    m_actual_size = m_size;
    memset(m_data, 0, length);
  }
  return m_data;
}

void HardwareBackend::OutputData::SetPending() {
  m_write_pending = true;
}

void HardwareBackend::OutputData::SetLatchBytes(unsigned int latch_bytes) {
  m_latch_bytes = latch_bytes;
}

HardwareBackend::OutputData& HardwareBackend::OutputData::operator=(
    const HardwareBackend::OutputData &other) {
  if (this != &other) {
    uint8_t *data = Resize(other.m_size + other.m_latch_bytes);
    if (data) {
      memcpy(data, other.m_data, other.m_size);
      memset(data + other.m_size, 0, other.m_latch_bytes);
      m_write_pending = true;
    } else {
      m_write_pending = false;
    }
  }
  return *this;
}

HardwareBackend::HardwareBackend(const Options &options,
                                 SPIWriterInterface *writer,
                                 ExportMap *export_map)
    : m_spi_writer(writer),
      m_output_count(1 << options.gpio_pins.size()),
      m_exit(false),
      m_gpio_pins(options.gpio_pins) {
  SetupOutputs(&m_output_data);
  if (export_map) {
    m_drop_map = export_map->GetUIntMapVar(SPI_DROP_VAR,
                                           SPI_DROP_VAR_KEY);
    (*m_drop_map)[m_spi_writer->DevicePath()] = 0;
  }
}


HardwareBackend::~HardwareBackend() {
  {
    MutexLocker lock(&m_mutex);
    m_exit = true;
  }

  m_cond_var.Signal();
  Join();

  STLDeleteElements(&m_output_data);
  CloseGPIOFDs();
}

bool HardwareBackend::Init() {
  if (!(m_spi_writer->Init() && SetupGPIO())) {
    return false;
  }

  if (!Start()) {
    CloseGPIOFDs();
    return false;
  }
  return true;
}

uint8_t *HardwareBackend::Checkout(uint8_t output_id,
                                   unsigned int length,
                                   unsigned int latch_bytes) {
  if (output_id >= m_output_count) {
    return NULL;
  }

  m_mutex.Lock();
  uint8_t *output = m_output_data[output_id]->Resize(length);
  if (!output) {
    m_mutex.Unlock();
  }
  m_output_data[output_id]->SetLatchBytes(latch_bytes);
  return output;
}

void HardwareBackend::Commit(uint8_t output) {
  if (output >= m_output_count) {
    return;
  }

  OutputData *output_data = m_output_data[output];
  if (output_data->IsPending() && m_drop_map) {
    // There was already another write pending which we're now stomping on
    (*m_drop_map)[m_spi_writer->DevicePath()]++;
  }
  output_data->SetPending();
  m_mutex.Unlock();
  m_cond_var.Signal();
}

void *HardwareBackend::Run() {
  Outputs outputs;
  SetupOutputs(&outputs);

  while (true) {
    m_mutex.Lock();

    if (m_exit) {
      m_mutex.Unlock();
      STLDeleteElements(&outputs);
      return NULL;
    }

    bool action_pending = false;
    Outputs::const_iterator iter = m_output_data.begin();
    for (; iter != m_output_data.end(); ++iter) {
      if ((*iter)->IsPending()) {
        action_pending = true;
        break;
      }
    }
    if (!action_pending) {
      m_cond_var.Wait(&m_mutex);
    }

    if (m_exit) {
      m_mutex.Unlock();
      STLDeleteElements(&outputs);
      return NULL;
    }

    for (unsigned int i = 0; i < m_output_data.size(); i++) {
      if (m_output_data[i]->IsPending()) {
        // take a copy
        *outputs[i] = *m_output_data[i];
        m_output_data[i]->ResetPending();
      }
    }
    m_mutex.Unlock();

    for (unsigned int i = 0; i < outputs.size(); i++) {
      if (outputs[i]->IsPending()) {
        WriteOutput(i, outputs[i]);
        outputs[i]->ResetPending();
      }
    }
  }
}

void HardwareBackend::SetupOutputs(Outputs *outputs) {
  for (unsigned int i = 0; i < m_output_count; i++) {
    outputs->push_back(new OutputData());
  }
}

void HardwareBackend::WriteOutput(uint8_t output_id, OutputData *output) {
  const string on("1");
  const string off("0");

  for (unsigned int i = 0; i < m_gpio_fds.size(); i++) {
    uint8_t pin = output_id & (1 << i);

    if (i >= m_gpio_pin_state.size()) {
      m_gpio_pin_state.push_back(!pin);
    }

    if (m_gpio_pin_state[i] != pin) {
      const string &data = pin ? on : off;
      if (write(m_gpio_fds[i], data.c_str(), data.size()) < 0) {
        OLA_WARN << "Failed to toggle SPI GPIO pin "
                 << static_cast<int>(m_gpio_pins[i]) << ": "
                 << strerror(errno);
        return;
      }
      m_gpio_pin_state[i] = pin;
    }
  }

  m_spi_writer->WriteSPIData(output->GetData(), output->Size());
}

bool HardwareBackend::SetupGPIO() {
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
    int fd;
    if (ola::io::Open(str.str(), O_RDWR, &fd)) {
      m_gpio_fds.push_back(fd);
    } else {
      failed = true;
      break;
    }

    // Set dir
    str.str("");
    str << "/sys/class/gpio/gpio" << static_cast<int>(*iter) << "/direction";
    if (!ola::io::Open(str.str(), O_RDWR, &fd)) {
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

SoftwareBackend::SoftwareBackend(const Options &options,
                                 SPIWriterInterface *writer,
                                 ExportMap *export_map)
    : m_spi_writer(writer),
      m_write_pending(false),
      m_exit(false),
      m_sync_output(options.sync_output),
      m_output_sizes(options.outputs, 0),
      m_latch_bytes(options.outputs, 0),
      m_output(NULL),
      m_length(0),
      m_buffer_size(0) {
  if (export_map) {
    m_drop_map = export_map->GetUIntMapVar(SPI_DROP_VAR,
                                           SPI_DROP_VAR_KEY);
    (*m_drop_map)[m_spi_writer->DevicePath()] = 0;
  }
}

SoftwareBackend::~SoftwareBackend() {
  {
    MutexLocker lock(&m_mutex);
    m_exit = true;
  }

  m_cond_var.Signal();
  Join();

  delete[] m_output;
}

bool SoftwareBackend::Init() {
  if (!m_spi_writer->Init()) {
    return false;
  }

  if (!Start()) {
    return false;
  }
  return true;
}

uint8_t *SoftwareBackend::Checkout(uint8_t output,
                                   unsigned int length,
                                   unsigned int latch_bytes) {
  if (output >= m_output_sizes.size()) {
    OLA_WARN << "Invalid SPI output " << static_cast<int>(output);
    return NULL;
  }

  m_mutex.Lock();

  unsigned int leading = 0;
  unsigned int trailing = 0;
  for (uint8_t i = 0; i < m_output_sizes.size(); i++) {
    if (i < output) {
      leading += m_output_sizes[i];
    } else if (i > output) {
      trailing += m_output_sizes[i];
    }
  }

  m_latch_bytes[output] = latch_bytes;
  const unsigned int total_latch_bytes = std::accumulate(
      m_latch_bytes.begin(), m_latch_bytes.end(), 0);
  const unsigned int required_size = (
      leading + length + trailing + total_latch_bytes);

  // Check if the current buffer is large enough to hold our data.
  if (required_size != m_length) {
    // The length changed
    uint8_t *new_output = new uint8_t[required_size];
    memcpy(new_output, m_output, leading);
    memset(new_output + leading, 0, length);
    memcpy(new_output + leading + length, m_output + leading, trailing);
    memset(new_output + leading + length + trailing, 0, total_latch_bytes);
    delete[] m_output;
    m_output = new_output;
    m_length = required_size;
    m_output_sizes[output] = length;
  }
  return m_output + leading;
}

void SoftwareBackend::Commit(uint8_t output) {
  if (output >= m_output_sizes.size()) {
    OLA_WARN << "Invalid SPI output " << static_cast<int>(output);
    return;
  }

  bool should_write = m_sync_output < 0 || output == m_sync_output;
  if (should_write) {
    if (m_write_pending && m_drop_map) {
      // There was already another write pending which we're now stomping on
      (*m_drop_map)[m_spi_writer->DevicePath()]++;
    }
    m_write_pending = should_write;
  }
  m_mutex.Unlock();
  if (should_write) {
    m_cond_var.Signal();
  }
}

void *SoftwareBackend::Run() {
  uint8_t *output_data = NULL;
  unsigned int length = 0;

  while (true) {
    m_mutex.Lock();

    if (m_exit) {
      m_mutex.Unlock();
      delete output_data;
      return NULL;
    }

    if (!m_write_pending) {
      m_cond_var.Wait(&m_mutex);
    }

    if (m_exit) {
      m_mutex.Unlock();
      delete[] output_data;
      return NULL;
    }

    bool write_pending = m_write_pending;
    m_write_pending = false;
    if (write_pending) {
      if (length < m_length) {
        delete[] output_data;
        output_data = new uint8_t[m_length];
        length = m_length;
      }
      // TODO(simon) Add an actual size here
      length = m_length;
      memcpy(output_data, m_output, m_length);
    }
    m_mutex.Unlock();

    if (write_pending) {
      m_spi_writer->WriteSPIData(output_data, length);
    }
  }
}

FakeSPIBackend::FakeSPIBackend(unsigned int outputs) {
  for (unsigned int i = 0; i < outputs; i++) {
    m_outputs.push_back(new Output());
  }
}

FakeSPIBackend::~FakeSPIBackend() {
  STLDeleteElements(&m_outputs);
}

uint8_t *FakeSPIBackend::Checkout(uint8_t output_id,
                                  unsigned int length,
                                  unsigned int latch_bytes) {
  if (output_id >= m_outputs.size()) {
    return NULL;
  }

  Output *output = m_outputs[output_id];

  if (output->length != length + latch_bytes) {
    delete[] output->data;
    output->data = new uint8_t[length + latch_bytes];
    memset(output->data, 0, length + latch_bytes);
    output->length = length + latch_bytes;
  }
  return output->data;
}

void FakeSPIBackend::Commit(uint8_t output) {
  if (output >= m_outputs.size()) {
    return;
  }
  m_outputs[output]->writes++;
}

const uint8_t *FakeSPIBackend::GetData(uint8_t output_id,
                                       unsigned int *length) {
  if (output_id >= m_outputs.size()) {
    return NULL;
  }
  Output *output = m_outputs[output_id];
  *length = output->length;
  return output->data;
}


unsigned int FakeSPIBackend::Writes(uint8_t output) const {
  if (output >= m_outputs.size()) {
    return 0;
  }
  return m_outputs[output]->writes;
}
}  // namespace spi
}  // namespace plugin
}  // namespace ola
