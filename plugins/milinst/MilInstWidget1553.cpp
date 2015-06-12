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
 * MilInstWidget1553.cpp
 * The MilInst 1-553 Widget.
 * Copyright (C) 2013 Peter Newman
 */

#include <algorithm>
#include <set>
#include <string>

#include "ola/io/Serial.h"
#include "ola/Logging.h"
#include "ola/StringUtils.h"
#include "ola/util/Utils.h"
#include "olad/Preferences.h"
#include "plugins/milinst/MilInstWidget1553.h"

namespace ola {
namespace plugin {
namespace milinst {

using std::set;
using std::string;

const speed_t MilInstWidget1553::DEFAULT_BAUDRATE = B9600;

const uint16_t MilInstWidget1553::CHANNELS_128 = 128;
const uint16_t MilInstWidget1553::CHANNELS_256 = 256;
const uint16_t MilInstWidget1553::CHANNELS_512 = 512;
const uint16_t MilInstWidget1553::DEFAULT_CHANNELS = CHANNELS_128;


MilInstWidget1553::MilInstWidget1553(const string &path,
                                     Preferences *preferences)
    : MilInstWidget(path),
      m_preferences(preferences) {
  SetWidgetDefaults();

  if (!StringToInt(m_preferences->GetValue(ChannelsKey()), &m_channels)) {
    OLA_DEBUG << "Invalid channels, defaulting to " << DEFAULT_CHANNELS;
    m_channels = DEFAULT_CHANNELS;
  }
}


/*
 * Connect to the widget
 */
bool MilInstWidget1553::Connect() {
  OLA_DEBUG << "Connecting to " << m_path;

  speed_t baudrate;
  uint32_t baudrate_int;
  if (!StringToInt(m_preferences->GetValue(BaudRateKey()), &baudrate_int) ||
      !ola::io::UIntToSpeedT(baudrate_int, &baudrate)) {
    OLA_DEBUG << "Invalid baudrate, defaulting to 9600";
    baudrate = DEFAULT_BAUDRATE;
  }

  int fd = ConnectToWidget(m_path, baudrate);

  if (fd < 0) {
    return false;
  }

  m_socket = new ola::io::DeviceDescriptor(fd);
  m_socket->SetOnData(
      NewCallback<MilInstWidget1553>(this, &MilInstWidget1553::SocketReady));

  OLA_DEBUG << "Connected to " << m_path;
  return true;
}


/*
 * Called when there is data to read
 */
void MilInstWidget1553::SocketReady() {
  while (m_socket->DataRemaining() > 0) {
    uint8_t byte = 0x00;
    unsigned int data_read;

    int ret = m_socket->Receive(&byte, 1, data_read);

    if (ret == -1 || data_read != 1) {
    } else {
      OLA_DEBUG << "Received byte " << static_cast<int>(byte);
    }
  }
}


/*
 * Check if this is actually a MilInst device
 * @return true if this is a milinst,  false otherwise
 */
bool MilInstWidget1553::DetectDevice() {
  // TODO(Peter): Fixme, check channel count or something!
  return true;
}


/*
 * Send a DMX msg.
  */
bool MilInstWidget1553::SendDmx(const DmxBuffer &buffer) const {
  // TODO(Peter): Probably add offset in here to send higher channels shifted
  // down
  int bytes_sent = Send(buffer);
  OLA_DEBUG << "Sending DMX, sent " << bytes_sent << " bytes";
  // Should this confirm we've sent more than 0 bytes and return false if not?
  return true;
}


//-----------------------------------------------------------------------------
// Private methods used for communicating with the widget
/*
 * Set a single channel
 */
int MilInstWidget1553::SetChannel(unsigned int chan, uint8_t val) const {
  uint8_t msg[4];

  msg[0] = MILINST_1553_LOAD_COMMAND;
  ola::utils::SplitUInt16(chan, &msg[1], &msg[2]);
  msg[3] = val;
  OLA_DEBUG << "Setting " << chan << " to " << static_cast<int>(val);
  return m_socket->Send(msg, sizeof(msg));
}


/*
 * Send data
 * @param buffer a DmxBuffer with the data
 */
int MilInstWidget1553::Send(const DmxBuffer &buffer) const {
  unsigned int channels = std::min(static_cast<unsigned int>(m_channels),
                                   buffer.Size());
  uint8_t msg[3 + channels];

  msg[0] = MILINST_1553_LOAD_COMMAND;
  ola::utils::SplitUInt16(1, &msg[1], &msg[2]);

  buffer.Get(msg + 3, &channels);

  return m_socket->Send(msg, sizeof(msg));
}


string MilInstWidget1553::BaudRateKey() const {
  return m_path + "-baudrate";
}


string MilInstWidget1553::ChannelsKey() const {
  return m_path + "-channels";
}


void MilInstWidget1553::SetWidgetDefaults() {
  bool save = false;

  set<unsigned int> valid_baudrates;
  valid_baudrates.insert(ola::io::BAUD_RATE_9600);
  valid_baudrates.insert(ola::io::BAUD_RATE_19200);

  set<unsigned int> valid_channels;
  valid_channels.insert(CHANNELS_128);
  valid_channels.insert(CHANNELS_256);
  valid_channels.insert(CHANNELS_512);

  // Set 1-553 widget options
  save |= m_preferences->SetDefaultValue(
      BaudRateKey(),
      SetValidator<unsigned int>(valid_baudrates),
      ola::io::BAUD_RATE_9600);

  // TODO(Peter): Fix me, default to 512 once we can set the channel count or it
  // behaves properly when sending higher channel counts when limited
  save |= m_preferences->SetDefaultValue(
      ChannelsKey(),
      SetValidator<unsigned int>(valid_channels),
      DEFAULT_CHANNELS);

  if (save) {
    m_preferences->Save();
  }
}
}  // namespace milinst
}  // namespace plugin
}  // namespace ola
