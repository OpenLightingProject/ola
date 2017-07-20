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
 * SpiDmxWidget.h
 * This is a wrapper around the needed SPIDEV calls.
 * Copyright (C) 2017 Florian Edelmann
 */

#ifndef PLUGINS_SPIDMX_SPIDMXWIDGET_H_
#define PLUGINS_SPIDMX_SPIDMXWIDGET_H_

#include <string>
#include <vector>
#include <linux/spi/spidev.h>

#include "ola/base/Macro.h"
#include "ola/DmxBuffer.h"

namespace ola {
namespace plugin {
namespace spidmx {

/**
 * An SPI widget (i.e. a serial port with suitable hardware attached)
 */
class SpiDmxWidget {
 public:
  /**
    * Construct a new SpiDmxWidget instance for one widget.
    * @param path The device file path of the serial port
    */
  SpiDmxWidget(const std::string &path);

  /** Destructor */
  ~SpiDmxWidget();

  /** Get the widget's device name */
  std::string Name() const { return m_path; }
  std::string Description() const { return m_path; }

  /** Open the widget */
  bool Open();

  /** Close the widget */
  bool Close();

  /** Check if the widget is open */
  bool IsOpen() const;

  /**
   * Read and write data from / to a previously-opened line. This operation
   * works like a shift register. Both buffers must be of the specified size.
   */
  bool ReadWrite(uint8_t *tx_buf, uint8_t *rx_buf, uint32_t blocklength);

  /** Setup device for DMX Output **/
  bool SetupOutput();

 private:
  const std::string m_path;

  /**
   * variable to hold the Unix file descriptor used to open and manipulate
   * the port. Set to -2 when port is not open.
   */
  int m_fd;

  /** Constant value for file is not open */
  static const int NOT_OPEN = -2;

  /** Constant value for failed to open file */
  static const int FAILED_OPEN = -1;

  /** SPI sample frequency (2MHz) = 8x DMX frequency (250kHz) */
  static const uint32_t SPI_SPEED = 2000000;

  /** Don't delay after a read/write operation */
  static const uint16_t SPI_DELAY = 0;

  /** Word size. */
  static const uint8_t SPI_BITS_PER_WORD = 8;

  /** Not relevant since CS line is not used. */
  static const uint8_t SPI_CS_CHANGE = 0;

  /** Not relevant since we always use 8-bit words. */
  static const uint16_t SPI_PAD = 0;

  /** Not relevant since we don't use the clock signal. */
  static const uint8_t SPI_MODE = SPI_MODE_0;

  DISALLOW_COPY_AND_ASSIGN(SpiDmxWidget);
};

}  // namespace spidmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_SPIDMX_SPIDMXWIDGET_H_
