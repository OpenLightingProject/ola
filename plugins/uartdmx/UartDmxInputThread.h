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
 * UartDmxThread.h
 * The DMX through a UART plugin for ola
 * Copyright (C) 2011 Rui Barreiros
 * Copyright (C) 2014 Richard Ash
 */

#ifndef PLUGINS_UARTDMX_UARTDMXINPUTTHREAD_H_
#define PLUGINS_UARTDMX_UARTDMXINPUTTHREAD_H_

#include "ola/DmxBuffer.h"
#include "ola/thread/Thread.h"
#include "plugins/uartdmx/UartWidget.h"

namespace ola {
namespace plugin {
namespace uartdmx {

class UartDmxInputPort; // let the compiler know such a class will be defined

class UartDmxInputThread : public ola::thread::Thread {
 public:
  UartDmxInputThread(UartWidget *widget, UartDmxInputPort *port);
  ~UartDmxInputThread();

  bool Stop();
  void *Run();

 private:
  UartWidget *m_widget;
  UartDmxInputPort *m_port;
  DmxBuffer m_buffer;

  enum DmxState { BREAK, MAB, STARTCODE, DATA, MTBF, MTBP };
  DmxState m_dmx_state;
  uint16_t m_dmx_count;
  uint8_t m_last_byte_received;
  void ResetDmxData();

  bool m_term;
  ola::thread::Mutex m_term_mutex;
  ola::thread::Mutex m_buffer_mutex;

  DISALLOW_COPY_AND_ASSIGN(UartDmxInputThread);
};
}  // namespace uartdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_UARTDMX_UARTDMXINPUTTHREAD_H_
