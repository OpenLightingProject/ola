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
 * FtdiWidget.cpp
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
 *
 * Additional modifications to enable support for multiple outputs and
 * additional device ids did change the original structure.
 *
 * by E.S. Rosenberg a.k.a. Keeper of the Keys 5774/2014
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H

#ifdef HAVE_LIBFTDI1
#include <libftdi1/ftdi.h>
#else
#include <ftdi.h>
#endif  // HAVE_LIBFTDI1

#include <assert.h>
#include <strings.h>

#include <string>
#include <algorithm>
#include <vector>

#include "ola/Logging.h"
#include "ola/BaseTypes.h"
#include "ola/StringUtils.h"
#include "ola/Constants.h"
#include "plugins/ftdidmx/FtdiWidget.h"

namespace ola {
namespace plugin {
namespace ftdidmx {

using std::string;
using std::vector;

const uint16_t FtdiWidgetInfo::FTDI_VID = 0x0403;
const uint16_t FtdiWidgetInfo::FT232_PID = 0x6001;
const uint16_t FtdiWidgetInfo::FT4232_PID = 0x6011;

FtdiWidget::FtdiWidget(const string& serial,
                       const string& name,
                       uint32_t id,
                       const uint16_t vid,
                       const uint16_t pid)
    : m_serial(serial),
      m_name(name),
      m_id(id),
      m_vid(vid),
      m_pid(pid) {}

FtdiWidget::~FtdiWidget() {}

/**
 * @brief Get the number of physical interfaces our widget has to offer.
 *
 * This does not deal with product names being named in a different way.
 *
 * Originally I had hoped to use ftdi_context::type however, it only gets set
 * properly after the device has been opened.
 */
int FtdiWidget::GetInterfaceCount() {
  std::string tmp = m_name;
  ToLower(&tmp);
  if (std::string::npos != tmp.find("plus4")) {
    return 4;
  } else  if (std::string::npos != tmp.find("plus2")) {
    return 2;
  } else {
    return 1;
  }
}

bool FtdiWidget::m_missing_serial = false;

/**
 * @brief Build a list of all attached ftdi devices
 */
void FtdiWidget::Widgets(vector<FtdiWidgetInfo> *widgets) {
  int i = -1;
  widgets->clear();
  struct ftdi_context *ftdi = ftdi_new();
  if (!ftdi) {
    OLA_WARN << "Failed to allocate FTDI context";
    return;
  }

  vector<uint16_t> pids;
  pids.push_back(FtdiWidgetInfo::FT232_PID);
  pids.push_back(FtdiWidgetInfo::FT4232_PID);
  const uint16_t vid = FtdiWidgetInfo::FTDI_VID;

  for (vector<uint16_t>::iterator current_pid = pids.begin();
       current_pid != pids.end();
       ++current_pid) {
    struct ftdi_device_list* list = NULL;

    int devices_found = ftdi_usb_find_all(ftdi, &list, vid, *current_pid);

    if (devices_found < 0) {
      OLA_WARN << "Failed to get FTDI devices: "
               << ftdi_get_error_string(ftdi)
               << " with PID: " << *current_pid;
    } else {
      OLA_INFO << "Found " << devices_found << " FTDI devices with PID: "
               << *current_pid << ".";

      ftdi_device_list* current_device = list;

      while (current_device != NULL) {
#if HAVE_LIBFTDI1
        struct libusb_device *dev = current_device->dev;
#else
        struct usb_device *dev = current_device->dev;
#endif  // HAVE_LIBFTDI1
        current_device = current_device->next;
        i++;

        if (!dev) {
          OLA_WARN << "Device returned from ftdi_usb_find_all was NULL";
          continue;
        }

        char serial[256];
        char name[256];
        char vendor[256];

        int r = ftdi_usb_get_strings(ftdi, dev,
                                     vendor, sizeof(vendor),
                                     name, sizeof(name),
                                     serial, sizeof(serial));

        if (r < 0 &&
            r != FtdiWidget::libftdi_ftdi_usb_get_strings_get_serial_failed) {
          OLA_WARN << "Unable to fetch string information from USB device: "
                   << ftdi_get_error_string(ftdi);
          continue;
        }

        string v = string(vendor);
        string sname = string(name);
        string sserial = string(serial);
        if (sserial == "?" ||
            r == FtdiWidget::libftdi_ftdi_usb_get_strings_get_serial_failed) {
          // This means there wasn't a serial number
          sserial.clear();
        }

        if (r == FtdiWidget::libftdi_ftdi_usb_get_strings_get_serial_failed) {
          if (FtdiWidget::m_missing_serial) {
            OLA_WARN << "Failed to read serial number or serial number empty. "
                     << "We can only support one device without a serial "
                     << "number.";
            continue;
          } else {
            OLA_WARN << "Failed to read serial number for " << sname;
            FtdiWidget::m_missing_serial = true;
          }
        }

        OLA_INFO << "Found FTDI device. Vendor: '" << v << "', Name: '"
                 << sname << "', Serial: '" << sserial << "'";
        ToUpper(&v);
        if (std::string::npos != v.find("FTDI") ||
            std::string::npos != v.find("KMTRONIC") ||
            std::string::npos != v.find("KWMATIK") ||
            std::string::npos != v.find("WWW.SOH.CZ")) {
          widgets->push_back(FtdiWidgetInfo(sname,
                                            sserial,
                                            i,
                                            vid,
                                            *current_pid));
        } else {
          OLA_INFO << "Unknown FTDI device with vendor string: '" << v << "'";
        }
      }  // while (list != NULL)
      OLA_DEBUG << "Freeing list";
      ftdi_list_free(&list);
    }
  }
  ftdi_free(ftdi);
}

FtdiInterface::FtdiInterface(const FtdiWidget * parent,
                             const ftdi_interface interface)
      : m_parent(parent),
        m_interface(interface) {
  memset(&m_handle, '\0', sizeof(struct ftdi_context));
  ftdi_init(&m_handle);
}


FtdiInterface::~FtdiInterface() {
  if (IsOpen()) {
    Close();
  }
  ftdi_deinit(&m_handle);
}

bool FtdiInterface::SetInterface() {
  OLA_INFO << "Setting interface to: " << m_interface;
  if (ftdi_set_interface(&m_handle, m_interface) < 0) {
    OLA_WARN << m_parent->Description() << " "
             << ftdi_get_error_string(&m_handle);
    return false;
  } else {
    return true;
  }
}

bool FtdiInterface::Open() {
  if (m_parent->Serial().empty()) {
    OLA_WARN << m_parent->Name() << " has no serial number, which might cause "
             << "issues with multiple devices";
    if (ftdi_usb_open(&m_handle, m_parent->Vid(), m_parent->Pid()) < 0) {
      OLA_WARN << m_parent->Description() << " "
               << ftdi_get_error_string(&m_handle);
      return false;
    } else {
      return true;
    }
  } else {
    OLA_DEBUG << "Opening FTDI device " << m_parent->Name() << ", serial: "
              << m_parent->Serial() << ", interface: " << m_interface;

    if (ftdi_usb_open_desc(&m_handle, m_parent->Vid(), m_parent->Pid(),
                           m_parent->Name().c_str(),
                           m_parent->Serial().c_str()) < 0) {
      OLA_WARN << m_parent->Description() << " "
               << ftdi_get_error_string(&m_handle);
      return false;
    } else {
      return true;
    }
  }
}

bool FtdiInterface::Close() {
  if (ftdi_usb_close(&m_handle) < 0) {
    OLA_WARN << m_parent->Description() << " "
             << ftdi_get_error_string(&m_handle);
    return false;
  } else {
    return true;
  }
}

bool FtdiInterface::IsOpen() const {
  return (m_handle.usb_dev != NULL);
}

bool FtdiInterface::Reset() {
  if (ftdi_usb_reset(&m_handle) < 0) {
    OLA_WARN << m_parent->Description() << " "
             << ftdi_get_error_string(&m_handle);
    return false;
  } else {
    return true;
  }
}

bool FtdiInterface::SetLineProperties() {
  if ((ftdi_set_line_property(&m_handle, BITS_8, STOP_BIT_2, NONE) < 0)) {
    OLA_WARN << m_parent->Description() << " "
             << ftdi_get_error_string(&m_handle);
    return false;
  } else {
    return true;
  }
}

bool FtdiInterface::SetBaudRate(int speed) {
  if (ftdi_set_baudrate(&m_handle, speed) < 0) {
    OLA_WARN << "Error setting " << m_parent->Description() << " to baud rate "
             << "of " << speed << " - " << ftdi_get_error_string(&m_handle);
    return false;
  } else {
    return true;
  }
}

bool FtdiInterface::SetFlowControl() {
  if (ftdi_setflowctrl(&m_handle, SIO_DISABLE_FLOW_CTRL) < 0) {
    OLA_WARN << m_parent->Description() << " "
             << ftdi_get_error_string(&m_handle);
    return false;
  } else {
    return true;
  }
}

bool FtdiInterface::ClearRts() {
  if (ftdi_setrts(&m_handle, 0) < 0) {
    OLA_WARN << m_parent->Description() << " "
             << ftdi_get_error_string(&m_handle);
    return false;
  } else {
    return true;
  }
}

bool FtdiInterface::PurgeBuffers() {
  if (ftdi_usb_purge_buffers(&m_handle) < 0) {
    OLA_WARN << m_parent->Description() << " "
             << ftdi_get_error_string(&m_handle);
    return false;
  } else {
    return true;
  }
}

bool FtdiInterface::SetBreak(bool on) {
  if (ftdi_set_line_property2(&m_handle, BITS_8, STOP_BIT_2, NONE,
                              (on ? BREAK_ON : BREAK_OFF)) < 0) {
    OLA_WARN << m_parent->Description() << " "
             << ftdi_get_error_string(&m_handle);
    return false;
  } else {
    return true;
  }
}

bool FtdiInterface::Write(const ola::DmxBuffer& data) {
  unsigned char buffer[DMX_UNIVERSE_SIZE + 1];
  unsigned int length = DMX_UNIVERSE_SIZE;
  buffer[0] = DMX512_START_CODE;

  data.Get(buffer + 1, &length);

  if (ftdi_write_data(&m_handle, buffer, length + 1) < 0) {
    OLA_WARN << m_parent->Description() << " "
             << ftdi_get_error_string(&m_handle);
    return false;
  } else {
    return true;
  }
}

bool FtdiInterface::Read(unsigned char *buff, int size) {
  int read = ftdi_read_data(&m_handle, buff, size);
  if (read <= 0) {
    OLA_WARN << m_parent->Description() << " "
             << ftdi_get_error_string(&m_handle);
    return false;
  } else {
    return true;
  }
}

bool FtdiInterface::SetupOutput() {
  // Setup the widget
  if (!SetInterface()) {
    OLA_WARN << "Error setting the device interface.";
    return false;
  }

  if (!Open()) {
    OLA_WARN << "Error Opening widget";
    return false;
  }

  if (!Reset()) {
    OLA_WARN << "Error Resetting widget";
    return false;
  }

  if (!SetBaudRate()) {
    OLA_WARN << "Error Setting baudrate";
    return false;
  }

  if (!SetLineProperties()) {
    OLA_WARN << "Error setting line properties";
    return false;
  }

  if (!SetFlowControl()) {
    OLA_WARN << "Error setting flow control";
    return false;
  }

  if (!PurgeBuffers()) {
    OLA_WARN << "Error purging buffers";
    return false;
  }

  if (!ClearRts()) {
    OLA_WARN << "Error clearing rts";
    return false;
  }

  return true;
}

}  // namespace ftdidmx
}  // namespace plugin
}  // namespace ola
