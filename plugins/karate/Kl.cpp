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


#include <stdio.h>
#include <string.h>
#include "kl.h"


#ifdef WIN32
// windows
#include <windows.h>
#define OPEN_DEVICE(name) CreateFile(name, GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0)
#define CLOSE_DEVICE(fd) CloseHandle(fd)
#define WRITE_DATA(fd, buf, len) WriteFile(fd, buf, (DWORD)len, &byteswritten, NULL)
#define READ_DATA(fd,buf,len,bread) ReadFile(fd, buf, (DWORD)len, &bread, NULL)
#else
// regular unix
#include <termios.h>
#include <fcntl.h>
#include <sys/file.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#define CLOSE_DEVICE(fd) close(fd)
#define WRITE_DATA(fd, buf, len) byteswritten = write(fd, buf, len); tcdrain(fd)
#define READ_DATA(fd,buf,len,bread) bread = read(fd, buf, len)
#endif



/**
 * Default constructor
 * \param device the filename of the device to use
 */
KarateLight::KarateLight(char *device) {
// set defaults 
  active  = false;
  use_memcmp = 1;
  fw_version = 0;
  hw_version = 0;
  nChannels = 0;

  // copy the device name into my context...
  if (device != 0) {
    devname = new char [ strlen(device) + 1];
    strcpy(devname, device);
  }
  
  // set the error message
  snprintf(errortext, MAX_ERROR_LENGTH, "not yet initalized");
}


/**
 * Default desctuctor
 * closes the device and does release the file-lock
 */
KarateLight::~KarateLight(){
#ifdef WIN32
  CloseHandle(fd);
#else
  // remove lock and close file
  flock(fd, LOCK_UN);
  tcflush(fd,TCIOFLUSH);
  close(fd);
#endif
  fd=0;
}



/**
 * Sends color values previously set via 
 * \sa SetColor
 * \returns KL_OK on success, a negative error value otherwise
 */
int KarateLight::UpdateColors() {
  int j;
  int chunks;

  if (!active)
    return KL_NOTACTIVE;

  chunks = (nChannels+31)/32;

  // write colors
  for (j = 0; j < chunks; j++) {
    if ((memcmp(&color_buffer[j*32],&color_buffer_old[j*32],32) == 0 ) && (use_memcmp == 1)) {
      continue;
    }
    WRITE_DATA(fd,wr_buffer,KarateLight::CreateCommand(j+0x20,&color_buffer[j*32],32));
    if (byteswritten != (CMD_DATA_START + 32)) {
      // set the error message
      snprintf(errortext, MAX_ERROR_LENGTH, "failed to write data to %s",devname);
      return KL_WRITEFAIL;
    }
    if (KarateLight::ReadBack() != KL_OK) {
      return KL_WRITEFAIL;
    }
  }
  // update old_values
  memcpy(&color_buffer_old[0],&color_buffer[0],MAX_CHANNELS);
  return KL_OK;
}

int KarateLight::SetColors(uint8_t* buffer, int length) {
  if (length <= MAX_CHANNELS) {
    memcpy(&color_buffer[0],buffer,length);
    return KL_OK;
  } else {
    snprintf(errortext, MAX_ERROR_LENGTH, "channel %d out of range",length);
    return KL_ERROR;
  }
}

/**
 * Sends the device into boot-mode for a following firmware update
 * Since this means a usb-disconnect it also sets the handle to inactive preventing
 * furher writes to the device.
 * \returns KL_OK on success, a negative error value otherwise
 */
int KarateLight::EnterBoot() {
  int j;

  if (!active)
    return KL_NOTACTIVE;

  // send boot request..
  WRITE_DATA(fd,wr_buffer,KarateLight::CreateCommand(CMD_BOOT_REQUEST,NULL,0));
  if (byteswritten != CMD_DATA_START) {
    snprintf(errortext, MAX_ERROR_LENGTH, "failed to write data to %s",devname);
    return KL_WRITEFAIL;
  }

  // read 2 bytes and send them as verification
  j = KarateLight::ReadBack();
  if (j == 2) {
    WRITE_DATA(fd,wr_buffer,KarateLight::CreateCommand(CMD_BOOT_START,&rd_buffer[CMD_DATA_START],2));
    if (byteswritten != (CMD_DATA_START + 2)) {
      snprintf(errortext, MAX_ERROR_LENGTH, "failed to write data to %s",devname);
      return KL_WRITEFAIL;
    }
    active = false;
    // no more readback... just close the device
  }
  return KL_OK;
}


/**
 * Sets a color for one specific channel
 * \param chan the channel
 * \param the brightness value to set
 * \returns KL_OK on success, a negative error value otherwise
 */
int KarateLight::SetColor(uint16_t chan, uint8_t val ) {
  if (chan < nChannels) {
    color_buffer[chan ] = val;
    return KL_OK;
  }
  snprintf(errortext, MAX_ERROR_LENGTH, "channel %d out of range",chan);
  return KL_ERROR;
}


/**
 * Sets all Channels to black and sends data to the device
 * \returns KL_OK on success, a negative error value otherwise
 */
int KarateLight::Blank () {
 int j;

  // set channels to black
  memset(color_buffer,0,MAX_CHANNELS);
  memset(color_buffer_old,1,MAX_CHANNELS);
  j = KarateLight::UpdateColors();
  if (j != 0)
    // error-string is already set while in UpdateColors()
    return j;

  return KL_OK;
}


/**
 * Sends the specified PWM-Calibation value to the device
 * \param val the value to be set
 * \return KL_OK on success
 */
int KarateLight::SetPWMValue(unsigned int val) {
  uint8_t pp[2];

  if (!active)
    return KL_NOTACTIVE;

  pwm_value=val;

  pp[0] = (pwm_value & 0xFF);
  pp[1] = (pwm_value>>8 & 0xFF);

  WRITE_DATA(fd,wr_buffer,KarateLight::CreateCommand(CMD_SET_TLC_PWM_VALUE,pp,2));
  if (byteswritten != (CMD_DATA_START + 2)) {
    snprintf(errortext, MAX_ERROR_LENGTH, "failed to write data to %s",devname);
    return KL_WRITEFAIL;
  }
  return KarateLight::ReadBack();
}


/**
 * Reads the  PWM-Calibation value from the device
 * depending on the firmware on the device, the firmware will
 * measure the value each time this command is received.
 *
 * \return the pwm_value, or a return-code <0 in case there is an error
 */
int KarateLight::GetPWMValue() {
  unsigned int j;

  if (!active)
    return KL_NOTACTIVE;

  WRITE_DATA(fd,wr_buffer,KarateLight::CreateCommand(CMD_GET_TLC_PWM_VALUE,NULL,0));
  if (byteswritten != CMD_DATA_START) {
    snprintf(errortext, MAX_ERROR_LENGTH, "failed to write data to %s",devname);
    return KL_WRITEFAIL;
  }

  j = KarateLight::ReadBack();
  if (j == 2) {
    pwm_value=rd_buffer[CMD_DATA_START];
    pwm_value+=rd_buffer[CMD_DATA_START+1]*256;
    return (unsigned int) pwm_value;
  } 
  return j; // if j!=2 the error-string will be set from within ReadBack();
}


/**
 * Reads the a single byte from the eeprom 
 * \parm addr the eeprom location
 * \return the eeprom byte, or a return-code <0 in case there is an error
 */
int KarateLight::ReadEeprom(uint8_t addr) {
  unsigned int j;

  if (!active)
    return KL_NOTACTIVE;

  WRITE_DATA(fd,wr_buffer,KarateLight::CreateCommand(CMD_READ_EEPROM,&addr,1));
  if (byteswritten != CMD_DATA_START+1) {
    snprintf(errortext, MAX_ERROR_LENGTH, "failed to write data to %s",devname);
    return KL_WRITEFAIL;
  }

  j = KarateLight::ReadBack();
  if (j == 1) {
    return (unsigned int) rd_buffer[CMD_DATA_START];
  } 
  return j; // if j!=2 the error-string will be set from within ReadBack();
}

/**
 * Write a single byte to the eeprom 
 * \parm addr the eeprom location
 * \parm val the eeprom location
 * \return return-code <0 in case there is an error
 */
int KarateLight::WriteEeprom(uint8_t addr, uint8_t val) {
  uint8_t eep[2];

  if (!active)
    return KL_NOTACTIVE;

  eep[0] = addr;
  eep[1] = val;

  WRITE_DATA(fd,wr_buffer,KarateLight::CreateCommand(CMD_WRITE_EEPROM,eep,2));
  if (byteswritten != CMD_DATA_START+2) {
    snprintf(errortext, MAX_ERROR_LENGTH, "failed to write data to %s",devname);
    return KL_WRITEFAIL;
  }

  return KarateLight::ReadBack();
}

/**
 * Reads the LDR value from the device
 * depending on the firmware on the device, the firmware will
 * measure the value each time this command is received.
 *
 * \return the ldr_value, or a return-code <0 in case there is an error
 */
int KarateLight::GetLDRValue() {
  int j;

  if (!active)
    return KL_NOTACTIVE;

  WRITE_DATA(fd,wr_buffer,KarateLight::CreateCommand(CMD_READ_ADC0,NULL,0));
  if (byteswritten != CMD_DATA_START) {
    snprintf(errortext, MAX_ERROR_LENGTH, "failed to write data to %s",devname);
    return KL_WRITEFAIL;
  }

  j = KarateLight::ReadBack();
  if (j == 2) {
    ldr_value=rd_buffer[CMD_DATA_START];
    ldr_value+=rd_buffer[CMD_DATA_START+1]*256;
    return (unsigned int) ldr_value;
  } 
  return j; // if j!=2 the error-string will be set from within ReadBack();
}


/**
 * Initialize the device
 * 1. open the devicefile and get a file lock
 * 2. read defaults (firmware, hardware, channels count)
 * 3. set all channels to black
 */
int KarateLight::Init() {
  int j,i;

  if (active)
    return KL_NOTACTIVE;
	
#ifdef WIN32
  DCB dcbSerialParams = {0};
  COMMTIMEOUTS timeouts={0};

  fd=CreateFileA(devnname, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

  if ((int)fd <0) {
    snprintf(errortext, MAX_ERROR_LENGTH, "failed to open %s",devname);
    return KarateLight::ERROR;
  }
  dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

  j = GetCommState(fd, &dcbSerialParams);
  if (j) {
    dcbSerialParams.BaudRate = CBR_57600;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    j = SetCommState(fd, &dcbSerialParams);
  } 
  if (!j) {
    snprintf(errortext, MAX_ERROR_LENGTH, "error configuring %s (SetCommState returned %d)\n",devname,j);
    return KarateLight::ERROR;
  }
  
  timeouts.ReadIntervalTimeout=50;
  timeouts.ReadTotalTimeoutConstant=50;
  timeouts.ReadTotalTimeoutMultiplier=10;
  timeouts.WriteTotalTimeoutConstant=50;
  timeouts.WriteTotalTimeoutMultiplier=10;
  if(!SetCommTimeouts(fd, &timeouts)){
    snprintf(errortext, MAX_ERROR_LENGTH,"error setting comm-timeouts (%s)\n",devname);
    return KarateLight::ERROR;
  }
#else
  struct termios options;

  //fd = open(devname, O_RDWR | O_NOCTTY |  O_NONBLOCK);
  fd = open(devname, O_RDWR | O_NOCTTY);
  if (fd <0) {
    snprintf(errortext, MAX_ERROR_LENGTH, "failed to open %s",devname);
    return KL_ERROR;
  }

  /* Clear the line */
  tcflush(fd,TCOFLUSH);

  memset(&options, 0, sizeof(options));

  cfsetispeed(&options, B115200);
  cfsetospeed(&options, B115200);

  //options.c_cflag = (CS8 | CSTOPB | CLOCAL | CREAD);
  options.c_cflag = (CS8 | CLOCAL | CREAD);

  //If MIN = 0 and TIME > 0, TIME serves as a timeout value. The read will be satisfied if a single character is read, or TIME is exceeded (t = TIME *0.1 s). If TIME is exceeded, no character will be returned. 
  options.c_cc[VTIME] = 1;
  options.c_cc[VMIN]  = 0; // always require at least one byte returned


  /* Update the options and do it NOW */
  if (tcsetattr(fd,TCSANOW,&options) != 0) {
    snprintf(errortext, MAX_ERROR_LENGTH, "tcsetattr failed on %s",devname);
    return KL_ERROR;
  } 

#endif

  // clear possible junk data still in the systems fifo
  do {
    READ_DATA(fd,rd_buffer,255,bytesread);
    if (bytesread > 0) {
#ifdef DEBUG
    printf("cleared %d junkbytes\n",bytesread);
    int j;
    printf("read buffer  (l=%2d) ",(int)(bytesread));
    for (j=0; j<bytesread; j++) {
      printf("0x%0X ",rd_buffer[j]);
    }
    printf("\n");
#endif
    }
  }
  while (bytesread > 0);


  // read firmware version
  WRITE_DATA(fd,wr_buffer,KarateLight::CreateCommand(CMD_GET_VERSION,NULL,0));
  if (byteswritten != CMD_DATA_START) {
    return KL_WRITEFAIL;
  }

  j = KarateLight::ReadBack();
  if (j == 1) {
    fw_version= (unsigned int) rd_buffer[CMD_DATA_START];
  } else {
    return j;
  }

  // read HW version
  WRITE_DATA(fd,wr_buffer,KarateLight::CreateCommand(CMD_GET_HARDWARE,NULL,0));
  if (byteswritten != CMD_DATA_START) {
    return KL_WRITEFAIL;
  }

  j = KarateLight::ReadBack();
  if (j == 1) {
    hw_version= (unsigned int) rd_buffer[CMD_DATA_START];
  } else {
    return j;
  }

  // read number of channels
  WRITE_DATA(fd,wr_buffer,KarateLight::CreateCommand(CMD_GET_N_CHANNELS,NULL,0));
  if (byteswritten != CMD_DATA_START) {
    return KL_WRITEFAIL;
  }

  j = KarateLight::ReadBack();
  if (j == 2) {
    nChannels=rd_buffer[CMD_DATA_START];
    nChannels+=rd_buffer[CMD_DATA_START+1]*256;
  } else {
    return j;
  }


  if (fw_version < 0x30) {
    snprintf(errortext, MAX_ERROR_LENGTH,"Firmware 0x%0x is to old!\n",fw_version);
    return KL_ERROR;
  } 
  
  active = true;
   
  if (hw_version == HW_ID_KARATE) {
    // disable memcmp for the classic KarateLight Hardware
    use_memcmp = 0;

    j = KarateLight::ReadEeprom(0x03);
    i = KarateLight::ReadEeprom(0x02);
    if ((j>=0) && (i>=0)) {
      dmx_offset = j*256;
      dmx_offset = i;
    } else {
      snprintf(errortext, MAX_ERROR_LENGTH,"Error Reading EEPROM\n");
      return j;
    }
    
    if (dmx_offset > 511) {
      snprintf(errortext, MAX_ERROR_LENGTH,"DMX Offset to large (%d)\n",dmx_offset);
      dmx_offset = 0;
      return KL_ERROR;
    }

  } else {
    dmx_offset = 0;
  }


  // set channels to zero
  KarateLight::Blank();

  snprintf(errortext, MAX_ERROR_LENGTH,"no error, fool!\n");
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
int KarateLight::CalcChecksum (int len) {
  int i;

  // clear byte 2
  wr_buffer[CMD_HD_CHECK] = 0;

  for (i=0; i<len; i++) {
    if (i !=CMD_HD_CHECK) {
      wr_buffer[CMD_HD_CHECK]^= wr_buffer[i];
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
int KarateLight::CreateCommand (int cmd, uint8_t * data, int len) {
  int i = 0;

  // maximum command lenght
  if (len > (CMD_MAX_LENGTH - CMD_DATA_START)) {
    snprintf(errortext, MAX_ERROR_LENGTH,"Error: Command is to long (%d > %d)\n",len, (CMD_MAX_LENGTH - CMD_DATA_START));
    return KL_ERROR;
  }

  // build header
  wr_buffer[CMD_HD_SYNC]     = CMD_SYNC_SEND;
  wr_buffer[CMD_HD_COMMAND]  = cmd;
  wr_buffer[CMD_HD_LEN]      = len;

  // copy data
  while (len > i) {
    wr_buffer[CMD_DATA_START+i] = data[i];
    i++;
  }
 
  // calc checksum
  i =  KarateLight::CalcChecksum(CMD_DATA_START+len);

#ifdef DEBUG
  int j;
  printf("write buffer (l=%2d) ",i);
  for (j=0; j<i; j++) {
    printf("0x%0X ",wr_buffer[j]);
  }
  printf ("\n");
#endif
  
  return i;
}

/**
 * Tries to read an answer from the device
 * \return the number of bytes read, or an error value <0
 */
int KarateLight::ReadBack () {
  // read header 
  READ_DATA(fd,rd_buffer,CMD_DATA_START,bytesread);

  if (bytesread != CMD_DATA_START) {
    snprintf(errortext, MAX_ERROR_LENGTH,"could not read 4 bytes (header) from %s",devname);
    return KL_ERROR;
  }
  
  // read sequential
  if (rd_buffer[CMD_HD_LEN] > 0) {
    READ_DATA(fd, &rd_buffer[CMD_DATA_START], rd_buffer[CMD_HD_LEN],bytesread);
  } else {
    bytesread = 0;
  }

#ifdef DEBUG
    int j;
    printf("read buffer  (l=%2d) ",(int)(bytesread+CMD_DATA_START));
    for (j=0; j<(bytesread+CMD_DATA_START); j++) {
      printf("0x%0X ",rd_buffer[j]);
    }
    printf("\n");
#endif

  if (bytesread == rd_buffer[CMD_HD_LEN]) {
    // TODO verify checksum
    return (bytesread);
  } else {
    snprintf(errortext, MAX_ERROR_LENGTH,"number of bytes read(%d) does not match number of bytes expected(%d) \n", bytesread, rd_buffer[CMD_HD_LEN]);  
    return KL_CHECKSUMFAIL;
  }
}
