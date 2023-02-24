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
 * ArduinoRGBDevice.cpp
 * The Arduino RGB Mixer device.
 * Copyright (C) 2010 Simon Newton
 */

#include <algorithm>
#include <iomanip>
#include <string>
#include "ola/Constants.h"
#include "ola/Logging.h"
#include "ola/StringUtils.h"
#include "ola/io/SelectServerInterface.h"
#include "ola/strings/Format.h"
#include "plugins/usbpro/ArduinoRGBDevice.h"

namespace ola {
namespace plugin {
namespace usbpro {

using std::string;


/*
 * New Arduino RGB Device
 */
ArduinoRGBDevice::ArduinoRGBDevice(ola::io::SelectServerInterface *ss,
                                   ola::AbstractPlugin *owner,
                                   const string &name,
                                   ArduinoWidget *widget,
                                   uint16_t esta_id,
                                   uint16_t device_id,
                                   uint32_t serial):
    UsbSerialDevice(owner, name, widget) {
  std::ostringstream str;
  str << std::hex << esta_id << "-" << device_id << "-" << serial;
  m_device_id = str.str();

  OutputPort *output_port = new ArduinoRGBOutputPort(
      this,
      widget,
      serial,
      ss->WakeUpTime(),
      5,  // start with 5 tokens in the bucket
      20);  // 22 frames per second seems to be the limit
  AddPort(output_port);
}


ArduinoRGBOutputPort::ArduinoRGBOutputPort(ArduinoRGBDevice *parent,
                                           ArduinoWidget *widget,
                                           uint32_t serial,
                                           const TimeStamp *wake_time,
                                           unsigned int initial_count,
                                           unsigned int rate)
    : BasicOutputPort(parent, 0, true, true),
      m_widget(widget),
      m_bucket(initial_count, rate, rate, *wake_time),
      m_wake_time(wake_time) {
  std::ostringstream str;
  str << "Serial #: " << strings::ToHex(serial);
  m_description = str.str();
}
}  // namespace usbpro
}  // namespace plugin
}  // namespace ola
