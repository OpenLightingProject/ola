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
 * SPIDMXThread.h
 * This thread runs while one or more ports are registered. It simultaneously
 * reads / writes SPI data and then calls the parser. This is repeated forever.
 * Copyright (C) 2017 Florian Edelmann
 */

#ifndef PLUGINS_SPIDMX_SPIDMXTHREAD_H_
#define PLUGINS_SPIDMX_SPIDMXTHREAD_H_

#include <memory>
#include <vector>
#include "ola/Callback.h"
#include "ola/DmxBuffer.h"
#include "ola/base/Macro.h"
#include "ola/thread/Thread.h"

namespace ola {
namespace plugin {
namespace spidmx {

class SPIDMXThread : public ola::thread::Thread {
 public:
  SPIDMXThread(SPIDMXWidget *widget, unsigned int blocklength);
  ~SPIDMXThread();

  void RegisterPort();
  void UnregisterPort();

  bool Stop();
  void *Run();

  bool WriteDMX(const DmxBuffer &buffer);
  const DmxBuffer &GetDmxInBuffer() const;

  bool SetReceiveCallback(Callback0<void> *callback);

 private:
  SPIDMXWidget *m_widget;
  unsigned int m_blocklength;

  bool m_term;
  int m_registered_ports;

  /** receive DMX buffer to give to InputPort's callback */
  DmxBuffer m_dmx_rx_buffer;
  /** transmit DMX buffer that is set from WriteDMX */
  DmxBuffer m_dmx_tx_buffer;

  /** receive buffer with raw SPI bytes */
  std::vector<uint8_t> m_spi_rx_buffer;
  /** transmit buffer with raw SPI bytes */
  std::vector<uint8_t> m_spi_tx_buffer;

  /** called when a new m_dmx_rx_buffer is ready */
  std::unique_ptr<Callback0<void> > m_receive_callback;

  ola::thread::Mutex m_term_mutex;
  ola::thread::Mutex m_buffer_mutex;

  DISALLOW_COPY_AND_ASSIGN(SPIDMXThread);
};

}  // namespace spidmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_SPIDMX_SPIDMXTHREAD_H_
