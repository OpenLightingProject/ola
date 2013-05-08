/*
# Copyright (C) 2012 Carsten Presser <c@rstenpresser.de>
#
# This file is part of karate_tools, a set of tools to control an debug
# the KarateLight hardware available at http://karatelight.de
#
# karate_tools is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of the
# License, or (at your option) any later version.
#
# karate_tools is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with karate_tools; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA
#
 */
#ifndef PLUGINS_KARATE_KL_H_
#define PLUGINS_KARATE_KL_H_

#ifndef WIN32
#include <stdint.h>
#include <unistd.h>
#else
#define uint8_t unsigned char
#endif

#define LIBKARATE_VERSION  "20130227"
#define MAX_CHANNELS       512
#define CMD_MAX_LENGTH     64
#define MAX_ERROR_LENGTH   128

class KarateLight {
  public:
    explicit KarateLight(char * device);
    ~KarateLight();
    int Init();

    int SetColor(uint16_t chan, uint8_t val);
    int SetColors(uint8_t* buffer, int length);

    int Blank();
    int UpdateColors();

    unsigned int GetnChannels() {
        return nChannels;
    };
    unsigned int GetFWVersion() {
        return fw_version;
    };
    unsigned int GetHWVersion() {
        return hw_version;
    };
    unsigned int GetDMXOffset() {
        return dmx_offset;
    };
    int GetPWMValue();
    int GetLDRValue();

    int ReadEeprom(uint8_t addr);
    int WriteEeprom(uint8_t addr, uint8_t data);

    int EnterBoot();
    int SetPWMValue(unsigned int val);

    char* GetLastError() {
        return errortext;
    }
    bool IsActive() {
        return active;
    };

  private:
    int CreateCommand(int cmd, uint8_t * data, int len);
    int CalcChecksum(int len);
    int ReadBack();

    char * devname;
#ifdef WIN32
    HANDLE fd;
    DWORD bytesread;
    DWORD byteswritten;
#else
    int fd;
    int bytesread;
    int byteswritten;
#endif

    unsigned int fw_version;
    unsigned int hw_version;
    unsigned int nChannels;
    unsigned int dmx_offset;

    uint8_t wr_buffer[CMD_MAX_LENGTH];
    uint8_t rd_buffer[CMD_MAX_LENGTH];

    unsigned int pwm_value;
    unsigned int ldr_value;

    uint8_t color_buffer[MAX_CHANNELS];
    uint8_t color_buffer_old[MAX_CHANNELS];
    uint8_t use_memcmp;

    bool active;
    char errortext[MAX_ERROR_LENGTH];
};

#define KL_OK           0
#define KL_ERROR        -1
#define KL_CHECKSUMFAIL -2
#define KL_WRITEFAIL    -3
#define KL_NOTACTIVE    -4


// address
#define CMD_HD_SYNC     0x00
#define CMD_HD_COMMAND  0x01
#define CMD_HD_CHECK    0x02
#define CMD_HD_LEN      0x03
#define CMD_DATA_START  0x04

// sync words
#define CMD_SYNC_SEND       0xAA
#define CMD_SYNC_RECV       0x55

// status
#define CMD_SYS_SYNC        0x00
#define CMD_SYS_ACK         0x01
#define CMD_SYS_NACK        0x02
#define CMD_SYS_NIMP        0xFF
#define CMD_SYS_IR          0x10
#define CMD_SYS_DATA        0x20
#define CMD_SYS_NACK_LENGTH 0x03
#define CMD_SYS_NACK_CHECK  0x04

// commands
#define CMD_SYS_SYNC        0x00
#define CMD_SYS_ACK         0x01
#define CMD_SYS_NACK        0x02
#define CMD_SYS_NIMP        0xFF
#define CMD_SYS_IR          0x10
#define CMD_SYS_DATA        0x20
#define CMD_SYS_NACK_LENGTH 0x03
#define CMD_SYS_NACK_CHECK  0x04

#define CMD_GET_VERSION     0x01
#define CMD_GET_HARDWARE    0x02

#define CMD_GET_TLC_PWM_VALUE   0x14
#define CMD_SET_TLC_PWM_VALUE   0x15


#define CMD_SET_DATA_00         0x20
#define CMD_SET_DATA_01         0x21
#define CMD_SET_DATA_02         0x22
#define CMD_SET_DATA_03         0x23
#define CMD_SET_DATA_04         0x24
#define CMD_SET_DATA_05         0x25
#define CMD_SET_DATA_06         0x26
#define CMD_SET_DATA_07         0x27
#define CMD_SET_DATA_08         0x28
#define CMD_SET_DATA_09         0x29
#define CMD_SET_DATA_10         0x2A
#define CMD_SET_DATA_11         0x2B
#define CMD_SET_DATA_12         0x2C
#define CMD_SET_DATA_13         0x2D
#define CMD_SET_DATA_14         0x2E
#define CMD_SET_DATA_15         0x2F


#define CMD_GET_N_CHANNELS      0x30

#define CMD_READ_ADC0       0x40

#define CMD_READ_EEPROM     0x50
#define CMD_WRITE_EEPROM    0x51

#define CMD_BOOT_REQUEST    0x80
#define CMD_BOOT_START      0x81



#define HW_ID_KARATE        0x01
#define HW_ID_USB2DMX       0x02


#endif  // PLUGINS_KARATE_KL_H_
