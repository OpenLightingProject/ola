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
 * KarateLight.h
 * The KarateLight communication class
 * Copyright (C) 2013 Carsten Presser
 */
#ifndef PLUGINS_KARATE_KARATELIGHT_H_
#define PLUGINS_KARATE_KARATELIGHT_H_
class KarateLight {
  public:
    explicit KarateLight(const string dev);
    ~KarateLight();
    int Init();

    int SetColors(uint8_t* buffer, int length);

    int Blank();
    int UpdateColors();

    uint16_t GetnChannels() {
        return nChannels;
    };
    uint8_t GetFWVersion() {
        return fw_version;
    };
    uint8_t GetHWVersion() {
        return hw_version;
    };
    uint16_t GetDMXOffset() {
        return dmx_offset;
    };

    int ReadEeprom(uint8_t addr);

    string GetLastError() {
        return errortext;
    }
    bool IsActive() {
        return active;
    };

  private:
    int CreateCommand(int cmd, uint8_t * data, int len);
    int CalcChecksum(int len);
    int ReadBack();

    const string m_devname;
    int m_fd;
    int m_bytesread;
    int m_byteswritten;

    unsigned uint8_t m_fw_version;
    unsigned uint8_t m_hw_version;
    unsigned uint16_t m_nChannels;
    unsigned uint16_t m_dmx_offset;

    uint8_t m_wr_buffer[CMD_MAX_LENGTH];
    uint8_t m_rd_buffer[CMD_MAX_LENGTH];

    unsigned uint16_t m_pwm_value;
    unsigned uint16_t m_ldr_value;

    uint8_t m_color_buffer[MAX_CHANNELS];
    uint8_t m_color_buffer_old[MAX_CHANNELS];
    uint8_t m_use_memcmp;

    bool m_active;
    string errortext;

    // basically defines
    const uint16_t CMD_MAX_LENGTH = 64;
    const uint16_t CHUNK_SIZE = 32;
    const uint16_t MAX_CHANNELS = 512;

    // communication stuff
    const unsigned char KL_OK = 0;
    const unsigned char KL_ERROR = -1;
    const unsigned char KL_CHECKSUMFAIL = -2;
    const unsigned char KL_WRITEFAIL = -3;
    const unsigned char KL_NOTACTIVE = -4;

    // address
    const uint8_t CMD_HD_SYNC = 0x00;
    const uint8_t CMD_HD_COMMAND = 0x01;
    const uint8_t CMD_HD_CHECK = 0x02;
    const uint8_t CMD_HD_LEN = 0x03;
    const uint8_t CMD_DATA_START = 0x04;

    // sync words
    const uint8_t CMD_SYNC_SEND = 0xAA;
    const uint8_t CMD_SYNC_RECV = 0x55;

    // status
    const uint8_t CMD_SYS_SYNC = 0x00;
    const uint8_t CMD_SYS_ACK = 0x01;
    const uint8_t CMD_SYS_NACK = 0x02;
    const uint8_t CMD_SYS_NIMP = 0xFF;
    const uint8_t CMD_SYS_IR = 0x10;
    const uint8_t CMD_SYS_DATA = 0x20;
    const uint8_t CMD_SYS_NACK_LENGTH = 0x03;
    const uint8_t CMD_SYS_NACK_CHECK = 0x04;

    // commands
    const uint8_t CMD_SYS_SYNC = 0x00;
    const uint8_t CMD_SYS_ACK = 0x01;
    const uint8_t CMD_SYS_NACK = 0x02;
    const uint8_t CMD_SYS_NIMP = 0xFF;
    const uint8_t CMD_SYS_IR = 0x10;
    const uint8_t CMD_SYS_DATA = 0x20;
    const uint8_t CMD_SYS_NACK_LENGTH = 0x03;
    const uint8_t CMD_SYS_NACK_CHECK = 0x04;

    const uint8_t CMD_GET_VERSION = 0x01;
    const uint8_t CMD_GET_HARDWARE = 0x02;

    const uint8_t CMD_GET_TLC_PWM_VALUE = 0x14;
    const uint8_t CMD_SET_TLC_PWM_VALUE = 0x15;

    const uint8_t CMD_SET_DATA_00 = 0x20;
    const uint8_t CMD_SET_DATA_01 = 0x21;
    const uint8_t CMD_SET_DATA_02 = 0x22;
    const uint8_t CMD_SET_DATA_03 = 0x23;
    const uint8_t CMD_SET_DATA_04 = 0x24;
    const uint8_t CMD_SET_DATA_05 = 0x25;
    const uint8_t CMD_SET_DATA_06 = 0x26;
    const uint8_t CMD_SET_DATA_07 = 0x27;
    const uint8_t CMD_SET_DATA_08 = 0x28;
    const uint8_t CMD_SET_DATA_09 = 0x29;
    const uint8_t CMD_SET_DATA_0A = 0x2A;
    const uint8_t CMD_SET_DATA_0B = 0x2B;
    const uint8_t CMD_SET_DATA_0C = 0x2C;
    const uint8_t CMD_SET_DATA_0D = 0x2D;
    const uint8_t CMD_SET_DATA_0E = 0x2E;
    const uint8_t CMD_SET_DATA_0F = 0x2F;

    const uint8_t CMD_GET_N_CHANNELS = 0x30;

    const uint8_t CMD_READ_ADC0 = 0x40;

    const uint8_t CMD_READ_EEPROM = 0x50;
    const uint8_t CMD_WRITE_EEPROM = 0x51;

    const uint8_t CMD_BOOT_REQUEST = 0x80;
    const uint8_t CMD_BOOT_START = 0x81;

    const uint8_t HW_ID_KARATE = 0x01;
    const uint8_t HW_ID_USB2DMX = 0x02;
};

#endif  // PLUGINS_KARATE_KARATELIGHT_H_
