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
 * usbprodevice.cpp
 * UsbPro device
 * Copyright (C) 2006  Simon Newton
 *
 * The device creates two ports, one in and one out, but you can only use one at a time.
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


#include "usbprodevice.h"
#include "usbproport.h"

#include <lla/logger.h>
#include <lla/preferences.h>


#if HAVE_CONFIG_H
#  include <config.h>
#endif

#define min(a,b) a<b?a:b

#define RCMODE_ALWAYS	0x00
#define RCMODE_CHANGE	0x01

#define SOM	0x7e
#define EOM	"\347"
#define ID_PRMREQ 	0x03
#define ID_PRMREP 	0x03
#define ID_PRMSET	0x04
#define ID_RDMX		0x05
#define ID_SDMX		0x06
#define ID_RDM		0x07
#define ID_RCMODE	0x08
#define ID_COS		0x09
#define ID_SNOREQ	0x0A
#define ID_SNOREP	0x0A

/*
 * Create a new device
 */
UsbProDevice::UsbProDevice(Plugin *owner, const char *name, string dev_path) : Device(owner, name) {
	m_dev_path = dev_path ;
	m_fd = 0;
	m_enabled = false ;
}


/*
 * Destory this device
 */
UsbProDevice::~UsbProDevice() {
	if (m_enabled)
		stop() ;
}


/*
 * Start this device
 *
 */
int UsbProDevice::start() {
	UsbProPort *port = NULL ;
	Port *prt = NULL;
	int debug = 0 ;
	int ret ;
	
	/* set up ports */
	for(int i=0; i < 2; i++) {
		port = new UsbProPort(this,i) ;

		if(port != NULL) 
			this->add_port(port) ;
	}
	
	// create new usbpro node, and and set config values
	ret = w_connect(m_dev_path.c_str() ) ;

	if(ret) {
		Logger::instance()->log(Logger::WARN, "UsbProPlugin: failed to connect to %s", m_dev_path.c_str()  ) ;
		goto e_dev ;
	}
	
	if (w_init() ) {
		Logger::instance()->log(Logger::WARN, "UsbProPlugin: init failed , is this an pro widget?" ) ;
		goto e_disconnect ;
	}

	// clear buffer
	memset(m_dmx, 0x00, DMX_BUF_LEN) ;

	m_enabled = true ;

	return 0;

e_disconnect:
	w_disconnect() ;

e_dev:
	for(int i=0; i < port_count() ; i++) {
		prt = get_port(i) ;
		if(prt != NULL) 
			delete prt ;
	}


	return -1;
}


/*
 * stop this device
 *
 */
int UsbProDevice::stop() {
	Port *prt = NULL;

	if (!m_enabled)
		return 0 ;

	// disconnect from widget
	w_disconnect() ;
	
	for(int i=0; i < port_count() ; i++) {
		prt = get_port(i) ;
		if(prt != NULL) 
			delete prt ;
	}

	m_enabled = false ;

	return 0;
}



/*
 * return the sd of this device
 *
 */
int UsbProDevice::get_sd() const {
	return m_fd ;
}

/*
 * Called when there is activity on our descriptors
 *
 * @param	data	user data (pointer to usbpro_device_priv
 */
int UsbProDevice::fd_action() {

	w_recv() ;
	
	return 0;
}

/*
 * Send the dmx out the widget
 * called from the UsbProPort
 *
 * @return 	0 on success, non 0 on failure
 */
int UsbProDevice::send_dmx(uint8_t *data, int len) {

	return w_send_dmx(data,len) ;

}

/*
 * Copy the dmx buffer into the arguments
 * Called from the UsbProPort
 *
 * @return 	the length of the dmx data copied
 */
int UsbProDevice::get_dmx(uint8_t *data, int len) {

	int l = min(len, DMX_BUF_LEN-1) ;

	// byte 0 is the start code which we ignore
	memcpy(data,m_dmx+1, l) ;

	return l;
}









// call this when something changes
// where to store data to ?
// I'm thinking a config file in /etc/llad/llad.conf
int UsbProDevice::save_config() {


	return 0;
}



/*
 * we can pass plugin specific messages here
 * make sure the user app knows the format though...
 *
 */
int UsbProDevice::configure(void *req, int len) {
	// handle short/ long name & subnet and port addresses
	
	req = 0 ;
	len = 0;

	return 0;
}


//-----------------------------------------------------------------------------
//
// Private method used for communicating with the widget

/*
 * Connect to the widget
 * TODO: allow the widget to be removed
 *
 */
int UsbProDevice::w_connect(const char *dev) {
	struct termios oldtio, newtio ;
	m_fd = open(dev, O_RDWR | O_NONBLOCK ) ;

	if(m_fd == -1) {
		return 1 ;
	}

	bzero(&newtio, sizeof(newtio)); /* clear struct for new port settings */
	tcsetattr(m_fd,TCSANOW,&newtio);

	return 0;
}


/*
 * Disconnect from the widget
 *
 */
int UsbProDevice::w_disconnect() {
	if(m_fd)
		close(m_fd);

	return 0;
}



/*
 * We perform a couple of tests here to make sure this behaves like we expect
 *
 *
 */
int UsbProDevice::w_init() {




	// ok looks good, set the mode to send on change
	w_send_rcmode(1) ;


	return 0;
}


/*
 * Set the length of the msg
 */
int UsbProDevice::w_set_msg_len(promsg *msg, int len) {
	msg->len = len & 0xFF ;
	msg->len_hi = (len & 0xFF00) >> 8 ;
}

/*
 * Send the msg
 */
int UsbProDevice::w_send_msg(promsg *msg) {
	int len = (msg->len_hi << 8) + msg->len ;
	write(m_fd, msg, len+4) ;
	write(m_fd, EOM, sizeof(EOM) ) ;
}

/*
 * Send a dmx msg
 *
 *
 */
int UsbProDevice::w_send_dmx(uint8_t *buf, int len) {
	int l = min(DMX_BUF_LEN, len) ;
	promsg msg ;

	msg.som = SOM ;
	msg.label = ID_SDMX ;
	w_set_msg_len(&msg, l+1);

	//start code to 0
	msg.pm_dmx.dmx[0] = 0 ;
	memcpy(&msg.pm_dmx.dmx[1], buf, l) ;

	return w_send_msg(&msg) ;
}


/*
 * Send a rdm msg
 *
 *
 */
int UsbProDevice::w_send_rdm(uint8_t *buf, int len) {

	promsg msg ;

	msg.som = SOM ;
	msg.label = ID_RDM ;
	w_set_msg_len(&msg, len);
	memcpy(&msg.pm_rdm.dmx, buf, len) ;

	return w_send_msg(&msg) ;
}

/*
 * Send a get param request
 *
 */
int UsbProDevice::w_send_prmreq(int usrsz) {

	promsg msg ;

	msg.som = SOM ;
	msg.label = ID_PRMREQ ;
	msg.pm_prmreq.len = usrsz & 0xFF;
	msg.pm_prmreq.len_hi = (usrsz & 0xFF) >> 8;
	
	return w_send_msg(&msg) ;
}

/*
 * Send a mode msg
 *
 *
 */
int UsbProDevice::w_send_rcmode(int mode) {
	int ret ;
	uint8_t m = mode ? RCMODE_CHANGE : RCMODE_ALWAYS ;
	promsg msg ;

	msg.som = SOM ;
	msg.label = ID_RCMODE ;
	w_set_msg_len(&msg, sizeof(pms_rcmode) );
	msg.pm_rcmode.mode = mode ;

	ret = w_send_msg(&msg) ;

	if(!ret && m) 
		memset(m_dmx, 0x00, DMX_BUF_LEN) ;
	return ret ;
}



/*
 * Send a serial number request
 *
 */
int UsbProDevice::w_send_snoreq() {

	promsg msg ;

	msg.som = SOM ;
	msg.label = ID_SNOREQ ;
	w_set_msg_len(&msg, sizeof(pms_snoreq) );
	
	return w_send_msg(&msg) ;
}



/*
 * Handle the dmx frame
 *
 */
int UsbProDevice::w_handle_dmx(pms_rdmx *dmx, int len) {

	printf(" stc %hhx  %hhx %hhx\n", dmx->dmx[0], dmx->dmx[1], dmx->dmx[2]) ;

	//copy to buffer here
	return 0;
}

/*
 * handle the dmx change of state frame
 *
 */
int UsbProDevice::w_handle_cos(pms_cos *cos, int len) {
	int chn_st = cos->start *8;
	int i, offset ;
	
	//get our input port
	Port *prt = get_port(0) ;
	
//	printf(" %hhx  %hhx %hhx %hhx %hhx\n", cos->changed[0], cos->changed[1], cos->changed[2], cos->changed[3], cos->changed[4]) ;

	// should be checking length here
	offset = 0 ;
	for(i = 0; i< 40; i++) {

		if( cos->changed[i/8] & (1 << (i%8)) ) {
			m_dmx[chn_st + i] = cos->data[offset] ;
			offset++ ;
		}
	}

	prt->dmx_changed() ;
	return 0;
}

/*
 * Handle the param reply
 *
 */
int UsbProDevice::w_handle_prmrep(pms_prmrep *rep, int len) {
	int frmvr = rep->firmv + (rep->firmv_hi <<8) ;
	
	printf("got param reply\n") ;
	printf(" firmware is %i\n", frmvr ) ;
	printf(" brk tm %hhx\n", rep->brtm) ;
	printf(" mab tm %hhx\n", rep->mabtm) ;
	printf(" rate %hhx\n", rep->rate) ;

//	send_snoreq() ;
}

/*
 * Handle the serial number reply
 */
int UsbProDevice::w_handle_snorep(pms_snorep *rep, int len) {
	
	printf("got serno reply\n") ;
	printf(" 1 %hhx\n", rep->srno[0] ) ;
	printf(" 2 %hhx\n", rep->srno[1]) ;
	printf(" 3 %hhx\n", rep->srno[2]) ;
	printf(" 4 %hhx\n", rep->srno[3]) ;

//	send_rcmode(1) ;
}

/*
 * Receive data from the widget
 *
 */
int UsbProDevice::w_recv() {
	uint8_t byte, label ;
	int cnt, unread, plen, bytes_read;
	pmu buf ;
	
	while(byte != 0x7e) {
		cnt = read(m_fd,&byte,1) ;

		if(cnt != 1) {
			printf("read to much %i\n", cnt) ;
			return -1 ;
		}

	}

	// try to read the label
	cnt = read(m_fd,&label,1) ;
	
	if(cnt != 1) {
		printf("read to much %i\n", cnt);
		return 1 ;
	}

	cnt = read(m_fd, &byte,1);
	if (cnt != 1) { 
	    printf("read to much %i\n", cnt);
    	return 1;
	}
	plen = byte ;
	
	cnt = read(m_fd, &byte,1);
	if (cnt != 1) { 
	    printf("read to much %i\n", cnt);
    	return 1;
	}
	plen += byte<<8;

//	printf("packet length is %i\n", plen) ;

	for(bytes_read = 0; bytes_read < plen;) {
		bytes_read += read(m_fd, (uint8_t*) &buf +bytes_read, plen-bytes_read) ;
	}

//	printf("read %i bytes.\n", bytes_read) ;
	// check here

	// check this is a valid frame with an end byte
	cnt = read(m_fd, &byte,1);
	if (cnt != 1) { 
	    printf("read to much %i\n", cnt);
    	return 1;
	}

	if(byte == 0xe7) {
		// looks ok
//		printf("read %i bytes\n", plen) ;
	
		switch(label) {
			case ID_RDMX:
				w_handle_dmx( &buf.pmu_rdmx, cnt) ;
				break;
			case ID_PRMREP:
				w_handle_prmrep( &buf.pmu_prmrep, cnt) ;
				break;
			case ID_COS:
				w_handle_cos( &buf.pmu_cos, cnt) ;
				break;
			case ID_SNOREP:
				w_handle_snorep( &buf.pmu_snorep, cnt) ;
				break;
			default :
				printf("not sure what msg this is\n") ;
		}
	}

	ioctl(m_fd, FIONREAD, &unread) ;
//	printf("finished reading,  %d bytes remain\n", unread) ;


}




