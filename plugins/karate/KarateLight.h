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
 * KarateLight.h
 * The KarateLight communication class
 * Copyright (C) 2013 Carsten Presser
 */
#ifndef PLUGINS_KARATE_KARATELIGHT_H_
#define PLUGINS_KARATE_KARATELIGHT_H_

#include <stdint.h>
#include <string>

#include "ola/BaseTypes.h"
#include "ola/DmxBuffer.h"

using std::string;

namespace ola {
namespace plugin {
namespace karate {

class KarateLight {
 public:
  explicit KarateLight(const string &dev);
  ~KarateLight();
  bool Init();
  void Close();

  bool Blank();
  bool SetColors(const DmxBuffer &da);

  uint16_t GetnChannels() const { return m_nChannels; }
  uint8_t GetFWVersion() const { return m_fw_version; }
  uint8_t GetHWVersion() const { return m_hw_version; }
  uint16_t GetDMXOffset() const { return m_dmx_offset; }
  bool IsActive() const { return m_active; }

 private:
  bool ReadBack(uint8_t *rd_data, uint8_t *rd_len);
  bool ReadByteFromEeprom(uint8_t addr, uint8_t *data);
  bool SendCommand(uint8_t cmd, const uint8_t *output_buffer,
                   int n_bytes_to_write, uint8_t *input_buffer,
                   int n_bytes_expected);
  bool UpdateColors();

  const string m_devname;
  int m_fd;

  static const uint16_t CMD_MAX_LENGTH = 64;
  static const uint16_t CHUNK_SIZE = 32;

  uint8_t m_fw_version;
  uint8_t m_hw_version;
  uint16_t m_nChannels;
  uint16_t m_dmx_offset;

  uint8_t m_color_buffer[DMX_UNIVERSE_SIZE];
  uint8_t m_color_buffer_old[DMX_UNIVERSE_SIZE];
  uint8_t m_use_memcmp;

  bool m_active;

  // address
  static const uint8_t CMD_HD_SYNC = 0x00;
  static const uint8_t CMD_HD_COMMAND = 0x01;
  static const uint8_t CMD_HD_CHECK = 0x02;
  static const uint8_t CMD_HD_LEN = 0x03;
  static const uint8_t CMD_DATA_START = 0x04;

  // sync words
  static const uint8_t CMD_SYNC_SEND = 0xAA;
  static const uint8_t CMD_SYNC_RECV = 0x55;

  // status
  static const uint8_t CMD_SYS_ACK = 0x01;
  static const uint8_t CMD_SYS_NACK = 0x02;
  static const uint8_t CMD_SYS_NIMP = 0xFF;
  static const uint8_t CMD_SYS_IR = 0x10;
  static const uint8_t CMD_SYS_DATA = 0x20;
  static const uint8_t CMD_SYS_NACK_LENGTH = 0x03;
  static const uint8_t CMD_SYS_NACK_CHECK = 0x04;

  // commands
  static const uint8_t CMD_GET_VERSION = 0x01;
  static const uint8_t CMD_GET_HARDWARE = 0x02;

  static const uint8_t CMD_GET_TLC_PWM_VALUE = 0x14;
  static const uint8_t CMD_SET_TLC_PWM_VALUE = 0x15;

  static const uint8_t CMD_SET_DATA_00 = 0x20;
  static const uint8_t CMD_SET_DATA_01 = 0x21;
  static const uint8_t CMD_SET_DATA_02 = 0x22;
  static const uint8_t CMD_SET_DATA_03 = 0x23;
  static const uint8_t CMD_SET_DATA_04 = 0x24;
  static const uint8_t CMD_SET_DATA_05 = 0x25;
  static const uint8_t CMD_SET_DATA_06 = 0x26;
  static const uint8_t CMD_SET_DATA_07 = 0x27;
  static const uint8_t CMD_SET_DATA_08 = 0x28;
  static const uint8_t CMD_SET_DATA_09 = 0x29;
  static const uint8_t CMD_SET_DATA_0A = 0x2A;
  static const uint8_t CMD_SET_DATA_0B = 0x2B;
  static const uint8_t CMD_SET_DATA_0C = 0x2C;
  static const uint8_t CMD_SET_DATA_0D = 0x2D;
  static const uint8_t CMD_SET_DATA_0E = 0x2E;
  static const uint8_t CMD_SET_DATA_0F = 0x2F;

  static const uint8_t CMD_GET_N_CHANNELS = 0x30;

  static const uint8_t CMD_READ_ADC0 = 0x40;

  static const uint8_t CMD_READ_EEPROM = 0x50;
  static const uint8_t CMD_WRITE_EEPROM = 0x51;

  static const uint8_t CMD_BOOT_REQUEST = 0x80;
  static const uint8_t CMD_BOOT_START = 0x81;

  static const uint8_t HW_ID_KARATE = 0x01;
  static const uint8_t HW_ID_USB2DMX = 0x02;
};
}  // namespace karate
}  // namespace plugin
}  // namespace ola

#endif  // PLUGINS_KARATE_KARATELIGHT_H_
