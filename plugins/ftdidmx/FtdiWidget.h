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
 */

#ifndef PLUGINS_FTDIDMX_FTDIWIDGET_H_
#define PLUGINS_FTDIDMX_FTDIWIDGET_H_

#ifdef FTD2XX
#  ifdef WIN32
#    include <windows.h>
#  endif
#  include <ftd2xx.h>
#else
#  include <ftdi.h>
#endif

#include <string>
#include <vector>

#include "ola/DmxBuffer.h"

namespace ola {
namespace plugin {
namespace ftdidmx {

using std::string;

/**
 * This class holds information about an attached ftdi chip
 */
class FtdiWidgetInfo {
 public:
    FtdiWidgetInfo(const string &name, const string &serial, int unsigned id)
      : m_name(name),
        m_serial(serial),
        m_id(id) {
    }

    FtdiWidgetInfo(const FtdiWidgetInfo &info)
      : m_name(info.Name()),
        m_serial(info.Serial()),
        m_id(info.Id()) {
    }

    virtual ~FtdiWidgetInfo() {}

    string Name() const { return m_name; }
    string Serial() const { return m_serial; }
    int unsigned Id() const { return m_id; }

    string Description() const {
      return m_name + " with serial number : " + m_serial + " ";
    }

    FtdiWidgetInfo& operator=(const FtdiWidgetInfo &other) {
      if (this != &other) {
        m_name = other.Name();
        m_serial = other.Serial();
        m_id = other.Id();
      }
      return *this;
    }

 private:
    string m_name;
    string m_serial;
    int unsigned m_id;
};


/**
 * An FTDI widget
 */
class FtdiWidget {
 public:
    static const int VID = 0x0403;  // FTDI Vendor ID
    static const int PID = 0x6001;  // FTDI Product ID

    /**
     * Construct a new FtdiWidget instance for one widget.
     * @param serial The widget's USB serial number
     * @param name The widget's USB name (description)
     * @param id The ID of the device (used only when FTD2XX is the backend)
     */
    FtdiWidget(const string &serial, const string &name, uint32_t id = 0);

    /** Destructor */
    virtual ~FtdiWidget();

    /** Get the widget's USB serial number */
    string Serial() const { return m_serial; }

    /** Get the widget's USB name */
    string Name() const { return m_name; }

    /** Get the widget's FTD2XX ID number */
    uint32_t Id() const { return m_id; }

    string Description() const {
      return m_name + " with serial number : " + m_serial +" ";
    }

    /** Open the widget */
    bool Open();

    /** Close the widget */
    bool Close();

    /** Check if the widget is open */
    bool IsOpen() const;

    /** Reset the communications line */
    bool Reset();

    /** Setup communications line for 8N2 traffic */
    bool SetLineProperties();

    /** Set 250kbps baud rate */
    bool SetBaudRate();

    /** Disable flow control */
    bool SetFlowControl();

    /** Clear the RTS bit */
    bool ClearRts();

    /** Purge TX & RX buffers */
    bool PurgeBuffers();

    /** Toggle communications line BREAK condition on/off */
    bool SetBreak(bool on);

    /** Write data to a previously-opened line */
    bool Write(const ola::DmxBuffer &data);

    /** Read data from a previously-opened line */
    bool Read(unsigned char* buff, int size);

    /** Setup device for DMX Output **/
    bool SetupOutput();

    /**
     * Build a list of available ftdi widgets.
     * @param widgets a pointer to a vector of FtdiWidgetInfo objects.
     */
    static void Widgets(std::vector<FtdiWidgetInfo> *widgets);

 private:
    string m_serial;
    string m_name;
    uint32_t m_id;

#ifdef FTD2XX
    FT_HANDLE m_handle;
#else
    struct ftdi_context m_handle;
#endif
};
}  // namespace ftdidmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_FTDIDMX_FTDIWIDGET_H_
