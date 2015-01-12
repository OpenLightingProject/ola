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
#include <unistd.h>
#include <algorithm>
#include <iostream>
#include <string>

#include "ola/Constants.h"
#include "ola/DmxBuffer.h"
#include "ola/Logging.h"
#include "ola/io/IOUtils.h"
#include "ola/strings/Format.h"
#include "plugins/karate/KarateLight.h"

namespace ola {
namespace plugin {
namespace karate {

using std::string;

/**
 * @brief Default constructor
 * @param dev the filename of the device to use
 */
KarateLight::KarateLight(const string &dev)
    : m_devname(dev),
      m_fd(-1),
      m_fw_version(0),
      m_hw_version(0),
      m_nChannels(0),
      m_dmx_offset(0),
      m_use_memcmp(1),
      m_active(false) {
}

/**
 * @brief Default destructor
 *
 * Closes the device and does release the file-lock
 */
KarateLight::~KarateLight() {
  KarateLight::Close();
}

void KarateLight::Close() {
  // remove lock and close file
  if (m_fd >= 0) {
    flock(m_fd, LOCK_UN);
    tcflush(m_fd, TCIOFLUSH);
    close(m_fd);
  }
  m_active = false;
}

/**
 * @brief Initialize the device
 *
 * 1. Open the device file and get a file lock
 * 2. Read defaults (firmware, hardware, channels count)
 * 3. Set all channels to black
 */
bool KarateLight::Init() {
  uint8_t rd_buffer[CMD_MAX_LENGTH];
  struct termios options;

  if (m_active)
    return false;

  if (!ola::io::Open(m_devname, O_RDWR | O_NOCTTY, &m_fd)) {
    return false;
  }

  /* Clear the line */
  tcflush(m_fd, TCOFLUSH);

  memset(&options, 0, sizeof(options));

  cfsetispeed(&options, B115200);
  cfsetospeed(&options, B115200);

  options.c_cflag = CS8 | CLOCAL | CREAD;

  // If MIN = 0 and TIME > 0, TIME serves as a timeout value. The read
  // will be satisfied if a single character is read, or TIME is
  // exceeded (t = TIME *0.1 s). If TIME is exceeded, no character will
  // be returned.
  options.c_cc[VTIME] = 1;
  options.c_cc[VMIN]  = 0;  // always require at least one byte returned

  // Update the options and do it NOW
  if (tcsetattr(m_fd, TCSANOW, &options) != 0) {
    OLA_WARN << "tcsetattr failed on " << m_devname;
    return false;
  }

  // Try to get a lock on the device, making access exclusive
  if (flock(m_fd, LOCK_EX | LOCK_NB) != 0) {
    OLA_WARN << "Error getting a lock on " << m_devname
             << "Maybe a other programm is accessing the device."
             << "Errorcode: " << strerror(errno);
    return false;
  }

  // clear possible junk data still in the systems fifo
  int bytes_read = 1;
  while (bytes_read > 0) {
    bytes_read = read(m_fd, rd_buffer, CMD_MAX_LENGTH);
  }

  // read firmware version
  if (SendCommand(CMD_GET_VERSION, NULL, 0, rd_buffer, 1)) {
    m_fw_version = rd_buffer[0];
  } else {
    OLA_WARN << "failed to read the firmware-version.";
    return false;
  }

  // if an older Firware-Version is used. quit. the communication wont work
  if (m_fw_version < 0x33) {
    OLA_WARN << "Firmware 0x" << static_cast<int>(m_fw_version) \
              << "is to old!";
    return false;
  }

  // read HW version
  if (SendCommand(CMD_GET_HARDWARE, NULL, 0, rd_buffer, 1)) {
    m_hw_version = rd_buffer[0];
  } else {
    OLA_WARN << "failed to read the hardware-revision.";
    return false;
  }

  // read number of channels
  if (SendCommand(CMD_GET_N_CHANNELS, NULL, 0, rd_buffer, 2)) {
    m_nChannels = (rd_buffer[1] << 8) + rd_buffer[0];
  } else {
    return false;
  }

  m_active = true;

  // stuff specific for the KarateLight8/16
  if (m_hw_version == HW_ID_KARATE) {
    // disable memcmp for the classic KarateLight Hardware
    m_use_memcmp = 0;

    // read the dmx_offset from eeprom
    uint8_t upper, lower;
    if (ReadByteFromEeprom(3, &upper) && ReadByteFromEeprom(2, &lower)) {
      m_dmx_offset = (upper << 8) + lower;
    } else {
      OLA_WARN << "Error Reading EEPROM";
      m_active = false;
      return false;
    }

    if (m_dmx_offset > 511) {
      OLA_WARN << "DMX Offset to large" << std::dec
               << m_dmx_offset << ". Setting it to 0";
      m_dmx_offset = 0;
    }
  } else {
    // KL-DMX-Device
    m_dmx_offset = 0;
  }

  OLA_INFO << "successfully initalized device " << m_devname
           << " with firmware version 0x"
           << strings::ToHex(m_fw_version)
           << ", hardware-revision = 0x"
           << strings::ToHex(m_hw_version)
           << ", channel_count = " << m_nChannels
           << ", dmx_offset = " << m_dmx_offset;

  // set channels to black
  return KarateLight::Blank();
}

/**
 * @brief Sets all Channels to black and sends data to the device
 * @returns true on success
 */
bool KarateLight::Blank() {
  memset(m_color_buffer, 0, DMX_UNIVERSE_SIZE);
  memset(m_color_buffer_old, 1, DMX_UNIVERSE_SIZE);
  return KarateLight::UpdateColors();
}

/**
 * @brief Copy contents of the DmxBuffer into my local scope
 * @returns true on success
 */
bool KarateLight::SetColors(const DmxBuffer &da) {
  // make sure not to request data beyond the bounds of the universe
  unsigned int length = std::min(static_cast<int>(da.Size()),
                                 DMX_UNIVERSE_SIZE - m_dmx_offset);

  da.GetRange(m_dmx_offset, m_color_buffer, &length);
  return KarateLight::UpdateColors();
}

/*
 PRIVATE FUNCTIONS
*/

/**
 * @brief Tries to read an answer from the device
 * @param rd_data buffer for the received data (excluding the header)
 * @param rd_len number of bytes to read (excluding the header), will be
 *     overwritten with the number of bytes received in the case
 *     of a mismatch
 * @return true on success
 */
bool KarateLight::ReadBack(uint8_t *rd_data, uint8_t *rd_len) {
  uint8_t rd_buffer[CMD_MAX_LENGTH];

  // read header (4 bytes)
  int bytes_read = read(m_fd, rd_buffer, CMD_DATA_START);
  if (bytes_read != CMD_DATA_START) {
    if (errno != EINTR) {  // this is also true for EAGAIN
      OLA_WARN << "Could not read 4 bytes (header) from " << m_devname
               << " ErrorCode: " << strerror(errno);
      KarateLight::Close();
      return false;
    }
  }
  bytes_read = 0;

  // read payload-data (if there is any)
  uint8_t payload_size = rd_buffer[CMD_HD_LEN];
  if (payload_size > CMD_MAX_LENGTH - CMD_DATA_START) {
    OLA_WARN << "KarateLight returned " << static_cast<int>(payload_size)
             << " bytes of data, this exceeds our buffer size";
    return false;
  }

  if (payload_size > 0u) {
    // we won't enter this loop if there are no bytes to receive
    bytes_read = read(m_fd, &rd_buffer[CMD_DATA_START], payload_size);
    if (bytes_read != payload_size) {
      if (errno != EINTR) {  // this is also true for EAGAIN (timeout)
        OLA_WARN << "Reading > " << static_cast<int>(payload_size)
                 << " < bytes payload from " << m_devname
                 << " ErrorCode: " << strerror(errno);
        KarateLight::Close();
        return false;
      }
    }
  }

  // verify data-length
  if (*rd_len != payload_size) {
    OLA_WARN << "Number of bytes read > " << bytes_read
             << " < does not match number of bytes expected > "
             << static_cast<int>(payload_size)
             << " <";
    KarateLight::Close();
    return false;
  }

  // verify checksum
  int checksum = 0;
  for (int i = 0; i < bytes_read + CMD_DATA_START; i++) {
    if (i != CMD_HD_CHECK) {
      checksum ^= rd_buffer[i];
    }
  }
  if (checksum != rd_buffer[CMD_HD_CHECK]) {
    OLA_WARN << "Checkum verification of incoming data failed. "
             << "Data-checkum is: " << strings::ToHex(checksum)
             << " but the device said it would be 0x"
             << static_cast<int>(rd_buffer[CMD_HD_CHECK]);
    KarateLight::Close();

    return false;
  }

  // prepare data
  *rd_len = static_cast<uint8_t>(bytes_read);
  memcpy(rd_data, &rd_buffer[CMD_DATA_START], *rd_len);

  return true;
}  // end of KarateLight::ReadBack

/**
 * @brief Reads the a single byte from the eeprom
 * @param addr the eeprom address to read from (0..255)
 * @param data location to store the received byte to
 * @return true on success
 */
bool KarateLight::ReadByteFromEeprom(uint8_t addr , uint8_t *data) {
  uint8_t rd_buffer[CMD_MAX_LENGTH];

  if (!m_active)
    return false;

  if (!SendCommand(CMD_READ_EEPROM, &addr, 1, rd_buffer, 1)) {
    return false;
  }

  *data = rd_buffer[0];
  return true;
}

/**
 * @brief Creates and Command, sends it, reads the reply
 *
 * Will return false in case the number of bytes received does not match
 * the number of bytes expected.
 *
 * @param cmd the commandcode to be used
 * @param output_buffer buffer containing payload-data to be send
 * @param n_bytes_to_write number of bytes to be written
 * @param input_buffer returned payload data will be stored here
 * @param n_bytes_expected number of bytes expected (excluding the header)
 * @returns true on success
 */
bool KarateLight::SendCommand(uint8_t cmd, const uint8_t *output_buffer,
                              int n_bytes_to_write, uint8_t *input_buffer,
                              int n_bytes_expected) {
  uint8_t wr_buffer[CMD_MAX_LENGTH];
  uint8_t n_bytes_read;

  // maximum command length
  uint8_t cmd_length = n_bytes_to_write + CMD_DATA_START;
  if (cmd_length > CMD_MAX_LENGTH) {
    OLA_WARN << "Error: Command is to long (" << std::dec
             << n_bytes_to_write << " > " << (CMD_MAX_LENGTH - CMD_DATA_START);
    return false;
  }

  // build header
  wr_buffer[CMD_HD_SYNC] = CMD_SYNC_SEND;
  wr_buffer[CMD_HD_COMMAND] = cmd;
  wr_buffer[CMD_HD_LEN] = n_bytes_to_write;

  // copy data to our local buffer
  memcpy(&wr_buffer[CMD_DATA_START], output_buffer, n_bytes_to_write);

  // calc checksum
  wr_buffer[CMD_HD_CHECK] = 0;  // clear byte 2
  for (int i = 0; i < cmd_length; i++) {
    if (i != CMD_HD_CHECK) {
      wr_buffer[CMD_HD_CHECK] ^= wr_buffer[i];
    }
  }

  // now write to the serial port
  if (write(m_fd, wr_buffer, cmd_length) != cmd_length) {
    OLA_WARN << "Failed to write data to " << m_devname;
    KarateLight::Close();
    return false;
  }

  // read the answer, check if we got the number of bytes we expected
  n_bytes_read = n_bytes_expected;
  if (!ReadBack(input_buffer, &n_bytes_read)
      || (n_bytes_read != n_bytes_expected)) {
    KarateLight::Close();
    return false;
  }

  return true;
}

/**
 * @brief Sends color values currently stored in the local buffer
 * to the hardware.
 * @returns true on success
 */
bool KarateLight::UpdateColors() {
  int block;
  int n_chunks;

  if (!m_active)
    return false;

  n_chunks = (m_nChannels + CHUNK_SIZE - 1) / CHUNK_SIZE;

  // write colors
  for (block = 0; block < n_chunks; block++) {
    if ((memcmp(&m_color_buffer[block * CHUNK_SIZE],
         &m_color_buffer_old[block * CHUNK_SIZE], CHUNK_SIZE) == 0)
         && (m_use_memcmp == 1)) {
      continue;
    }
    if (!SendCommand(CMD_SET_DATA_00 + block,  // cmd-code
                     &m_color_buffer[block * CHUNK_SIZE],  // data to write
                     CHUNK_SIZE,  // len of data
                     NULL,  // buffer for incoming data
                     0)) {  // number of data-bytes expected
      KarateLight::Close();
      return false;
    }
  }
  // update old_values
  memcpy(m_color_buffer_old, m_color_buffer, DMX_UNIVERSE_SIZE);
  return true;
}  // end of UpdateColors()
}  // namespace karate
}  // namespace plugin
}  // namespace ola
