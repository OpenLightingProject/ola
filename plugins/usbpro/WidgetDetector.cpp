/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * WidgetDetector.cpp
 * Handles creating UsbWidget objects.
 * Copyright (C) 2010 Simon Newton
 *
 * The device represents the widget.
 */

#include <errno.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#include <map>
#include <set>
#include <string>

#include "ola/Logging.h"
#include "ola/network/Socket.h"
#include "ola/network/NetworkUtils.h"
#include "plugins/usbpro/WidgetDetector.h"
#include "plugins/usbpro/UsbWidget.h"

namespace ola {
namespace plugin {
namespace usbpro {

using std::map;
using std::set;
using ola::network::NetworkToHost;

/*
 * Delete any in-discovery widgets
 */
WidgetDetector::~WidgetDetector() {
  map<UsbWidget*, DeviceInformation>::iterator iter;
  for (iter = m_widgets.begin(); iter != m_widgets.end(); ++iter)
    delete iter->first;
  m_widgets.clear();

  if (m_timeout_id != ola::network::INVALID_TIMEOUT)
    m_ss->RemoveTimeout(m_timeout_id);
}


/*
 * Start the discovery process for a widget
 * @param path the path to the device
 * @return is the process begun ok, false otherwise.
 */
bool WidgetDetector::Discover(const string &path) {
  struct termios newtio;
  int fd = open(path.data(), O_RDWR | O_NONBLOCK | O_NOCTTY);

  if (fd == -1) {
    OLA_WARN << "Failed to open " << path << " " << strerror(errno);
    return false;
  }

  bzero(&newtio, sizeof(newtio));  // clear struct for new port settings
  cfsetispeed(&newtio, B115200);
  cfsetospeed(&newtio, B115200);
  tcsetattr(fd, TCSANOW, &newtio);

  UsbWidget *widget = new UsbWidget(
      m_ss,
      fd);
  widget->SetMessageHandler(
    NewCallback(this, &WidgetDetector::HandleMessage, widget));

  bool ret = SendDiscoveryMessages(widget);

  if (!ret) {
    delete widget;
    return false;
  }

  m_widgets[widget];

  // Set delete_on_close here so that device removal works
  /*
  widget->SetOnClose(NewSingleClosure(this,
                     &WidgetDetector::DeviceClosed,
                     widget));
  */

  // register a timeout for this widget
  m_timeout_id = m_ss->RegisterSingleTimeout(
      1000,
      NewSingleClosure(this, &WidgetDetector::DiscoveryTimeout, widget));
  return true;
}


/*
 * Called by the widgets when they get messages
 */
void WidgetDetector::HandleMessage(UsbWidget *widget,
                                  uint8_t label,
                                  unsigned int length,
                                  const uint8_t *data) {
  switch (label) {
    case UsbWidget::MANUFACTURER_LABEL:
      HandleIdResponse(widget, length, data, false);
      break;
    case UsbWidget::DEVICE_LABEL:
      HandleIdResponse(widget, length, data, true);
      break;
    case UsbWidget::SERIAL_LABEL:
      HandleSerialResponse(widget, length, data);
      break;
    default:
      OLA_WARN << "Unknown response label";
  }
}


/*
 * Called if a widget fails to respond in a given interval
 */
void WidgetDetector::DiscoveryTimeout(UsbWidget *widget) {
  m_timeout_id = ola::network::INVALID_TIMEOUT;
  map<UsbWidget*, DeviceInformation>::iterator iter = m_widgets.find(widget);

  if (iter != m_widgets.end()) {
    OLA_WARN << "Usb Widget didn't respond to messages, esta id " <<
      iter->second.esta_id << ", device id " << iter->second.device_id;
    delete widget;
    m_widgets.erase(iter);
  }
}


/*
 * Send the discovery messages to the widget
 * @param widget the widget to send to
 */
bool WidgetDetector::SendDiscoveryMessages(UsbWidget *widget) {
  if (!widget->SendMessage(UsbWidget::MANUFACTURER_LABEL, 0, NULL))
    return false;

  if (!widget->SendMessage(UsbWidget::DEVICE_LABEL, 0, NULL))
    return false;

  return widget->SendMessage(UsbWidget::SERIAL_LABEL, 0, NULL);
}


/*
 * Handle a Device Manufacturer or Device Name response.
 * @param widget the UsbWidget
 * @param length length of the response
 * @param data pointer to the response data
 * @param is_device true if this is a device response, false if it's a
 *   manufactuer response.
 */
void WidgetDetector::HandleIdResponse(UsbWidget *widget,
                                      unsigned int length,
                                      const uint8_t *data,
                                      bool is_device) {
  id_response response;
  memset(&response, 0, sizeof(response));
  memcpy(&response, data, length);

  map<UsbWidget*, DeviceInformation>::iterator iter = m_widgets.find(widget);

  if (iter == m_widgets.end())
    return;

  uint16_t id = (response.id_high << 8) + response.id_low;
  if (length < sizeof(id)) {
    OLA_WARN << "Received small response packet";
    return;
  }

  if (is_device) {
    iter->second.device_id = id;
    iter->second.device = string(
        reinterpret_cast<const char*>(response.text));
  } else {
    iter->second.esta_id = id;
    iter->second.manufactuer = string(
        reinterpret_cast<const char*>(response.text));
  }
}


/*
 * Handle a serial response, this ends the device detection phase.
 */
void WidgetDetector::HandleSerialResponse(UsbWidget *widget,
                                         unsigned int length,
                                         const uint8_t *data) {
  map<UsbWidget*, DeviceInformation>::iterator iter = m_widgets.find(widget);

  if (iter == m_widgets.end())
    return;
  DeviceInformation information = iter->second;

  if (length == DeviceInformation::SERIAL_LENGTH)
    memcpy(information.serial, data, DeviceInformation::SERIAL_LENGTH);
  else
    OLA_WARN << "Serial number response size " << length << " != " <<
      DeviceInformation::SERIAL_LENGTH;

  m_widgets.erase(iter);

  OLA_INFO << "Detected USB Device: ESTA Id: 0x" << std::hex <<
    information.esta_id  << " (" << information.manufactuer << "), device: "
    << information.device_id << " (" << information.device << ")";

  if (!m_listener) {
    OLA_WARN << "No listener provided";
    return;
  }

  m_listener->NewWidget(widget, information);
  return;
}
}  // usbpro
}  // plugin
}  // ola
