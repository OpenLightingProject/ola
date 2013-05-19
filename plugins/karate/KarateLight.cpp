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
#include <stdio.h>
#include <string.h>


#include <termios.h>
#include <fcntl.h>
#include <sys/file.h>
#include <errno.h>
#include <math.h>

#include "plugins/karate/KarateLight.h"


/**
 * Default constructor
 * \param dev the filename of the device to use
 */
KarateLight::KarateLight(const string &dev) : 
    m_devname(dev),
    m_fw_version(0), 
    m_hw_version(0), 
    m_nChannels(0), 
    m_use_memcmp(1), 
    m_active(false),
    m_errortext("not yet initalized") {
}

/**
 * Default desctuctor
 * closes the device and does release the file-lock
 */
KarateLight::~KarateLight() {
    // remove lock and close file
    flock(m_fd, LOCK_UN);
    tcflush(m_fd, TCIOFLUSH);
    close(m_fd);
}



/**
 * Sends color values previously set via
 * \sa SetColor
 * \returns KL_OK on success, a negative error value otherwise
 */
int KarateLight::UpdateColors() {
    int j;
    int chunks;

    if (!m_active)
        return KL_NOTACTIVE;

    chunks = (m_nChannels + CHUNK_SIZE - 1)/CHUNK_SIZE;

    // write colors
    for (j = 0; j < chunks; j++) {
        if ((memcmp(&m_color_buffer[j*CHUNK_SIZE], \
                    &m_color_buffer_old[j*CHUNK_SIZE], CHUNK_SIZE) == 0) \
            && (m_use_memcmp == 1)) {
            continue;
        }
        m_byteswritten = write(m_fd, m_wr_buffer, \
                               KarateLight::CreateCommand(j+CMD_SET_DATA_00, \
                               &m_color_buffer[j*CHUNK_SIZE], \
                               CHUNK_SIZE));
        if (m_byteswritten != (CMD_DATA_START + CHUNK_SIZE)) {
            // set the error message
            // XXX TODO cpresser
            // snprintf(m_errortext, MAX_ERROR_LENGTH, "failed to write data to %s", m_devname);
            return KL_WRITEFAIL;
        }
        if (KarateLight::ReadBack() != KL_OK) {
            return KL_WRITEFAIL;
        }
    }
    // update old_values
    memcpy(&m_color_buffer_old[0], &m_color_buffer[0], MAX_CHANNELS);
    return KL_OK;
}

int KarateLight::SetColors(uint8_t* buffer, int length) {
    if (length <= MAX_CHANNELS) {
        memcpy(&m_color_buffer[0], buffer, length);
        return KL_OK;
    } else {
        // XXX TODO cpresser
        // snprintf(m_errortext, MAX_ERROR_LENGTH, "channel %d out of range", length);
        return KL_ERROR;
    }
}

/**
 * Sets all Channels to black and sends data to the device
 * \returns KL_OK on success, a negative error value otherwise
 */
int KarateLight::Blank() {
    int j;

    // set channels to black
    memset(m_color_buffer, 0, MAX_CHANNELS);
    memset(m_color_buffer_old, 1, MAX_CHANNELS);
    j = KarateLight::UpdateColors();
    if (j != 0)
        // error-string is already set while in UpdateColors()
        return j;

    return KL_OK;
}

/**
 * Reads the a single byte from the eeprom
 * \parm addr the eeprom location
 * \return the eeprom byte, or a return-code <0 in case there is an error
 */
int KarateLight::ReadEeprom(uint8_t addr) {
    unsigned int j;

    if (!m_active)
        return KL_NOTACTIVE;

    m_byteswritten = write(m_fd, m_wr_buffer, \
                           KarateLight::CreateCommand(CMD_READ_EEPROM, &addr, 1));
    if (m_byteswritten != CMD_DATA_START+1) {
        // XXX TODO cpresser
        // snprintf(m_errortext, MAX_ERROR_LENGTH, "failed to write data to %s", m_devname);
        return KL_WRITEFAIL;
    }

    j = KarateLight::ReadBack();
    if (j == 1) {
        return m_rd_buffer[CMD_DATA_START];
    }
    return j;  // if j!=2 the error-string will be set from within ReadBack();
}

/**
 * Initialize the device
 * 1. open the devicefile and get a file lock
 * 2. read defaults (firmware, hardware, channels count)
 * 3. set all channels to black
 */
int KarateLight::Init() {
    int j, i;
    struct termios options;

    if (m_active)
        return KL_NOTACTIVE;

    // fd = open(devname, O_RDWR | O_NOCTTY |  O_NONBLOCK);
    m_fd = open(m_devname.c_str(), O_RDWR | O_NOCTTY);
    if (m_fd < 0) {
        // XXX TODO cpresser
        // snprintf(errortext, MAX_ERROR_LENGTH, "failed to open %s", devname);
        return KL_ERROR;
    }

    /* Clear the line */
    tcflush(m_fd, TCOFLUSH);

    memset(&options, 0, sizeof(options));

    cfsetispeed(&options, B115200);
    cfsetospeed(&options, B115200);

    // options.c_cflag = (CS8 | CSTOPB | CLOCAL | CREAD);
    options.c_cflag = (CS8 | CLOCAL | CREAD);

    // If MIN = 0 and TIME > 0, TIME serves as a timeout value. The read will be satisfied if a single character is read, or TIME is exceeded (t = TIME *0.1 s). If TIME is exceeded, no character will be returned.
    options.c_cc[VTIME] = 1;
    options.c_cc[VMIN]  = 0;  // always require at least one byte returned


    // Update the options and do it NOW
    if (tcsetattr(m_fd, TCSANOW, &options) != 0) {
        // snprintf(errortext, MAX_ERROR_LENGTH, "tcsetattr failed on %s", devname);
        return KL_ERROR;
    }


    // clear possible junk data still in the systems fifo
    do {
        m_bytesread = read(m_fd, m_rd_buffer, 255);
    }
    while (m_bytesread > 0);


    // read firmware version
    m_byteswritten = write(m_fd, m_wr_buffer, \
                           KarateLight::CreateCommand(CMD_GET_VERSION, NULL, 0));
    if (m_byteswritten != CMD_DATA_START) {
        return KL_WRITEFAIL;
    }

    j = KarateLight::ReadBack();
    if (j == 1) {
        m_fw_version = m_rd_buffer[CMD_DATA_START];
    } else {
        return j;
    }

    // if an older Firware-Version is used. quit. the communication wont work out
    if (m_fw_version < 0x30) {
        // snprintf(m_errortext, MAX_ERROR_LENGTH, "Firmware 0x%0x is to old!\n", fw_version);
        return KL_ERROR;
    }
    m_active = true;

    // read HW version
    m_byteswritten = write(m_fd, m_wr_buffer, \
                           KarateLight::CreateCommand(CMD_GET_HARDWARE, NULL, 0));
    if (m_byteswritten != CMD_DATA_START) {
        return KL_WRITEFAIL;
    }

    j = KarateLight::ReadBack();
    if (j == 1) {
        m_hw_version = m_rd_buffer[CMD_DATA_START];
    } else {
        return j;
    }

    // read number of channels
    m_byteswritten = write(m_fd, m_wr_buffer, \
                           KarateLight::CreateCommand(CMD_GET_N_CHANNELS, NULL, 0));
    if (m_byteswritten != CMD_DATA_START) {
        return KL_WRITEFAIL;
    }

    j = KarateLight::ReadBack();
    if (j == 2) {
        m_nChannels = m_rd_buffer[CMD_DATA_START] + m_rd_buffer[CMD_DATA_START+1] * 256;;
    } else {
        return j;
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
            // snprintf(errortext, MAX_ERROR_LENGTH, "Error Reading EEPROM\n");
            return j;
        }

        if (m_dmx_offset > 511) {
            // snprintf(errortext, MAX_ERROR_LENGTH, "DMX Offset to large (%d)\n", m_dmx_offset);
            m_dmx_offset = 0;
            return KL_ERROR;
        }

    } else {
        // KL-DMX-Device
        m_dmx_offset = 0;
    }


    // set channels to zero
    KarateLight::Blank();

    return KL_OK;
}

/*
 PRIVATE FUNCTIONS
*/

/**
 * Perform a checksum calculation of the command currently in the buffer
 * \param the lenght of the command to be processed
 * \returns the lengh of the command
 */
int KarateLight::CalcChecksum(int len) {
    int i;

    // clear byte 2
    m_wr_buffer[CMD_HD_CHECK] = 0;

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
int KarateLight::CreateCommand(int cmd, uint8_t * data, int len) {
    int i = 0;

    // maximum command lenght
    if (len > (CMD_MAX_LENGTH - CMD_DATA_START)) {
        //snprintf(m_errortext, MAX_ERROR_LENGTH, "Error: Command is to long (%d > %d)\n", len, (CMD_MAX_LENGTH - CMD_DATA_START));
        return KL_ERROR;
    }

    // build header
    m_wr_buffer[CMD_HD_SYNC]     = CMD_SYNC_SEND;
    m_wr_buffer[CMD_HD_COMMAND]  = cmd;
    m_wr_buffer[CMD_HD_LEN]      = len;

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
        //snprintf(m_errortext, MAX_ERROR_LENGTH, "could not read 4 bytes (header) from %s", m_devname);
        return KL_ERROR;
    }

    // read sequential
    if (m_rd_buffer[CMD_HD_LEN] > 0) {
        m_bytesread = read(m_fd, &m_rd_buffer[CMD_DATA_START], m_rd_buffer[CMD_HD_LEN]);
    } else {
        m_bytesread = 0;
    }

    if (m_bytesread == m_rd_buffer[CMD_HD_LEN]) {
        // TODO(cpresser) verify checksum
        return (m_bytesread);
    } else {
        //snprintf(m_errortext, MAX_ERROR_LENGTH, "number of bytes read(%d) does not match number of bytes expected(%d) \n", m_bytesread, m_rd_buffer[CMD_HD_LEN]);
        return KL_CHECKSUMFAIL;
    }
}
