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
 * FakeSPIWriter.cpp
 * The SPIWriter used for testing.
 * Copyright (C) 2013 Simon Newton
 */

#include <string.h>
#include <numeric>
#include <string>
#include "ola/Logging.h"
#include "ola/testing/TestUtils.h"
#include "plugins/spi/FakeSPIWriter.h"

namespace ola {
namespace plugin {
namespace spi {

using ola::thread::MutexLocker;

bool FakeSPIWriter::WriteSPIData(const uint8_t *data, unsigned int length) {
  {
    MutexLocker lock(&m_mutex);

    if (m_last_write_size != length) {
      delete[] m_data;
      m_data = new uint8_t[length];
    }
    memcpy(m_data, data, length);

    m_writes++;
    m_write_pending = true;
    m_last_write_size = length;
  }
  m_cond_var.Signal();

  MutexLocker lock(&m_write_lock);
  return true;
}

void FakeSPIWriter::BlockWriter() {
  m_write_lock.Lock();
}

void FakeSPIWriter::UnblockWriter() {
  m_write_lock.Unlock();
}

void FakeSPIWriter::ResetWrite() {
  MutexLocker lock(&m_mutex);
  m_write_pending = false;
}

void FakeSPIWriter::WaitForWrite() {
  MutexLocker lock(&m_mutex);
  if (m_write_pending)
    return;
  m_cond_var.Wait(&m_mutex);
}

unsigned int FakeSPIWriter::WriteCount() const {
  MutexLocker lock(&m_mutex);
  return m_writes;
}

unsigned int FakeSPIWriter::LastWriteSize() const {
  MutexLocker lock(&m_mutex);
  return m_last_write_size;
}

void FakeSPIWriter::CheckDataMatches(
    const ola::testing::SourceLine &source_line,
    const uint8_t *expected,
    unsigned int length) {
  MutexLocker lock(&m_mutex);
  ola::testing::ASSERT_DATA_EQUALS(source_line, expected, length, m_data,
                                   m_last_write_size);
}
}  // namespace spi
}  // namespace plugin
}  // namespace ola
