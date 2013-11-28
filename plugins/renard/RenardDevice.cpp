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
 * RenardDevice.cpp
 * Renard device
 * Copyright (C) 2013 Hakan Lindestaf
 */

#include <set>
#include <string>

#include "ola/Constants.h"
#include "ola/Logging.h"
#include "ola/StringUtils.h"
#include "olad/Preferences.h"
#include "plugins/renard/RenardPort.h"
#include "plugins/renard/RenardWidgetSS8.h"

namespace ola {
namespace plugin {
namespace renard {

using ola::AbstractPlugin;
using ola::io::ConnectedDescriptor;

const char RenardDevice::RENARD_DEVICE_NAME[] = "Renard Device";
const uint8_t RenardDevice::RENARD_CHANNELS_IN_BANK = 8;
const uint8_t RenardDevice::RENARD_AVAILABLE_ADDRESSES = 128;
const uint8_t RenardDevice::DEFAULT_DMX_OFFSET = 0;
const uint8_t RenardDevice::DEFAULT_NUM_CHANNELS = 64;
const uint32_t RenardDevice::DEFAULT_BAUDRATE = 57600;
const char RenardDevice::BAUDRATE_19200[] = "19200";
const char RenardDevice::BAUDRATE_38400[] = "38400";
const char RenardDevice::BAUDRATE_57600[] = "57600";
const char RenardDevice::BAUDRATE_115200[] = "115200";

/*
 * Create a new device
 *
 * @param owner the plugin that owns this device
 * @param preferences config settings
 * @param dev_path path to the pro widget
 */
RenardDevice::RenardDevice(AbstractPlugin *owner,
                           class Preferences *preferences,
                           const string &dev_path)
    : Device(owner, RENARD_DEVICE_NAME),
      m_path(dev_path),
      m_preferences(preferences) {

  m_device_name = dev_path;
  OLA_INFO << "Create device " << m_device_name;

  SetDefaults();

  uint8_t dmxOffset;
  if (!StringToInt(m_preferences->GetValue(DeviceDmxOffsetKey()), &dmxOffset))
    dmxOffset = DEFAULT_DMX_OFFSET;

  uint8_t channels;
  if (!StringToInt(m_preferences->GetValue(DeviceChannelsKey()), &channels))
    channels = DEFAULT_NUM_CHANNELS;

  uint32_t baudrate;
  if (!StringToInt(m_preferences->GetValue(DeviceBaudrateKey()), &baudrate))
    baudrate = DEFAULT_BAUDRATE;

  m_widget.reset(new RenardWidgetSS8(m_path, dmxOffset, channels, baudrate));

  OLA_DEBUG << "DMX offset set to " << static_cast<int>(dmxOffset);
  OLA_DEBUG << "Channels set to " << static_cast<int>(channels);
  OLA_DEBUG << "Baudrate set to " << static_cast<int>(baudrate);
}


/*
 * Destroy this device
 */
RenardDevice::~RenardDevice() {
  // Stub destructor for compatibility with RenardWidget subclasses
}


/*
 * Start this device
 */
bool RenardDevice::StartHook() {
  if (!m_widget.get())
    return false;

  if (!m_widget->Connect()) {
    OLA_WARN << "Failed to connect to " << m_path;
    return false;
  }

  if (!m_widget->DetectDevice()) {
    OLA_WARN << "No device found at " << m_path;
    return false;
  }

  RenardOutputPort *port = new RenardOutputPort(this, 0, m_widget.get());
  AddPort(port);
  return true;
}


/*
 * Stop this device
 */
void RenardDevice::PrePortStop() {
  // disconnect from widget
  m_widget->Disconnect();

  m_preferences->Save();
}

string RenardDevice::DeviceBaudrateKey() const {
  return m_device_name + "-baudrate";
}

string RenardDevice::DeviceChannelsKey() const {
  return m_device_name + "-channels";
}

string RenardDevice::DeviceDmxOffsetKey() const {
  return m_device_name + "-dmx-offset";
}

void RenardDevice::SetDefaults() {
  set<string> valid_baudrates;
  valid_baudrates.insert(BAUDRATE_19200);
  valid_baudrates.insert(BAUDRATE_38400);
  valid_baudrates.insert(BAUDRATE_57600);
  valid_baudrates.insert(BAUDRATE_115200);

  // Set device options
  m_preferences->SetDefaultValue(DeviceBaudrateKey(),
                                 SetValidator(valid_baudrates),
                                 BAUDRATE_57600);
  m_preferences->SetDefaultValue(DeviceChannelsKey(),
                                 IntValidator(RENARD_CHANNELS_IN_BANK,
                                              RENARD_AVAILABLE_ADDRESSES *
                                              RENARD_CHANNELS_IN_BANK),
                                 IntToString(DEFAULT_NUM_CHANNELS));
  m_preferences->SetDefaultValue(DeviceDmxOffsetKey(),
                                 IntValidator(0, DMX_UNIVERSE_SIZE -
                                              RENARD_CHANNELS_IN_BANK),
                                 IntToString(DEFAULT_DMX_OFFSET));
}

/*
 * return the sd for this device
 */
ConnectedDescriptor *RenardDevice::GetSocket() const {
  return m_widget->GetSocket();
}
}  // namespace renard
}  // namespace plugin
}  // namespace ola
