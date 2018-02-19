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
#include "plugins/renard/RenardWidget.h"
#include "plugins/renard/RenardDevice.h"

namespace ola {
namespace plugin {
namespace renard {

using ola::AbstractPlugin;
using ola::io::ConnectedDescriptor;
using std::set;
using std::string;

const char RenardDevice::RENARD_DEVICE_NAME[] = "Renard Device";
// The default Renard firmware has 0x80 as start address. It would be
// possible to make this configurable in a future release if needed.
const uint8_t RenardDevice::RENARD_START_ADDRESS = 0x80;
// Between 0x80 and 0xFF
const uint8_t RenardDevice::RENARD_AVAILABLE_ADDRESSES = 127;
const uint8_t RenardDevice::DEFAULT_DMX_OFFSET = 0;
const uint8_t RenardDevice::DEFAULT_NUM_CHANNELS = 64;
const uint32_t RenardDevice::DEFAULT_BAUDRATE = ola::io::BAUD_RATE_57600;

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
      m_dev_path(dev_path),
      m_preferences(preferences) {

  OLA_INFO << "Create device " << m_dev_path;

  SetDefaults();

  uint32_t dmxOffset;
  if (!StringToInt(m_preferences->GetValue(DeviceDmxOffsetKey()),
                                           &dmxOffset)) {
    dmxOffset = DEFAULT_DMX_OFFSET;
  }

  uint32_t channels;
  if (!StringToInt(m_preferences->GetValue(DeviceChannelsKey()), &channels)) {
    channels = DEFAULT_NUM_CHANNELS;
  }

  uint32_t baudrate;
  if (!StringToInt(m_preferences->GetValue(DeviceBaudrateKey()), &baudrate)) {
    baudrate = DEFAULT_BAUDRATE;
  }

  m_widget.reset(new RenardWidget(m_dev_path, dmxOffset, channels, baudrate,
                                  RENARD_START_ADDRESS));

  OLA_DEBUG << "DMX offset set to " << static_cast<int>(dmxOffset);
  OLA_DEBUG << "Channels set to " << static_cast<int>(channels);
  OLA_DEBUG << "Baudrate set to " << static_cast<uint32_t>(baudrate);
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
  if (!m_widget.get()) {
    return false;
  }

  if (!m_widget->Connect()) {
    OLA_WARN << "Failed to connect to " << m_dev_path;
    return false;
  }

  if (!m_widget->DetectDevice()) {
    OLA_WARN << "No device found at " << m_dev_path;
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
  return m_dev_path + "-baudrate";
}

string RenardDevice::DeviceChannelsKey() const {
  return m_dev_path + "-channels";
}

string RenardDevice::DeviceDmxOffsetKey() const {
  return m_dev_path + "-dmx-offset";
}

void RenardDevice::SetDefaults() {
  set<unsigned int> valid_baudrates;
  valid_baudrates.insert(ola::io::BAUD_RATE_19200);
  valid_baudrates.insert(ola::io::BAUD_RATE_38400);
  valid_baudrates.insert(ola::io::BAUD_RATE_57600);
  valid_baudrates.insert(ola::io::BAUD_RATE_115200);

  // Set device options
  m_preferences->SetDefaultValue(DeviceBaudrateKey(),
                                 SetValidator<unsigned int>(valid_baudrates),
                                 DEFAULT_BAUDRATE);
  // Renard supports more than 512 channels, but in our application
  // we're tied to a single DMX universe so we'll limit it to 512 channels.
  m_preferences->SetDefaultValue(
      DeviceChannelsKey(),
      UIntValidator(RenardWidget::RENARD_CHANNELS_IN_BANK, DMX_UNIVERSE_SIZE),
      DEFAULT_NUM_CHANNELS);
  m_preferences->SetDefaultValue(
      DeviceDmxOffsetKey(),
      UIntValidator(
          0, DMX_UNIVERSE_SIZE - RenardWidget::RENARD_CHANNELS_IN_BANK),
      DEFAULT_DMX_OFFSET);
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
