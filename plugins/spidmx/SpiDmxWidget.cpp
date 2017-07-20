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
 * SpiDmxWidget.cpp
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
#include <memory.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
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
#include "plugins/spidmx/SpiDmxWidget.h"

namespace ola {
namespace plugin {
namespace spidmx {

using std::string;
using std::vector;

SpiDmxWidget::SpiDmxWidget(const string& path)
    : m_path(path),
      m_fd(NOT_OPEN) {
}

SpiDmxWidget::~SpiDmxWidget() {
  if (IsOpen()) {
    Close();
  }
}


bool SpiDmxWidget::Open() {
  OLA_DEBUG << "Opening SPI port " << Name();
  if (!ola::io::Open(m_path, O_RDWR, &m_fd)) {
    m_fd = FAILED_OPEN;
    OLA_WARN << Name() << " failed to open";
    return false;
  } else {
    OLA_DEBUG << "Opened SPI port " << Name();
    return true;
  }
}

bool SpiDmxWidget::Close() {
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

bool SpiDmxWidget::IsOpen() const {
  return m_fd >= 0;
}

bool SpiDmxWidget::ReadWrite(uint8_t *tx_buf, uint8_t *rx_buf,
                             uint32_t blocklength) {
  struct spi_ioc_transfer tr;
  memset(&tr, 0, sizeof(spi_ioc_transfer));

  tr.tx_buf        = (uint64_t) tx_buf;
  tr.rx_buf        = (uint64_t) rx_buf;
  tr.len           = (uint32_t) blocklength;
  tr.speed_hz      = SPI_SPEED;
  tr.delay_usecs   = SPI_DELAY;
  tr.bits_per_word = SPI_BITS_PER_WORD;
	tr.cs_change     = SPI_CS_CHANGE;
	tr.pad           = SPI_PAD;

  int ret = ioctl(m_fd, SPI_IOC_MESSAGE(1), &tr);
  if (ret < 1) {
    OLA_WARN << Name() << " ioctl read/write error";
    return false;
  }
  return true;
}

/**
 * Setup our device for DMX receive.
 * Also used to test if device is working correctly before AddDevice()
 */
bool SpiDmxWidget::SetupOutput() {
  if (!IsOpen()) {
    if (!Open()) {
      return false;
    }
  }

  // spi mode
  uint8_t mode = SPI_MODE;
  int ret = ioctl(m_fd, SPI_IOC_WR_MODE, &mode);
  if (ret == -1) {
    OLA_WARN << Name() << " can't set spi mode";
    return false;
  }

  // bits per word
  uint8_t bits = SPI_BITS_PER_WORD;
  ret = ioctl(m_fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
  if (ret == -1) {
    OLA_WARN << Name() << " can't set bits per word";
    return false;
  }

  ret = ioctl(m_fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
  if (ret == -1) {
    OLA_WARN << Name() << " can't get bits per word";
    return false;
  }
  if (bits != SPI_BITS_PER_WORD) {
    OLA_WARN << Name() << "'s bits per word (" << bits
             << ") are not as expected";
    return false;
  }

  // max speed
  uint32_t speed = SPI_SPEED;
  ret = ioctl(m_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
  if (ret == -1) {
    OLA_WARN << Name() << " can't set max speed";
    return false;
  }

  ret = ioctl(m_fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
  if (ret == -1) {
    OLA_WARN << Name() << " can't get max speed";
    return false;
  }
  if (speed != SPI_SPEED) {
    OLA_WARN << Name() << "'s max speed (" << speed
             << ") is not as expected";
    return false;
  }


  // everything must have worked to get here
  return true;
}

}  // namespace spidmx
}  // namespace plugin
}  // namespace ola
