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
 * KarateLight.cpp
 * The KarateLight communication class
 * Copyright (C) 2013 Carsten Presser
 */

#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include <termios.h>
#include <iostream>
#include <string>

#include "ola/Logging.h"
#include "ola/DmxBuffer.h"
#include "plugins/karate/KarateLight.h"

namespace ola {
namespace plugin {
namespace karate {

/**
 * Default constructor
 * \param dev the filename of the device to use
 */
KarateLight::KarateLight(const string &dev)
  : m_devname(dev),
    m_fw_version(0),
    m_hw_version(0),
    m_nChannels(0),
    m_use_memcmp(1),
    m_active(false) {
}

/**
 * Default desctuctor
 * closes the device and does release the file-lock
 */
KarateLight::~KarateLight() {
  KarateLight::Close();
}

void KarateLight::Close() {
  // remove lock and close file
  flock(m_fd, LOCK_UN);
  tcflush(m_fd, TCIOFLUSH);
  close(m_fd);
  m_active = false;
}

/**
 * Initialize the device
 * 1. open the devicefile and get a file lock
 * 2. read defaults (firmware, hardware, channels count)
 * 3. set all channels to black
 */
bool KarateLight::Init() {
  int j, i;
  struct termios options;

  if (m_active)
    return false;

  m_fd = open(m_devname.c_str(), O_RDWR | O_NOCTTY);
  if (m_fd < 0) {
    OLA_FATAL << "failed to open " << m_devname;
    return false;
  }

  /* Clear the line */
  tcflush(m_fd, TCOFLUSH);

  memset(&options, 0, sizeof(options));

  cfsetispeed(&options, B115200);
  cfsetospeed(&options, B115200);

  // options.c_cflag = (CS8 | CSTOPB | CLOCAL | CREAD);
  options.c_cflag = (CS8 | CLOCAL | CREAD);

  // If MIN = 0 and TIME > 0, TIME serves as a timeout value. The read
  // will be satisfied if a single character is read, or TIME is
  // exceeded (t = TIME *0.1 s). If TIME is exceeded, no character will
  // be returned.
  options.c_cc[VTIME] = 1;
  options.c_cc[VMIN]  = 0;  // always require at least one byte returned

  // Update the options and do it NOW
  if (tcsetattr(m_fd, TCSANOW, &options) != 0) {
    OLA_FATAL << "tcsetattr failed on " << m_devname;
    return false;
  }

  // clear possible junk data still in the systems fifo
  m_bytesread = 1;
  while (m_bytesread > 0) {
    m_bytesread = read(m_fd, m_rd_buffer, 255);
  }

  // read firmware version
  m_byteswritten = write(m_fd, m_wr_buffer, \
           KarateLight::CreateCommand(CMD_GET_VERSION, NULL, 0));
  if (m_byteswritten != CMD_DATA_START) {
    OLA_WARN << "failed to write data to " << m_devname;
    return false;
  }

  if (KarateLight::ReadBack() == 1) {  // we are expecting one byte
    m_fw_version = m_rd_buffer[CMD_DATA_START];
  } else {
    OLA_FATAL << "failed to read the firmware-version ";
    return false;
  }

  // if an older Firware-Version is used. quit. the communication wont work
  if (m_fw_version < 0x30) {
    OLA_FATAL << "Firmware 0x" << static_cast<int>(m_fw_version) \
              << "is to old!";
    return false;
  }

  // read HW version
  m_byteswritten = write(m_fd, m_wr_buffer, \
                    KarateLight::CreateCommand(CMD_GET_HARDWARE, NULL, 0));
  if (m_byteswritten != CMD_DATA_START) {
    return false;
  }

  if (KarateLight::ReadBack() == 1) {  // we are expecting one byte
    m_hw_version = m_rd_buffer[CMD_DATA_START];
  } else {
    return false;
  }

  // read number of channels
  m_byteswritten = write(m_fd, m_wr_buffer, \
                    KarateLight::CreateCommand(CMD_GET_N_CHANNELS, NULL, 0));
  if (m_byteswritten != CMD_DATA_START) {
    return false;
  }

  if (KarateLight::ReadBack() == 2) {  // we are expecting two bytes
    m_nChannels = m_rd_buffer[CMD_DATA_START] \
                + m_rd_buffer[CMD_DATA_START+1] * 256;
  } else {
    return false;
  }

  // stuff specific for the KarateLight8/16
  if (m_hw_version == HW_ID_KARATE) {
    // disable memcmp for the classic KarateLight Hardware
    m_use_memcmp = 0;

    j = KarateLight::ReadEeprom(0x03);
    i = KarateLight::ReadEeprom(0x02);
    if ((j >= 0) && (i >= 0)) {
      m_dmx_offset = j*256;
      m_dmx_offset += i;
    } else {
      OLA_WARN << "Error Reading EEPROM";
      return false;
    }

    if (m_dmx_offset > 511) {
      OLA_WARN << "DMX Offset to large" << std::dec \
               << m_dmx_offset << ". Setting it to 0";
      m_dmx_offset = 0;
    }

  } else {
    // KL-DMX-Device
    m_dmx_offset = 0;
  }

  m_active = true;

  OLA_INFO << "successfully initalized device " << m_devname \
           << " with firmware revision 0x" \
           << std::hex << static_cast<int>(m_fw_version) \
           << ", hardware-id = 0x" \
           << std::hex << static_cast<int>(m_hw_version) \
           << ", channel_count = " << std::dec << m_nChannels \
           << ", dmx_offset = " << m_dmx_offset;

  // set channels to zero
  return KarateLight::Blank();
}

/**
 * Sets all Channels to black and sends data to the device
 * \returns true on success
 */
bool KarateLight::Blank() {
  // set channels to black
  memset(m_color_buffer, 0, MAX_CHANNELS);
  memset(m_color_buffer_old, 1, MAX_CHANNELS);

  return KarateLight::UpdateColors();
}

/**
 * copy contents of the DmxBuffer into my local scope
 * \returns true on success
 */
bool KarateLight::SetColors(const DmxBuffer &da) {
  unsigned int length = da.Size();

  if ((length + m_dmx_offset) > MAX_CHANNELS) {
    length = MAX_CHANNELS - m_dmx_offset;
  }
  da.GetRange(m_dmx_offset, m_color_buffer, &length);
  return KarateLight::UpdateColors();
}

/**
 * Sends color values previously set via
 * \sa SetColor
 * \returns true on success
 */
bool KarateLight::UpdateColors() {
  int block;
  int n_chunks;

  if (!m_active)
    return false;

  n_chunks = (m_nChannels + CHUNK_SIZE - 1)/CHUNK_SIZE;

  // write colors
  for (block = 0; block < n_chunks; block++) {
    if ((memcmp(&m_color_buffer[block*CHUNK_SIZE], \
         &m_color_buffer_old[block*CHUNK_SIZE], CHUNK_SIZE) == 0) \
         && (m_use_memcmp == 1)) {
      continue;
    }
    m_byteswritten = write(m_fd, m_wr_buffer, \
                      KarateLight::CreateCommand(block+CMD_SET_DATA_00, \
                      &m_color_buffer[block*CHUNK_SIZE], \
                      CHUNK_SIZE));
    if (m_byteswritten != (CMD_DATA_START + CHUNK_SIZE)) {
      OLA_WARN << "failed to write data to " << m_devname;
      KarateLight::Close();
      return false;
    }
    if (KarateLight::ReadBack() != 0) {  // we expect a return with 0 bytes
      KarateLight::Close();
      return false;
    }
  }
  // update old_values
  memcpy(&m_color_buffer_old[0], &m_color_buffer[0], MAX_CHANNELS);
  return true;
}

/*
 PRIVATE FUNCTIONS
*/

/**
 * Perform a checksum calculation of the command currently in the buffer
 * The Checksum is a simple XOR over all bytes send, except the chechsum-byte.
 * \param the lenght of the command to be processed
 * \returns the lengh of the command
 */
int KarateLight::CalcChecksum(uint8_t len) {
  int i;

  m_wr_buffer[CMD_HD_CHECK] = 0;  // clear byte 2

  for (i = 0; i < len; i++) {
    if (i != CMD_HD_CHECK) {
      m_wr_buffer[CMD_HD_CHECK]^= m_wr_buffer[i];
    }
  }

  return len;
}

/**
 * Creates and stores a command in the command buffer
 * \param cmd the commandcode to be used
 * \param data a pointer to a buffer containing data to be included in the command
 * \param len number of bytes to be read from data
 * \returns the lengh of the command, negative values indicate an error
 */
int KarateLight::CreateCommand(uint8_t cmd, uint8_t * data, uint8_t len) {
  int i = 0;

  // maximum command lenght
  if (len > (CMD_MAX_LENGTH - CMD_DATA_START)) {
    OLA_WARN << "Error: Command is to long (" << std::dec << len << " > "\
             << (CMD_MAX_LENGTH - CMD_DATA_START);
    return -1;
  }

  // build header
  m_wr_buffer[CMD_HD_SYNC] = CMD_SYNC_SEND;
  m_wr_buffer[CMD_HD_COMMAND] = cmd;
  m_wr_buffer[CMD_HD_LEN] = len;

  // copy data
  while (len > i) {
    m_wr_buffer[CMD_DATA_START+i] = data[i];
    i++;
  }

  // calc checksum
  i =  KarateLight::CalcChecksum(CMD_DATA_START+len);

  return i;
}

/**
 * Tries to read an answer from the device
 * \return the number of bytes read, or an error value <0
 */
int KarateLight::ReadBack() {
  // read header (4 bytes)
  m_bytesread = read(m_fd, m_rd_buffer, CMD_DATA_START);

  if (m_bytesread != CMD_DATA_START) {
    OLA_FATAL << "could not read 4 bytes (header) from " << m_devname;
    KarateLight::Close();
    return -1;
  }

  // read sequential
  if (m_rd_buffer[CMD_HD_LEN] > 0) {
    m_bytesread = read(m_fd, &m_rd_buffer[CMD_DATA_START], \
                   m_rd_buffer[CMD_HD_LEN]);
  } else {
    m_bytesread = 0;
  }

  if (m_bytesread == m_rd_buffer[CMD_HD_LEN]) {
    // TODO(cpresser) verify checksum
    return (m_bytesread);
  } else {
    OLA_WARN << "number of bytes read" << m_bytesread \
             << "does not match number of bytes expected" \
             << static_cast<int>(m_rd_buffer[CMD_HD_LEN]);
    KarateLight::Close();
    return -1;
  }
}

/**
 * Reads the a single byte from the eeprom
 * \parm addr the eeprom location
 * \return the eeprom byte, or a return-code <0 in case there is an error
 */
int KarateLight::ReadEeprom(uint8_t addr) {
  unsigned int j;

  if (!m_active)
    return -1;

  m_byteswritten = write(m_fd, m_wr_buffer, \
                    KarateLight::CreateCommand(CMD_READ_EEPROM, &addr, 1));
  if (m_byteswritten != CMD_DATA_START+1) {
    OLA_WARN << "failed to write data to " << m_devname;
    KarateLight::Close();
    return -1;
  }

  j = KarateLight::ReadBack();
  if (j == 1) {
    return m_rd_buffer[CMD_DATA_START];
  }
  return j;  // if j!=1 byte
}
}  // namespace karate
}  // namespace plugin
}  // namespace ola
