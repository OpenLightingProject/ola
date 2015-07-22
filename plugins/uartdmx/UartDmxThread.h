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

#ifndef PLUGINS_UARTDMX_UARTDMXTHREAD_H_
#define PLUGINS_UARTDMX_UARTDMXTHREAD_H_

#include "ola/DmxBuffer.h"
#include "ola/thread/Thread.h"

namespace ola {
namespace plugin {
namespace uartdmx {

class UartDmxThread : public ola::thread::Thread {
 public:
  UartDmxThread(UartWidget *widget, unsigned int breakt, unsigned int malft);
  ~UartDmxThread();

  bool Stop();
  void *Run();
  bool WriteDMX(const DmxBuffer &buffer);

 private:
  enum TimerGranularity { UNKNOWN, GOOD, BAD };

  TimerGranularity m_granularity;
  UartWidget *m_widget;
  bool m_term;
  unsigned int m_breakt;
  unsigned int m_malft;
  DmxBuffer m_buffer;
  ola::thread::Mutex m_term_mutex;
  ola::thread::Mutex m_buffer_mutex;

  void CheckTimeGranularity();

  static const uint32_t DMX_MAB = 16;

  DISALLOW_COPY_AND_ASSIGN(UartDmxThread);
};
}  // namespace uartdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_UARTDMX_UARTDMXTHREAD_H_
