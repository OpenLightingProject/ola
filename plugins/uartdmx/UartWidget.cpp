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
 * UartWidget.cpp
 * This class is based on QLCFTDI class from
 *
 * Q Light Controller
 * qlcftdi-libftdi.cpp
 *
 * Copyright (C) Heikki Junnila
 *
 * Only standard CPP conversion was changed and function name changed
 * to follow OLA coding standards.
 *
 * by Rui Barreiros
 * Copyright (C) 2014 Richard Ash
 */

#include <strings.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include <string>
#include <algorithm>
#include <vector>

#include "ola/Constants.h"
#include "ola/io/ExtendedSerial.h"
#include "ola/io/IOUtils.h"
#include "ola/Logging.h"
#include "plugins/uartdmx/UartWidget.h"
#include "plugins/uartdmx/UartDmxDevice.h"

namespace ola {
namespace plugin {
namespace uartdmx {

using std::string;
using std::vector;

UartWidget::UartWidget(const std::string &path, unsigned int padding)
    : m_path(path),
      m_padding(padding),
      m_fd(NOT_OPEN) {
}

UartWidget::~UartWidget() {
  if (IsOpen()) {
    Close();
  }
}


bool UartWidget::Open() {
  OLA_DEBUG << "Opening serial port " << Name();
  if (!ola::io::Open(m_path, O_WRONLY, &m_fd)) {
    m_fd = FAILED_OPEN;
    OLA_WARN << Name() << " failed to open";
    return false;
  } else {
    OLA_DEBUG << "Opened serial port " << Name();
    return true;
  }
}

bool UartWidget::Close() {
  if (!IsOpen()) {
    return true;
  }

  if (close(m_fd) > 0) {
    OLA_WARN << Name() << " error closing";
    m_fd = NOT_OPEN;
    return false;
  } else {
    m_fd = NOT_OPEN;
    return true;
  }
}

bool UartWidget::IsOpen() const {
  return m_fd >= 0;
}

bool UartWidget::SetBreak(bool on) {
  unsigned long request;  /* NOLINT(runtime/int) */
  /* this is passed to ioctl, which is declared to take
   * unsigned long as it's second argument
   */
  if (on == true) {
    request = TIOCSBRK;
  } else {
    request = TIOCCBRK;
  }

  if (ioctl(m_fd, request, NULL) < 0) {
    OLA_WARN << Name() << " ioctl() failed";
    return false;
  } else {
    return true;
  }
}

bool UartWidget::Write(const ola::DmxBuffer& data) {
  unsigned char buffer[DMX_UNIVERSE_SIZE + 1];
  unsigned int length = DMX_UNIVERSE_SIZE;
  buffer[0] = DMX512_START_CODE;

  data.Get(buffer + 1, &length);
  if (length < m_padding) {
  	  memset((buffer + 1 + length), 0x00, (m_padding - length) );
      length = m_padding;
    }

  if (write(m_fd, buffer, length + 1) <= 0) {
    // TODO(richardash1981): handle errors better as per the test code,
    // especially if we alter the scheduling!
    OLA_WARN << Name() << " Short or failed write!";
    return false;
  } else {
    return true;
  }
}

bool UartWidget::Read(unsigned char *buff, int size) {
  int readb = read(m_fd, buff, size);
  if (readb <= 0) {
    OLA_WARN << Name() << " read error";
    return false;
  } else {
    return true;
  }
}

/**
 * Setup our device for DMX send
 * Also used to test if device is working correctly
 * before AddDevice()
 */
bool UartWidget::SetupOutput() {
  struct termios my_tios;
  // Setup the Uart for DMX
  if (Open() == false) {
    OLA_WARN << "Error Opening widget";
    return false;
  }
  /* do the port settings */

  if (tcgetattr(m_fd, &my_tios) < 0) {  // get current settings
    OLA_WARN << "Failed to get POSIX port settings";
    return false;
  }
  cfmakeraw(&my_tios);  // make it a binary data port

  my_tios.c_cflag |= CLOCAL;    // port is local, no flow control
  my_tios.c_cflag &= ~CSIZE;
  my_tios.c_cflag |= CS8;       // 8 bit chars
  my_tios.c_cflag &= ~PARENB;   // no parity
  my_tios.c_cflag |= CSTOPB;    // 2 stop bit for DMX
  my_tios.c_cflag &= ~CRTSCTS;  // no CTS/RTS flow control

  if (tcsetattr(m_fd, TCSANOW, &my_tios) < 0) {  // apply settings
    OLA_WARN << "Failed to get POSIX port settings";
    return false;
  }

  /* Do the platform-specific initialisation of the UART to 250kbaud */
  if (!ola::io::LinuxHelper::SetDmxBaud(m_fd)) {
    OLA_WARN << "Failed to set baud rate to 250k";
    return false;
  }
  /* everything must have worked to get here */
  return true;
}

}  // namespace uartdmx
}  // namespace plugin
}  // namespace ola
