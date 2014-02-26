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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
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
    UartWidget(const std::string &path, int device_id);

    /** Destructor */
    virtual ~UartWidget();

    /** Get the widget's device name */
    std::string Name() const { return m_path; }
    std::string Description() const { return m_path; }

    int Number() const { return m_device_id; }

    /** Open the widget */
    bool Open();

    /** Close the widget */
    bool Close();

    /** Check if the widget is open */
    bool IsOpen() const;
#if 0
    /** Set 250kbps baud rate */
    bool SetBaudRate();

    /** Purge TX & RX buffers */
    bool PurgeBuffers();
#endif
    /** Toggle communications line BREAK condition on/off */
    bool SetBreak(bool on);

    /** Write data to a previously-opened line */
    bool Write(const ola::DmxBuffer &data);

    /** Read data from a previously-opened line */
    bool Read(unsigned char* buff, int size);

    /** Setup device for DMX Output **/
    bool SetupOutput();

 private:
    std::string m_path;

    int m_device_id;   // a number to tell different ports apart (in the future)
	 /**
	  * variable to hold the Unix file descriptor used to open and manipulate
	  * the port. Set to -2 when port is not open.
	  */
    int m_filed;
};
}  // namespace uartdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_UARTDMX_UARTWIDGET_H_
