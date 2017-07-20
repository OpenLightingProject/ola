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
 * SpiDmxThread.cpp
 * The DMX through a SPI plugin for ola
 * Copyright (C) 2011 Rui Barreiros
 * Copyright (C) 2014 Richard Ash
 */

#include <string>
#include "ola/Logging.h"
#include "plugins/spidmx/SpiDmxWidget.h"
#include "plugins/spidmx/SpiDmxThread.h"
#include "plugins/spidmx/SpiDmxParser.h"

namespace ola {
namespace plugin {
namespace spidmx {

SpiDmxThread::SpiDmxThread(SpiDmxWidget *widget, unsigned int blocklength)
  : m_widget(widget),
    m_blocklength(blocklength),
    m_term(false),
    m_registered_ports(0) {
  OLA_DEBUG << "SpiDmxThread constructor called";

  m_spi_rx_buffer = new uint8_t[m_blocklength];
  m_spi_tx_buffer = new uint8_t[m_blocklength];
}

SpiDmxThread::~SpiDmxThread() {
  Stop();
  delete[] m_spi_rx_buffer;
  delete[] m_spi_tx_buffer;
}

/**
 * This thread does only have to run if ports using it are patched to a
 * universe. Thus, they must register and unregister to notify this thread.
 */
void SpiDmxThread::RegisterPort() {
  m_registered_ports++;

  if (m_registered_ports == 1) {
    Start();
  }
}
void SpiDmxThread::UnregisterPort() {
  m_registered_ports--;

  if (m_registered_ports == 0) {
    Stop();
  }
}


/**
 * Stop this thread
 */
bool SpiDmxThread::Stop() {
  {
    ola::thread::MutexLocker locker(&m_term_mutex);
    m_term = true;
  }
  return Join();
}


/**
 * Copy a DMXBuffer to the output thread
 */
bool SpiDmxThread::WriteDMX(const DmxBuffer &buffer) {
  ola::thread::MutexLocker locker(&m_buffer_mutex);
  m_dmx_tx_buffer.Set(buffer);
  return true;
}

/**
  * @brief Get DMX Buffer
  * @returns DmxBuffer with current input values.
  */
const DmxBuffer &SpiDmxThread::GetDmxInBuffer() const {
  return m_dmx_rx_buffer;
}


/**
  * @brief Set the callback to be called when the receive buffer is updated.
  * @param callback The callback to call.
  */
bool SpiDmxThread::SetReceiveCallback(Callback0<void> *callback) {
  OLA_INFO << "SpiDmxThread::SetReceiveCallback called";
  m_receive_callback.reset(callback);

  if (!callback) {
    // InputPort unregistered
    UnregisterPort();
    return true;
  }

  RegisterPort();
  return m_widget->SetupOutput();
}


/**
 * The method called by the thread
 */
void *SpiDmxThread::Run() {
  OLA_INFO << "SpiDmxThread::Run called";
  DmxBuffer buffer;

  // Setup the widget
  if (!m_widget->IsOpen()) {
    if (!m_widget->SetupOutput()) {
      OLA_INFO << "SpiDmxThread::Run stopped";
      return NULL;
    }
  }

  // Setup the parser
  SpiDmxParser *parser = new SpiDmxParser(&m_dmx_rx_buffer,
                                          m_receive_callback.get());

  while (1) {
    {
      ola::thread::MutexLocker locker(&m_term_mutex);
      if (m_term) {
        break;
      }
    }

    {
      ola::thread::MutexLocker locker(&m_buffer_mutex);
      buffer.Set(m_dmx_tx_buffer);
    }

    if (!m_widget->ReadWrite(NULL, m_spi_rx_buffer, m_blocklength)) {
      OLA_WARN << "SPIDMX Read / Write failed, stopping thread...";
      break;
    }

    parser->ParseDmx(m_spi_rx_buffer, (uint64_t) m_blocklength);
  }
  return NULL;
}

}  // namespace spidmx
}  // namespace plugin
}  // namespace ola
