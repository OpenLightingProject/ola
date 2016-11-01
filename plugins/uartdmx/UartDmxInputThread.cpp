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
 * UartDmxThread.cpp
 * The DMX through a UART plugin for ola
 * Copyright (C) 2011 Rui Barreiros
 * Copyright (C) 2014 Richard Ash
 */

#include <math.h>
#include <unistd.h>
#include <string>
#include <stdio.h>

#include "ola/Constants.h"
#include "ola/Clock.h"
#include "ola/Logging.h"
#include "ola/StringUtils.h"

#include "plugins/uartdmx/UartDmxInputThread.h"
#include "plugins/uartdmx/UartDmxPort.h"
#include "plugins/uartdmx/UartWidget.h"

namespace ola {
namespace plugin {
namespace uartdmx {

UartDmxInputThread::UartDmxInputThread(UartWidget *widget, UartDmxInputPort *port)
  : m_widget(widget),
    m_port(port),
    m_term(false) {
}

UartDmxInputThread::~UartDmxInputThread() {
  printf("UartDmxInputThread destructor called\n");
  Stop();
}


/**
 * Stop this thread
 */
bool UartDmxInputThread::Stop() {
  printf("UartDmxInputThread::Stop called\n");
  {
    ola::thread::MutexLocker locker(&m_term_mutex);
    m_term = true;
  }
  return Join();
}

void UartDmxInputThread::ResetDmxData() {
  m_dmx_state = BREAK;
  m_dmx_count = 0;
  m_last_byte_received = 255;
}


/**
 * The method called by the thread
 */
void *UartDmxInputThread::Run() {
  // Setup the widget
  if (!m_widget->IsOpen())
    m_widget->SetupOutput();

  ResetDmxData();

  while (1) {
    {
      ola::thread::MutexLocker locker(&m_term_mutex);
      if (m_term) {
        printf("UartDmxInputThread's loop noticed\n");
        break;
      }
    }

    int current_bytes[3];
    current_bytes[0] = 0;
    current_bytes[1] = -1;
    current_bytes[2] = -1;


    /*printf("Run select\n");
    int resultcode = m_widget->Select();
    if (resultcode <= 0) {
      // resultcode = -2: BREAK
      // resultcode = -1: error
      // resultcode = 0: timed out (1 sec)

      if (resultcode == -2) {
        m_port->NewDMXData(m_buffer);
      }
      ResetDmxData();
      continue;
    }*/
    printf("Run read\n");
    int n = m_widget->Read(current_bytes, 3);
    if (n == 0) {
      // no data available -> try again
      continue;
    }

    printf("Received %d bytes: %d %d %d, break occured? %d\n", n, current_bytes[0], current_bytes[1], current_bytes[2], m_widget->m_break_detected);

    /*if (m_dmx_state == BREAK) {
      if (current_byte != 0) {
        printf("wait for next BREAK\n");
        // wait for the next break
        continue;
      }
      m_dmx_state = STARTCODE;
      printf("It was the BREAK\n");
    }
    else if (m_dmx_state == STARTCODE) {
      if (current_byte != 0) {
        printf("Startcode didn't follow\n");
        ResetDmxData();
        continue;
      }
      printf("It was the start code\n");
      m_dmx_state = DATA;
    }
    else if (m_dmx_state == DATA) {
      printf("It's channel %d\n", m_dmx_count+1);
      m_buffer.SetChannel(m_dmx_count, current_byte);
      m_dmx_count++;

      if (m_dmx_count >= DMX_UNIVERSE_SIZE) {
        printf("Reset\n");
        m_port->NewDMXData(m_buffer);
        ResetDmxData();
      }
      else if (current_byte == 0 && m_last_byte_received == 0) {

      }
    }*/

    /*if (m_dmx_state == BREAK && m_dmx_count == 0) {
      current_byte = m_last_byte_received;
    }
    else {
      int resultcode = m_widget->Select();
      if (resultcode <= 0) {
        // resultcode < 0: error
        // resultcode = 0: timed out (1 sec)
        // in both cases, reset
        ResetDmxData();
        continue;
      }

      int n = m_widget->Read(&current_byte, 1);
      if (n == 0) {
        // no data available -> try again
        continue;
      }
    }

    // 1 bit: 4µs
    // -----------------------------------------
    // 1. BREAK (>= 22 bit): low
    // 2. Mark After Break (>= 1 / 2 bit): high
    // 3. Start Code (11 bit): 0 0000 0000 11
    // 4a. Data Bytes (11 bit): 0 xxxx xxxx 11 (least significant bit first)
    // 4b. Mark Time Between Frames (>= 0 bit): high
    // 5. Mark Time Between Packets (>= 0bit): high

    if (m_dmx_state == BREAK) {
      if (m_dmx_count == 0) {
        switch (current_byte) {
          case   0: m_dmx_count += 8; break;
          case 128: m_dmx_count += 7; break;
          case  64: m_dmx_count += 6; break;
          case  32: m_dmx_count += 5; break;
          case  16: m_dmx_count += 4; break;
          case   8: m_dmx_count += 3; break;
          case   4: m_dmx_count += 2; break;
          case   2: m_dmx_count += 1; break;
          default:
            ResetDmxData();
        }
      }
      else if (m_dmx_count <= 14) {
        if (current_byte == 0) {
          m_dmx_count += 8;
        }
        else {
          ResetDmxData();
        }
      }
      else {
        uint8_t remaining_zeros = 22 - m_dmx_count;
        // test if start of current byte contains remaining_zeros zeros
        if (current_byte >> (8 - remaining_zeros) == 0) {

        }
      }

      if (m_dmx_count < 22) {
        m_dmx_state = MAB;
      }
    }
      if (current_byte == 0) {
        m_dmx_count++;
      }
      else {
        ResetDmxData();
      }
    }*/


    //m_last_byte_received = current_byte;

    /*m_buffer.SetFromString("0,1,2,3,4");
    m_port->NewDMXData(m_buffer);*/

    usleep(5000);
  }
  return NULL;
}

}  // namespace uartdmx
}  // namespace plugin
}  // namespace ola
