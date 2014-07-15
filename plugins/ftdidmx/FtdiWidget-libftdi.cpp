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

#include <strings.h>
#include <ftdi.h>
#include <assert.h>

#include <string>
#include <algorithm>
#include <vector>
#include <libusb-1.0/libusb.h>

#include "ola/Logging.h"
#include "ola/BaseTypes.h"
#include "plugins/ftdidmx/FtdiWidget.h"

namespace ola {
namespace plugin {
namespace ftdidmx {

using std::string;
using std::vector;

FtdiWidget::FtdiWidget(const string& serial,
                       const string& name,
                       uint32_t id,
                       const int vid,
                       const int pid
                      )
    : m_serial(serial),
      m_name(name),
      m_id(id),
      m_vid(vid),
      m_pid(pid) {}

FtdiWidget::~FtdiWidget() {
}

/**
 * Get the number of physical interfaces our widgit has to offer.
 * 
 * This does not deal with product names being all caps etc.
 * Originally I had hoped to use ftdi_context::type however, it only gets set properly after the device has been opened.
 */
int FtdiWidget::GetInterfaceCount() {
  if(std::string::npos != m_name.find("Plus4")) {
    return 4;
  } else  if (std::string::npos != m_name.find("Plus2")) {
    return 2;
  } else {
    return 1;
  }
}

/**
 * Build a list of all attached ftdi devices
 */
void FtdiWidget::Widgets(vector<FtdiWidgetInfo> *widgets) {
  int i = -1;
  widgets->clear();
  struct ftdi_context *ftdi = ftdi_new();
  if (!ftdi) {
    OLA_WARN << "Failed to allocate FTDI context";
    return;
  }

  struct ftdi_device_list* list = NULL;

  int devices_found = ftdi_usb_find_all(ftdi, &list, 0, 0);
  if (devices_found < 0)
    OLA_WARN << "Failed to get FTDI devices: " <<  ftdi_get_error_string(ftdi);

  ftdi_device_list* current_device = list;

  while (current_device != NULL) {
    //libftdi1 uses libusb_device* we are for now trying to stick with libftdi0
    //struct libusb_device *dev = current_device->dev;
    struct usb_device *dev = current_device->dev;
    current_device = current_device->next;
    i++;

    if (!dev) {
      OLA_WARN << "Device returned from ftdi_usb_find_all was NULL";
      continue;
    }
    //struct libusb_device_descriptor device_descriptor;
    //libusb_get_device_descriptor(dev, &device_descriptor);

    //if(device_descriptor.idProduct != 0x6001 && device_descriptor.idProduct != 0x6011) {
    if(dev->descriptor.idProduct != 0x6001 && dev->descriptor.idProduct != 0x6011) {
      // Since all FTDI devices are found by ftdi_usb_find_all and I only know that these IDs are supported by this code.
      continue;
    }
    char serial[256];
    char name[256];
    char vendor[256];

    int r = ftdi_usb_get_strings(ftdi, dev,
                                 vendor, sizeof(vendor),
                                 name, sizeof(name),
                                 serial, sizeof(serial));

    // libftdi doesn't enumerate error codes, -9 is 'get serial number failed'
    if (r < 0 && r != -9) {
      OLA_WARN << "Unable to fetch string information from USB device: " <<
        ftdi_get_error_string(ftdi);
      continue;
    }

    string v = string(vendor);
    string sname = string(name);
    string sserial = string(serial);
    if (sserial == "?" || r == -9) {
      // this means there wasn't a serial number
      sserial.clear();
    }
    OLA_INFO << "Found FTDI device. Vendor: '" << v << "', Name: '" << sname <<
      "', Serial: '" << sserial << "'";
    std::transform(v.begin(), v.end(), v.begin(), ::toupper);
    if (std::string::npos != v.find("FTDI") ||
        std::string::npos != v.find("KMTRONIC") ||
        std::string::npos != v.find("WWW.SOH.CZ")) {
      //widgets->push_back(FtdiWidgetInfo(sname, sserial, i, device_descriptor.idVendor, device_descriptor.idProduct));
      widgets->push_back(FtdiWidgetInfo(sname, sserial, i, dev->descriptor.idVendor, dev->descriptor.idProduct));
    } else {
      OLA_INFO << "Unknown FTDI device with vendor string: '" << v << "'";
    }

  }// while (list != NULL)
  OLA_DEBUG << "Freeing list";
  ftdi_list_free(&list);
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
  if(IsOpen()) {
    Close();
  }
  ftdi_deinit(&m_handle);
}

bool FtdiInterface::SetInterface() {
  OLA_INFO << "Setting interface to: " << m_interface;
  if(ftdi_set_interface(&m_handle, m_interface) < 0) {
    OLA_WARN << m_parent->Name() << " " << ftdi_get_error_string(&m_handle);
    return false;
  } else {
    return true;
  }
}

bool FtdiInterface::Open() {
  if (m_parent->Serial().empty()) {
    OLA_WARN << m_parent->Name() << " has no serial number, "
      "might cause issues with multiple devices";
    if (ftdi_usb_open(&m_handle, m_parent->Vid(), m_parent->Pid()) < 0) {
      OLA_WARN << m_parent->Name() << " " << ftdi_get_error_string(&m_handle);
      return false;
    } else {
      return true;
    }
  } else {
    OLA_DEBUG << "Opening FTDI device " << m_parent->Name() << ", serial: " 
              << m_parent->Serial() << ", interface: " << m_interface;

    if (ftdi_usb_open_desc(&m_handle, m_parent->Vid(), m_parent->Pid(),
                           m_parent->Name().c_str(), m_parent->Serial().c_str()) < 0) {
      OLA_WARN << m_parent->Name() << " " << ftdi_get_error_string(&m_handle);
      return false;
    } else {
      return true;
    }
  }
}

bool FtdiInterface::Close() {
  if (ftdi_usb_close(&m_handle) < 0) {
    OLA_WARN << m_parent->Name() << " " << ftdi_get_error_string(&m_handle);
    return false;
  } else {
    return true;
  }
}

bool FtdiInterface::IsOpen() const {
  return (m_handle.usb_dev != NULL) ? true : false;
}

bool FtdiInterface::Reset() {
  if (ftdi_usb_reset(&m_handle) < 0) {
    OLA_WARN << m_parent->Name() << " " << ftdi_get_error_string(&m_handle);
    return false;
  } else {
    return true;
  }
}

bool FtdiInterface::SetLineProperties() {
    if ((ftdi_set_line_property(&m_handle, BITS_8, STOP_BIT_2, NONE) < 0)/* || (ftdi_set_interface(&m_handle, 2) < 0)*/) {
    OLA_WARN << m_parent->Name() << " " << ftdi_get_error_string(&m_handle);
    return false;
  } else {
    return true;
  }
}

bool FtdiInterface::SetBaudRate() {
  if (ftdi_set_baudrate(&m_handle, 250000) < 0) {
    OLA_WARN << m_parent->Name() << " " << ftdi_get_error_string(&m_handle);
    return false;
  } else {
    return true;
  }
}

bool FtdiInterface::SetFlowControl() {
  if (ftdi_setflowctrl(&m_handle, SIO_DISABLE_FLOW_CTRL) < 0) {
    OLA_WARN << m_parent->Name() << " " << ftdi_get_error_string(&m_handle);
    return false;
  } else {
    return true;
  }
}

bool FtdiInterface::ClearRts() {
  if (ftdi_setrts(&m_handle, 0) < 0) {
    OLA_WARN << m_parent->Name() << " " << ftdi_get_error_string(&m_handle);
    return false;
  } else {
    return true;
  }
}

bool FtdiInterface::PurgeBuffers() {
  if (ftdi_usb_purge_buffers(&m_handle) < 0) {
    OLA_WARN << m_parent->Name() << " " << ftdi_get_error_string(&m_handle);
    return false;
  } else {
    return true;
  }
}

bool FtdiInterface::SetBreak(bool on) {
  ftdi_break_type type;
  if (on == true)
    type = BREAK_ON;
  else
    type = BREAK_OFF;

  if (ftdi_set_line_property2(&m_handle, BITS_8, STOP_BIT_2, NONE, type) < 0) {
    OLA_WARN << m_parent->Name() << " " << ftdi_get_error_string(&m_handle);
    return false;
  } else {
    return true;
  }
}

bool FtdiInterface::Write(const ola::DmxBuffer& data) {
  unsigned char buffer[DMX_UNIVERSE_SIZE + 1];
  int unsigned length = DMX_UNIVERSE_SIZE;
  buffer[0] = 0x00;

  data.Get(buffer + 1, &length);

  if (ftdi_write_data(&m_handle, buffer, length + 1) < 0) {
    OLA_WARN << m_parent->Name() << " " << ftdi_get_error_string(&m_handle);
    return false;
  } else {
    return true;
  }
}

bool FtdiInterface::Read(unsigned char *buff, int size) {
  int read = ftdi_read_data(&m_handle, buff, size);
  if (read <= 0) {
    OLA_WARN << m_parent->Name() << " " << ftdi_get_error_string(&m_handle);
    return false;
  } else {
    return true;
  }
}

bool FtdiInterface::SetupOutput() {
  // Setup the widget
  if (SetInterface() == false) {
    OLA_WARN << "Error setting the device interface.";
    return false;
  }

  if (Open() == false) {
    OLA_WARN << "Error Opening widget";
    return false;
  }

  if (Reset() == false) {
    OLA_WARN << "Error Resetting widget";
    return false;
  }

  if (SetBaudRate() == false) {
    OLA_WARN << "Error Setting baudrate";
    return false;
  }

  if (SetLineProperties() == false) {
    OLA_WARN << "Error setting line properties";
    return false;
  }

  if (SetFlowControl() == false) {
    OLA_WARN << "Error setting flow control";
    return false;
  }

  if (PurgeBuffers() == false) {
    OLA_WARN << "Error purging buffers";
    return false;
  }

  if (ClearRts() == false) {
    OLA_WARN << "Error clearing rts";
    return false;
  }

  return true;
}

}  // namespace ftdidmx
}  // namespace plugin
}  // namespace ola
