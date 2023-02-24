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
 * UartWidget.h
 * This class is based on QLCFTDI class from
 *
 * Q Light Controller
 * qlcftdi.h
 *
 * Copyright (C) Heikki Junnila
 *
 * Only standard CPP conversion was changed and function name changed
 * to follow OLA coding standards.
 *
 * by Rui Barreiros
 * Copyright (C) 2014 Richard Ash
 */

#ifndef PLUGINS_UARTDMX_UARTWIDGET_H_
#define PLUGINS_UARTDMX_UARTWIDGET_H_

#include <string>
#include <vector>
#include "ola/base/Macro.h"
#include "ola/DmxBuffer.h"

namespace ola {
namespace plugin {
namespace uartdmx {

/**
 * An UART widget (i.e. a serial port with suitable hardware attached)
 */
class UartWidget {
 public:
    /**
     * Construct a new UartWidget instance for one widget.
     * @param path The device file path of the serial port
     */
    explicit UartWidget(const std::string &path);

    /** Destructor */
    virtual ~UartWidget();

    /** Get the widget's device name */
    std::string Name() const { return m_path; }
    std::string Description() const { return m_path; }

    /** Open the widget */
    bool Open();

    /** Close the widget */
    bool Close();

    /** Check if the widget is open */
    bool IsOpen() const;

    /** Toggle communications line BREAK condition on/off */
    bool SetBreak(bool on);

    /** Write data to a previously-opened line */
    bool Write(const ola::DmxBuffer &data);

    /** Read data from a previously-opened line */
    bool Read(unsigned char* buff, int size);

    /** Setup device for DMX Output **/
    bool SetupOutput();

 private:
  const std::string m_path;

  /**
   * variable to hold the Unix file descriptor used to open and manipulate
   * the port. Set to -2 when port is not open.
   */
  int m_fd;

  /**
   * Constant value for file is not open
   */
  static const int NOT_OPEN = -2;

  /**
   * Constant value for failed to open file
   */
  static const int FAILED_OPEN = -1;

  DISALLOW_COPY_AND_ASSIGN(UartWidget);
};
}  // namespace uartdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_UARTDMX_UARTWIDGET_H_
