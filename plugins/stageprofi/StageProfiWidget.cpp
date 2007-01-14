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
 * stageprofiwidget.cpp
 * StageProfi Widget
 * Copyright (C) 2006-2007 Simon Newton
 *
 * The device represents the widget.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <llad/logger.h>

#include "StageProfiWidget.h"

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#define min(a,b) a<b?a:b

static const unsigned DMX_MSG_LEN = 256;

enum stageprofi_packet_type_e {
	ID_GETDMX =	0xFE,
	ID_SETDMX = 0xFF,
	ID_SETLO =  0xE0,
	ID_SETHI =  0xE1,
};

typedef enum stageprofi_packet_type_e stageprofi_packet_type;

/*
 * Connect to the widget
 */
int StageProfiWidget::connect(const string &path) {
	struct termios newtio;
	m_fd = open(path.c_str(), O_RDWR | O_NONBLOCK | O_NOCTTY);

	if(m_fd == -1) {
		return 1 ;
	}

	bzero(&newtio, sizeof(newtio)); // clear struct for new port settings
	tcgetattr(m_fd, &newtio);
	cfsetospeed(&newtio, B38400);
	tcsetattr(m_fd,TCSANOW,&newtio);

	return 0;
}


/*
 * Disconnect from the widget
 */
int StageProfiWidget::disconnect() {
	if(m_fd)
		close(m_fd);

	m_fd = -1;
	return 0;
}


/*
 * Send a dmx msg
 */
int StageProfiWidget::send_dmx(uint8_t *buf, unsigned int len) const {

	if(len > DMX_MSG_LEN) {
		// send the other 256
		send_256(0, buf, DMX_MSG_LEN);
		send_256(DMX_MSG_LEN, buf+DMX_MSG_LEN, len-DMX_MSG_LEN);
	} else {
		send_256(0, buf, len);
	}

	return 0;
}


/*
 * read data from the widget
 */
int StageProfiWidget::recv() {
	int unread;

	// check if there is data to be read 
	if ( ioctl( m_fd, FIONREAD , &unread) ) {
		// the device has been removed
		Logger::instance()->log(Logger::WARN, "StageProfiPlugin: device removed" );
		return -1;
	}

	while (unread != 0) {
		do_recv();
		ioctl(m_fd, FIONREAD, &unread);
	}

	return 0;
}



//-----------------------------------------------------------------------------
// Private methods used for communicating with the widget


/*
 * Send 256 channels
 */
int StageProfiWidget::send_256(unsigned int start, uint8_t *buf, unsigned int len) const {
	uint8_t msg[256+4];
	unsigned int l = min(DMX_MSG_LEN, len);

	msg[0] = ID_SETDMX;
	msg[1] = start & 0xFF;
	msg[2] = (start>>8) & 0xFF;
	msg[3] = l-1;
	memcpy(msg+4, buf, l);
//	printf("send %hhx %hhx %hhx %hhx %hhx %hhx\n", msg[0], msg[1], msg[2], msg[3], msg[4]);
	return write(m_fd, msg, len+4);
}


/*
 *
 */
int StageProfiWidget::do_recv() {
	uint8_t byte = 0x00;
    int cnt;

    while(byte != 'G') {
		cnt = read(m_fd,&byte,1);

		if(cnt != 1) {
			printf("1, read to much %i\n", cnt);
			return -1;
		}
		printf("Got %c\n", byte);
	}

	return 0;
}
