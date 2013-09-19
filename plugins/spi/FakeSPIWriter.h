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
 * FakeSPIWriter.h
 * The SPIWriter used for testing.
 * Copyright (C) 2013 Simon Newton
 */

#ifndef PLUGINS_SPI_FAKESPIWRITER_H_
#define PLUGINS_SPI_FAKESPIWRITER_H_

#include <ola/thread/Mutex.h>
#include <stdint.h>
#include <string>
#include "plugins/spi/SPIWriter.h"

namespace ola {
namespace plugin {
namespace spi {

using std::string;

/**
 * A Fake SPI Writer used for testing
 */
class FakeSPIWriter : public SPIWriterInterface {
  public:
    explicit FakeSPIWriter(const string &device_path)
      : m_device_path(device_path),
        m_write_pending(0),
        m_writes(0),
        m_last_write_size(0),
        m_data(NULL) {
    }

    ~FakeSPIWriter() {
      delete[] m_data;
    }

    bool Init() { return true; }

    string DevicePath() const { return m_device_path; }

    bool WriteSPIData(const uint8_t *data, unsigned int length);

    // Methods used for testing
    void BlockWriter();
    void UnblockWriter();

    void ResetWrite();
    void WaitForWrite();

    unsigned int WriteCount() const;
    unsigned int LastWriteSize() const;
    void CheckDataMatches(unsigned int line, const uint8_t *data,
                          unsigned int length);

  private:
    const string m_device_path;
    bool m_write_pending;  // GUARDED_BY(m_mutex)
    unsigned int m_writes;  // GUARDED_BY(m_mutex)
    unsigned int m_last_write_size;  // GUARDED_BY(m_mutex)
    uint8_t *m_data;  // GUARDED_BY(m_mutex)

    ola::thread::Mutex m_write_lock;
    mutable ola::thread::Mutex m_mutex;
    ola::thread::ConditionVariable m_cond_var;
};

}  // namespace spi
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_SPI_FAKESPIWRITER_H_
