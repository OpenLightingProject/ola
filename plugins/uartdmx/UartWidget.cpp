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

#include "ola/Logging.h"
#include "ola/BaseTypes.h"
#include "plugins/uartdmx/UartWidget.h"

namespace ola {
namespace plugin {
namespace uartdmx {

using std::string;
using std::vector;

UartWidget::UartWidget(const string& name)
    : m_name(name) {
  m_filed = -2;
}

UartWidget::~UartWidget() {
  if (IsOpen())
    Close();
}


bool UartWidget::Open() {
  OLA_DEBUG << "Opened serial port " << Name();
  m_filed = open(Name().c_str(), O_WRONLY);
  if (m_filed == -1) {
    OLA_WARN << Name() << " failed to open";
    return false;
  } else {
    return true;
  }
}

bool UartWidget::Close() {
  if (close(m_filed) > 0) {
    OLA_WARN << Name() << " error closing";
	m_filed = -2;
    return false;
  } else {
	m_filed = -2;
    return true;
  }
}

bool UartWidget::IsOpen() const {
  return (m_filed > 0) ? true : false;
}
/*
bool UartWidget::SetLineProperties() {
  if (ftdi_set_line_property(&m_handle, BITS_8, STOP_BIT_2, NONE) < 0) {
    OLA_WARN << Name() << " " << ftdi_get_error_string(&m_handle);
    return false;
  } else {
    return true;
  }
}

bool UartWidget::SetBaudRate() {
  if (ftdi_set_baudrate(&m_handle, 250000) < 0) {
    OLA_WARN << Name() << " " << ftdi_get_error_string(&m_handle);
    return false;
  } else {
    return true;
  }
}

bool UartWidget::SetFlowControl() {
  if (ftdi_setflowctrl(&m_handle, SIO_DISABLE_FLOW_CTRL) < 0) {
    OLA_WARN << Name() << " " << ftdi_get_error_string(&m_handle);
    return false;
  } else {
    return true;
  }
}

bool UartWidget::ClearRts() {
  if (ftdi_setrts(&m_handle, 0) < 0) {
    OLA_WARN << Name() << " " << ftdi_get_error_string(&m_handle);
    return false;
  } else {
    return true;
  }
}

bool UartWidget::PurgeBuffers() {
  if (ftdi_usb_purge_buffers(&m_handle) < 0) {
    OLA_WARN << Name() << " " << ftdi_get_error_string(&m_handle);
    return false;
  } else {
    return true;
  }
}
*/
bool UartWidget::SetBreak(bool on) {
  unsigned long request;
  if (on == true)
    request = TIOCSBRK;
  else
    request = TIOCCBRK;

  if (ioctl(m_filed, request, NULL) < 0) {
    OLA_WARN << Name() << " ioctl() failed";
    return false;
  } else {
    return true;
  }
}

bool UartWidget::Write(const ola::DmxBuffer& data) {
  unsigned char buffer[DMX_UNIVERSE_SIZE + 1];
  int unsigned length = DMX_UNIVERSE_SIZE;
  buffer[0] = 0x00;  // start code of 0 for dimmer control messages

  data.Get(buffer + 1, &length);

  if (write(m_filed, buffer, length + 1) <= 0) {
    // TODO: handle errors better as per the test code, especially if we alter the scheduling!
    OLA_WARN << Name() << " Short or failed write!";
    return false;
  } else {
    return true;
  }
}

bool UartWidget::Read(unsigned char *buff, int size) {
  int readb = read(m_filed, buff, size);
  if (readb <= 0) {
    OLA_WARN << Name() << " read error";
    return false;
  } else {
    return true;
  }
}

/**
 * Setup our device for DMX send
 * Mainly used to test if device is working correctly
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

  if (tcgetattr(m_filed, &my_tios) < 0)	// get current settings
    {
    OLA_WARN << "Failed to get POSIX port settings";
    return false;
	}
  cfmakeraw(&my_tios);		// make it a binary data port

  my_tios.c_cflag |= CLOCAL;	// port is local, no flow control

  my_tios.c_cflag &= ~CSIZE;
  my_tios.c_cflag |= CS8;		// 8 bit chars
  my_tios.c_cflag &= ~PARENB;	// no parity
  my_tios.c_cflag |= CSTOPB;	// 2 stop bit for DMX
  my_tios.c_cflag &= ~CRTSCTS;	// no CTS/RTS flow control

  if (tcsetattr(m_filed, TCSANOW, &my_tios) < 0)	// apply settings
    {
    OLA_WARN << "Failed to get POSIX port settings";
    return false;
	}

/*  if (Reset() == false) {
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
*/
  return true;
}

#if 0
/**
 * Build a list of all attached ftdi devices
 */
void UartWidget::Widgets(vector<UartWidgetInfo> *widgets) {
  int i = -1;
  widgets->clear();
  struct ftdi_context *ftdi = ftdi_new();
  if (!ftdi) {
    OLA_WARN << "Failed to allocated FTDI context";
    return;
  }

  struct ftdi_device_list* list = NULL;
  int devices_found = ftdi_usb_find_all(ftdi, &list, UartWidget::VID,
                                        UartWidget::PID);
  if (devices_found < 0)
    OLA_WARN << "Failed to get FTDI devices: " <<  ftdi_get_error_string(ftdi);

  while (list != NULL) {
    struct usb_device *dev = list->dev;
    list = list->next;
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
      widgets->push_back(UartWidgetInfo(sname, sserial, i));
    } else {
      OLA_INFO << "Unknown FTDI device with vendor string: '" << v << "'";
    }
  }

  ftdi_list_free(&list);
  ftdi_free(ftdi);
}
#endif
}  // namespace uartdmx
}  // namespace plugin
}  // namespace ola
