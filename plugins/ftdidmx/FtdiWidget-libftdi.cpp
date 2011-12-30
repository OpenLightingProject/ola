/*
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  Version 2 as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details. The license is
  in the file "COPYING".

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

  This class is based on QLCFTDI class from

  Q Light Controller
  qlcftdi-libftdi.cpp

  Copyright (C) Heikki Junnila

  Only standard CPP conversion was changed and function name changed
  to follow OLA coding standards.

  by
  Rui Barreiros
*/

#include <strings.h>
#include <ftdi.h>
#include <assert.h>

#include <string>
#include <algorithm>
#include <vector>

#include "ola/Logging.h"
#include "ola/BaseTypes.h"
#include "plugins/ftdidmx/FtdiWidget.h"

namespace ola {
namespace plugin {
namespace ftdidmx {

using std::string;

FtdiWidget::FtdiWidget(const string& serial,
                             const string& name,
                             uint32_t id)
  : m_serial(serial)
  , m_name(name)
  , m_id(id) {
  bzero(&m_handle, sizeof(struct ftdi_context));
  ftdi_init(&m_handle);
}

FtdiWidget::~FtdiWidget() {
  if (IsOpen() == true)
    Close();
  ftdi_deinit(&m_handle);
}

vector<FtdiWidgetInfo> FtdiWidget::Widgets() {
  int i = 0;
  vector <FtdiWidgetInfo> widgetList;
  struct ftdi_device_list* list = 0;
  struct ftdi_context ftdi;
  ftdi_init(&ftdi);
  ftdi_usb_find_all(&ftdi, &list, FtdiWidget::VID, FtdiWidget::PID);

  while (list != NULL) {
    struct usb_device* dev = list->dev;
    assert(dev != NULL);

    char serial[256];
    char name[256];
    char vendor[256];

    ftdi_usb_get_strings(&ftdi, dev,
                         vendor, sizeof(vendor),
                         name, sizeof(name),
                         serial, sizeof(serial));

    string v = string(vendor);
    string sname = string(name);
    string sserial = string(serial);
    std::transform(v.begin(), v.end(), v.begin(), ::toupper);
    if (std::string::npos != v.find("FTDI")) {
      widgetList.push_back(FtdiWidgetInfo(sname, sserial, i));
    }

    list = list->next;
    i++;
  }

  ftdi_deinit(&ftdi);
  return widgetList;
}

bool FtdiWidget::Open() {
  if (ftdi_usb_open_desc(&m_handle, FtdiWidget::VID, FtdiWidget::PID,
                         Name().c_str(), Serial().c_str()) < 0) {
    OLA_WARN << Name() << " " << ftdi_get_error_string(&m_handle);
    return false;
  } else {
    return true;
  }
}

bool FtdiWidget::Close() {
  if (ftdi_usb_close(&m_handle) < 0) {
    OLA_WARN << Name() << " " << ftdi_get_error_string(&m_handle);
    return false;
  } else {
    return true;
  }
}

bool FtdiWidget::IsOpen() const {
  return (m_handle.usb_dev != NULL) ? true : false;
}

bool FtdiWidget::Reset() {
  if (ftdi_usb_reset(&m_handle) < 0) {
    OLA_WARN << Name() << " " << ftdi_get_error_string(&m_handle);
    return false;
  } else {
    return true;
  }
}

bool FtdiWidget::SetLineProperties() {
  if (ftdi_set_line_property(&m_handle, BITS_8, STOP_BIT_2, NONE) < 0) {
    OLA_WARN << Name() << " " << ftdi_get_error_string(&m_handle);
    return false;
  } else {
    return true;
  }
}

bool FtdiWidget::SetBaudRate() {
  if (ftdi_set_baudrate(&m_handle, 250000) < 0) {
    OLA_WARN << Name() << " " << ftdi_get_error_string(&m_handle);
    return false;
  } else {
    return true;
  }
}

bool FtdiWidget::SetFlowControl() {
  if (ftdi_setflowctrl(&m_handle, SIO_DISABLE_FLOW_CTRL) < 0) {
    OLA_WARN << Name() << " " << ftdi_get_error_string(&m_handle);
    return false;
  } else {
    return true;
  }
}

bool FtdiWidget::ClearRts() {
  if (ftdi_setrts(&m_handle, 0) < 0) {
    OLA_WARN << Name() << " " << ftdi_get_error_string(&m_handle);
    return false;
  } else {
    return true;
  }
}

bool FtdiWidget::PurgeBuffers() {
  if (ftdi_usb_purge_buffers(&m_handle) < 0) {
    OLA_WARN << Name() << " " << ftdi_get_error_string(&m_handle);
    return false;
  } else {
    return true;
  }
}

bool FtdiWidget::SetBreak(bool on) {
  ftdi_break_type type;
  if (on == true)
    type = BREAK_ON;
  else
    type = BREAK_OFF;

  if (ftdi_set_line_property2(&m_handle, BITS_8, STOP_BIT_2, NONE, type) < 0) {
    OLA_WARN << Name() << " " << ftdi_get_error_string(&m_handle);
    return false;
  } else {
    return true;
  }
}

bool FtdiWidget::Write(const ola::DmxBuffer& data) {
  unsigned char buffer[DMX_UNIVERSE_SIZE + 1];
  int unsigned length = DMX_UNIVERSE_SIZE;
  buffer[0] = 0x00;

  data.Get(buffer+1, &length);

  if (ftdi_write_data(&m_handle, buffer, sizeof(buffer)) < 0) {
    OLA_WARN << Name() << " " << ftdi_get_error_string(&m_handle);
    return false;
  } else {
    return true;
  }
}

bool FtdiWidget::Read(unsigned char *buff, int size) {
  int read = ftdi_read_data(&m_handle, buff, size);
  if (read <= 0) {
    OLA_WARN << Name() << " " << ftdi_get_error_string(&m_handle);
    return false;
  } else {
    return true;
  }
}
}
}
}
