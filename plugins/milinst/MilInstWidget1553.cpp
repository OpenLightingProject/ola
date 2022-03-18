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
#include <vector>

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
using std::vector;

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
  vector<uint8_t> data;

  while (m_socket->DataRemaining() > 0) {
    uint8_t byte = 0x00;
    unsigned int data_read;

    int ret = m_socket->Receive(&byte, 1, data_read);

    if (ret == -1 || data_read != 1) {
    } else {
      OLA_DEBUG << "Received byte 0x" << std::hex << static_cast<int>(byte);
      data.push_back(byte);
    }
  }

  OLA_DEBUG << "Received " << static_cast<int>(data.size()) << " bytes";
  switch (m_current_receive_state) {
    case LOAD:
      OLA_DEBUG << "Rx in load";
      break;
    case SET_CHANNEL_COUNT:
      OLA_DEBUG << "Rx in set chan count";
      if (data.size() == 1) {
        if (data[0] == MILINST_1553_END_BYTE) {
          OLA_DEBUG << "Set chan count successful";
        } else {
          OLA_DEBUG << "Received unexpected byte, got "
                    << static_cast<int>(data[0]) << " expecting "
                    << static_cast<int>(MILINST_1553_END_BYTE);
        }
      } else {
        OLA_DEBUG << "Received unexpected number of bytes, got "
                  << static_cast<int>(data.size()) << " expecting 1";
      }
      break;
    case GET_CHANNEL_COUNT:
      OLA_DEBUG << "Rx in get chan count";
      if (data.size() == 2) {
        uint16_t channels = ola::utils::JoinUInt8(data[1], data[0]);
        OLA_DEBUG << "Got channel count of " << channels << " channels";
        if (channels < m_channels) {
          OLA_DEBUG << "Config mismatch, device is configured for " << channels
                    << " channels, but config says " << m_channels
                    << " channels; reducing config to match";
          // Todo(Peter): Run this value through the set validator?
          m_preferences->SetValue(ChannelsKey(), channels);
          m_preferences->Save();
          m_channels = channels;

          // Todo(Peter): Work out what to do here, set the device to match the
          // config or the config to match the device?
        }
      } else {
        OLA_DEBUG << "Received unexpected number of bytes, got "
                  << static_cast<int>(data.size()) << " expecting 2";
      }
      break;
    default:
      OLA_WARN << "Unknown state " << static_cast<int>(m_current_receive_state);
  }
}


/*
 * Check if this is actually a MilInst device
 * @return true if this is a milinst,  false otherwise
 */
bool MilInstWidget1553::DetectDevice() {
  // TODO(Peter): Fixme, check channel count or something!
  uint8_t msg[1];
  msg[0] = MILINST_1553_GET_CHANNEL_COUNT_COMMAND;
  m_current_receive_state = GET_CHANNEL_COUNT;

//  uint8_t msg[2];
//  msg[0] = MILINST_1553_SET_CHANNEL_COUNT_COMMAND;
//  msg[1] = 1;
//  m_current_receive_state = SET_CHANNEL_COUNT;

  m_socket->Send(msg, sizeof(msg));

  return true;
}


/*
 * Send a DMX msg.
  */
bool MilInstWidget1553::SendDmx(const DmxBuffer &buffer) {
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
int MilInstWidget1553::SetChannel(unsigned int chan, uint8_t val) {
  uint8_t msg[4];

  msg[0] = MILINST_1553_LOAD_COMMAND;
  ola::utils::SplitUInt16(chan, &msg[1], &msg[2]);
  msg[3] = val;
  OLA_DEBUG << "Setting " << chan << " to " << static_cast<int>(val);
  m_current_receive_state = LOAD;
  return m_socket->Send(msg, sizeof(msg));
}


/*
 * Send data
 * @param buffer a DmxBuffer with the data
 */
int MilInstWidget1553::Send(const DmxBuffer &buffer) {
  unsigned int channels = std::min(static_cast<unsigned int>(m_channels),
                                   buffer.Size());
  uint8_t msg[3 + channels];

  msg[0] = MILINST_1553_LOAD_COMMAND;
  ola::utils::SplitUInt16(1, &msg[1], &msg[2]);

  buffer.Get(msg + 3, &channels);

  m_current_receive_state = LOAD;

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
