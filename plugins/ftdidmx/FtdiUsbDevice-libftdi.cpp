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
  qlcftdi-libftdi.h

  Copyright (C) Heikki Junnila

  Only standard CPP conversion was changed and function name changed
  to follow OLA coding standards.

  by
  Rui Barreiros
*/

#include <string>
#include <strings.h>
#include <ftdi.h>
#include <assert.h>
#include <algorithm>

#include "ola/Logging.h"
#include "ola/BaseTypes.h"
#include "plugins/ftdidmx/FtdiUsbDevice.h"

namespace ola {
namespace plugin {
namespace ftdidmx {

using std::string;

FtdiUsbDevice::FtdiUsbDevice(const string& serial, const string& name, uint32_t id)
  : m_serial(serial)
  , m_name(name)
  , m_id(id)
{
  bzero(&m_handle, sizeof(struct ftdi_context));
  ftdi_init(&m_handle);
}

FtdiUsbDevice::~FtdiUsbDevice()
{
  if (IsOpen() == true)
    Close();
  ftdi_deinit(&m_handle);
}

vector<FtdiUsbDeviceInfo> FtdiUsbDevice::Widgets()
{
  int i = 0;
  vector <FtdiUsbDeviceInfo> widgetList;
  struct ftdi_device_list* list = 0;
  struct ftdi_context ftdi;
  ftdi_init(&ftdi);
  ftdi_usb_find_all(&ftdi, &list, FtdiUsbDevice::VID, FtdiUsbDevice::PID);

  while (list != NULL)
  {
    struct usb_device* dev = list->dev;
    assert(dev != NULL);
    
    char serial[256];
    char name[256];
    char vendor[256];
    
    ftdi_usb_get_strings(&ftdi, dev,
			 vendor, sizeof(vendor),
			 name, sizeof(name),
			 serial, sizeof(serial));

    string v = vendor;
    std::transform(v.begin(), v.end(), v.begin(), ::toupper);
    if(std::string::npos != v.find("FTDI"))
    {
      widgetList.push_back(FtdiUsbDeviceInfo(name, serial, i));      
    }

    list = list->next;
    i++;
  }
  
  ftdi_deinit(&ftdi);
  return widgetList;
}
  
bool FtdiUsbDevice::Open()
{
  if (ftdi_usb_open_desc(&m_handle, FtdiUsbDevice::VID, FtdiUsbDevice::PID,
			 Name().c_str(), Serial().c_str()) < 0)
  {
    OLA_WARN << Name() << " " << ftdi_get_error_string(&m_handle);
    return false;
  }
  else
  {
    return true;
  }
}

bool FtdiUsbDevice::Close()
{
  if (ftdi_usb_close(&m_handle) < 0)
  {
    OLA_WARN << Name() << " " << ftdi_get_error_string(&m_handle);
    return false;
  }
  else
  {
    return true;
  }
}

bool FtdiUsbDevice::IsOpen() const
{
  return (m_handle.usb_dev != NULL) ? true : false;
}

bool FtdiUsbDevice::Reset()
{
  if (ftdi_usb_reset(&m_handle) < 0)
  {
    OLA_WARN << Name() << " " << ftdi_get_error_string(&m_handle);
    return false;
  }
  else
  {
    return true;
  }
}

bool FtdiUsbDevice::SetLineProperties()
{
  if (ftdi_set_line_property(&m_handle, BITS_8, STOP_BIT_2, NONE) < 0)
  {
    OLA_WARN << Name() << " " << ftdi_get_error_string(&m_handle);
    return false;
  }
  else
  {
    return true;
  }
}

bool FtdiUsbDevice::SetBaudRate()
{
  if (ftdi_set_baudrate(&m_handle, 250000) < 0)
  {
    OLA_WARN << Name() << " " << ftdi_get_error_string(&m_handle);
    return false;
  }
  else
  {
    return true;
  }
}

bool FtdiUsbDevice::SetFlowControl()
{
  if (ftdi_setflowctrl(&m_handle, SIO_DISABLE_FLOW_CTRL) < 0)
  {
    OLA_WARN << Name() << " " << ftdi_get_error_string(&m_handle);
    return false;
  }
  else
  {
    return true;
  }
}

bool FtdiUsbDevice::ClearRts()
{
  if (ftdi_setrts(&m_handle, 0) < 0)
  {
    OLA_WARN << Name() << " " << ftdi_get_error_string(&m_handle);
    return false;
  }
  else
  {
    return true;
  }
}

bool FtdiUsbDevice::PurgeBuffers()
{
  if (ftdi_usb_purge_buffers(&m_handle) < 0)
  {
    OLA_WARN << Name() << " " << ftdi_get_error_string(&m_handle);
    return false;
  }
  else
  {
    return true;
  }
}

bool FtdiUsbDevice::SetBreak(bool on)
{
  ftdi_break_type type;
  if (on == true)
    type = BREAK_ON;
  else
    type = BREAK_OFF;
  
  if (ftdi_set_line_property2(&m_handle, BITS_8, STOP_BIT_2, NONE, type) < 0)
  {
    OLA_WARN << Name() << " " << ftdi_get_error_string(&m_handle);
    return false;
  }
  else
  {
    return true;
  }
}

bool FtdiUsbDevice::Write(const ola::DmxBuffer& data)
{
  unsigned char buffer[DMX_UNIVERSE_SIZE + 1];
  int unsigned length = DMX_UNIVERSE_SIZE;
  buffer[0] = 0x00;
  
  data.Get(buffer+1, &length);

  if (ftdi_write_data(&m_handle, buffer, sizeof(buffer)) < 0)
  {
    OLA_WARN << Name() << " " << ftdi_get_error_string(&m_handle);
    return false;
  }
  else
  {
    return true;
  }
}

bool FtdiUsbDevice::Read(unsigned char *buff, int size)
{
  int read = ftdi_read_data(&m_handle, buff, size);
  if (read <= 0)
  {
    OLA_WARN << Name() << " " << ftdi_get_error_string(&m_handle);
    return false;
  }
  else
  {
    return true;
  }
}

}
}
}
