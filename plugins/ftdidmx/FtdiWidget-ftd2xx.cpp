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
  qlcftdi-ftd2xx.cpp

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

/**
 * Get some interesting strings from the device.
 *
 * @param deviceIndex The device index, whose strings to get
 * @param vendor Returned vendor string
 * @param description Returned description string
 * @param serial Returned serial string
 * @return FT_OK if strings were extracted successfully
 */
static FT_STATUS qlcftdi_get_strings(DWORD deviceIndex,
				     string& vendor,
				     string& description,
				     string& serial) {
  char cVendor[256];
  char cVendorId[256];
  char cDescription[256];
  char cSerial[256];

  FT_HANDLE handle;

  FT_STATUS status = FT_Open(deviceIndex, &handle);
  if (status != FT_OK)
    return status;

  FT_PROGRAM_DATA pData;
  pData.Signature1 = 0;
  pData.Signature2 = 0xFFFFFFFF;
  pData.Version = 0x00000005;
  pData.Manufacturer = cVendor;
  pData.ManufacturerId = cVendorId;
  pData.Description = cDescription;
  pData.SerialNumber = cSerial;
  status = FT_EE_Read(handle, &pData);
  if (status == FT_OK) {
    vendor = string(cVendor);
    description = string(cDescription);
    serial = string(cSerial);
  }

  FT_Close(handle);

  return status;
}

FtdiWidget::FtdiWidget(const string& serial,
                             const string& name,
                             uint32_t id)
  : m_serial(serial)
  , m_name(name)
  , m_id(id) {
}

FtdiWidget::~FtdiWidget() {
  if (IsOpen() == true)
    Close();
}

vector<FtdiWidgetInfo> FtdiWidget::Widgets() {
  int i = 0;
  vector <FtdiWidgetInfo> widgetList;

  /* Find out the number of FTDI devices present */
  DWORD num = 0;
  FT_STATUS status = FT_CreateDeviceInfoList(&num);
  if (status != FT_OK) {
    qWarning() << Q_FUNC_INFO << "CreateDeviceInfoList:" << status;
    return list;
  } else if (num <= 0) {
    return list;
  }

  // Allocate storage for list based on numDevices
  FT_DEVICE_LIST_INFO_NODE* devInfo = new FT_DEVICE_LIST_INFO_NODE[num];

  // Get the device information list
  if (FT_GetDeviceInfoList(devInfo, &num) == FT_OK) {
    for (DWORD i = 0; i < num; i++) {
      string vendor, description, serial;

      if (qlcftdi_get_strings(i, vendor, description, serial) != FT_OK)
	continue;

      if (vendor.toUpper().contains("FTDI") == true)
	widgetList.push_back(FtdiWidgetInfo(name, serial, i);
    }
  }

  delete [] devInfo;
  return widgetList;
}

bool FtdiWidget::Open() {
  FT_STATUS status = FT_Open(m_id, &m_handle);
  if (status != FT_OK) {
    OLA_WARN << name() << " " << status;
    return false;
  } else {
    return true;
  }
}

bool FtdiWidget::Close() {
  FT_STATUS status = FT_Close(m_handle);
  m_handle = NULL;
  if (status != FT_OK) {
    OLA_WARN << name() << " " << status;
    return false;
  } else {
    return true;
  }
}

bool FtdiWidget::IsOpen() const {
  return (m_handle != NULL) ? true : false;
}

bool FtdiWidget::Reset() {
  FT_STATUS status = FT_ResetDevice(m_handle);
  if (status != FT_OK) {
    OLA_WARN << name() << " " << status;
    return false;
  } else {
    return true;
  }
}

bool FtdiWidget::SetLineProperties() {
  FT_STATUS status = FT_SetDataCharacteristics(m_handle, 8, 2, 0);
  if (status != FT_OK) {
    OLA_WARN << name() << " " << status;
    return false;
  } else {
    return true;
  }
}

bool FtdiWidget::SetBaudRate() {
  FT_STATUS status = FT_SetBaudRate(m_handle, 250000);
  if (status != FT_OK) {
    OLA_WARN << name() << " " << status;
    return false;
  } else {
    return true;
  }
}

bool FtdiWidget::SetFlowControl() {
  FT_STATUS status = FT_SetFlowControl(m_handle, 0, 0, 0);
  if (status != FT_OK) {
    OLA_WARN << name() << " " << status;
    return false;
  } else {
    return true;
  }
}

bool FtdiWidget::ClearRts() {
  FT_STATUS status = FT_ClrRts(m_handle);
  if (status != FT_OK) {
    OLA_WARN << name() << " " << status;
    return false;
  } else {
    return true;
  }
}

bool FtdiWidget::PurgeBuffers() {
  FT_STATUS status = FT_Purge(m_handle, FT_PURGE_RX | FT_PURGE_TX);
  if (status != FT_OK) {
    OLA_WARN << name() << " " << status;
    return false;
  } else {
    return true;
  }
}

bool FtdiWidget::SetBreak(bool on) {
  FT_STATUS status;
  if (on == true)
    status = FT_SetBreakOn(m_handle);
  else
    status = FT_SetBreakOff(m_handle);

  if (status != FT_OK) {
    OLA_WARN << name() << " " << status;
    return false;
  } else {
    return true;
  }
}

bool FtdiWidget::Write(const DmxBuffer& data) {
  DWORD written = 0;
  unsigned char buffer[DMX_UNIVERSE_SIZE + 1];
  int unsigned length = DMX_UNIVERSE_SIZE;
  buffer[0] = 0x00;

  data.Get(buffer+1, &length);

  FT_STATUS status = FT_Write(m_handle, buffer, sizeof(buffer), &written);
  if (status != FT_OK) {
    OLA_WARN << name() << " " << status;
    return false;
  } else {
    return true;
  }
}

bool FtdiWidget::Read(unsigned char *buffer, int size) {
  DWORD read = 0;

  FT_STATUS status = FT_Read(m_handle, buffer, size, &read);
  if (status != FT_OK) {
    OLA_WARN << name() << " " << status;
    return false;
  } else {
    return true;
  }
}
}
}
}
