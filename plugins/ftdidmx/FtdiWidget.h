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
 * FtdiWidget.h
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
 *
 * Additional modifications to enable support for multiple outputs and
 * additional devices ids did change the original structure.
 *
 * by E.S. Rosenberg a.k.a. Keeper of the Keys 5774/2014
 */

#ifndef PLUGINS_FTDIDMX_FTDIWIDGET_H_
#define PLUGINS_FTDIDMX_FTDIWIDGET_H_

#if HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H

#ifdef HAVE_LIBFTDI1
#include <libftdi1/ftdi.h>
#else
#include <ftdi.h>
#endif  // HAVE_LIBFTDI1

#include <stdint.h>
#include <string.h>

#include <string>
#include <vector>

#include "ola/DmxBuffer.h"

namespace ola {
namespace plugin {
namespace ftdidmx {

/**
 * @brief This class holds information about an attached FTDI chip
 */
class FtdiWidgetInfo {
 public:
  static const uint16_t FTDI_VID;
  static const uint16_t FT232_PID;
  static const uint16_t FT4232_PID;

  FtdiWidgetInfo(const std::string &name,
                 const std::string &serial,
                 unsigned int id,
                 const uint16_t vid = FTDI_VID,
                 const uint16_t pid = FT232_PID)
    : m_name(name),
      m_serial(serial),
      m_id(id),
      m_vid(vid),
      m_pid(pid) {
  }

  FtdiWidgetInfo(const FtdiWidgetInfo &info)
    : m_name(info.Name()),
      m_serial(info.Serial()),
      m_id(info.Id()),
      m_vid(info.Vid()),
      m_pid(info.Pid()) {
  }

  ~FtdiWidgetInfo() {}

  std::string Name() const { return m_name; }
  std::string Serial() const { return m_serial; }

  unsigned int Id() const { return m_id; }
  uint16_t Vid() const { return m_vid; }
  uint16_t Pid() const { return m_pid; }

  std::string Description() const {
    return m_name + " with serial number : " + m_serial + " ";
  }

  FtdiWidgetInfo& operator=(const FtdiWidgetInfo &other) {
    if (this != &other) {
      m_name = other.Name();
      m_serial = other.Serial();
      m_id = other.Id();
      m_vid = other.Vid();
      m_pid = other.Pid();
    }
    return *this;
  }

 private:
  std::string m_name;
  std::string m_serial;

  unsigned int m_id;
  uint16_t m_vid;
  uint16_t m_pid;
};

/**
 * @brief An FTDI widget
 */
class FtdiWidget {
 public:
  /**
   * @brief Construct a new FtdiWidget instance for one widget.
   * @param serial The widget's USB serial number
   * @param name The widget's USB name (description)
   * @param id based on order of adding it seems from the code
   * @param vid The VendorID of the device, def = FtdiWidgetInfo::ftdi_vid
   * @param pid The ProductID of the device, def = FtdiWidgetInfo::ft232_pid
   */
  FtdiWidget(const std::string &serial,
             const std::string &name,
             uint32_t id = 0,
             const uint16_t vid = FtdiWidgetInfo::FTDI_VID,
             const uint16_t pid = FtdiWidgetInfo::FT232_PID);

  /** @brief Destructor */
  virtual ~FtdiWidget();

  /** @brief Get the widget's USB serial number */
  std::string Serial() const { return m_serial; }

  /** @brief Get the widget's USB name */
  std::string Name() const { return m_name; }

  uint16_t Vid() const { return m_vid; }
  uint16_t Pid() const { return m_pid; }

  /** @brief Get the widget's FTD2XX ID number */
  uint32_t Id() const { return m_id; }

  void setId(uint32_t id) { m_id = id; }

  std::string Description() const {
    return m_name + " serial: " + m_serial;
  }

  /** @brief Get Widget available interface count **/
  int GetInterfaceCount();

  /**
   * @brief Build a list of available ftdi widgets.
   * @param widgets a pointer to a vector of FtdiWidgetInfo objects.
   */
  static void Widgets(std::vector<FtdiWidgetInfo> *widgets);

  // From reading libftdi docs it seems they may reuse error codes, which is
  // why I chose to name this const <lib>_<function>_<error>.
  static const int libftdi_ftdi_usb_get_strings_get_serial_failed = -9;

  static bool m_missing_serial;

 private:
  std::string m_serial;
  std::string m_name;
  uint32_t m_id;
  const uint16_t m_vid;
  const uint16_t m_pid;
};

enum EchoState {
  UNKNOWN,
  ON,
  OFF
};

class FtdiInterface {
 public:
  FtdiInterface(FtdiWidget * parent,
                const ftdi_interface interface);

  virtual ~FtdiInterface();

  std::string Description() const;

  /** @brief Pick interface on multiport widgets */
  bool SetInterface();

  /** @brief Open the widget */
  bool Open();

  /** @brief Close the widget */
  bool Close();

  /** @brief Check if the widget is open */
  bool IsOpen() const;

  /** @brief Reset the communications line */
  bool Reset();

  /** @brief Setup communications line for 8N2 traffic */
  bool SetLineProperties();

  /** @brief Set 250kbps baud rate */
  bool SetBaudRate(int speed = 250000);

  /** @brief Disable flow control */
  bool SetFlowControl();

  /** @brief Clear the RTS bit */
  bool ClearRts();

  /** @brief Purge TX & RX buffers */
  bool PurgeBuffers();

  /** @brief Toggle communications line BREAK condition on/off */
  bool SetBreak(bool on);

  /** @brief Write data to a previously-opened line, DMX only */
  bool Write(const ola::DmxBuffer &data);

  /** @brief Write prepared packets to previously opened line,
   *         agnostic to packet contents
   *  @pre The whole line setup and opening sequence.
   *       Should haven been performed by the plugin before ever reaching this.
   */
  bool Write(ola::io::ByteString *packet);

  /** @brief Read data from a previously-opened line
   *  @pre The whole line setup and opening sequence.
   *       Should haven been performed by the plugin before ever reaching this.
   */
  int Read(unsigned char* buff, int size);

  void DetectEchoState();

  /** @brief Setup device for DMX Output **/
  bool SetupOutput();

 private:
  FtdiWidget * m_parent;
  struct ftdi_context m_handle;
  const ftdi_interface m_interface;
  EchoState m_echoState;
};  // FtdiInterface
}  // namespace ftdidmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_FTDIDMX_FTDIWIDGET_H_
