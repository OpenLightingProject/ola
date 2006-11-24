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

#include <lla/logger.h>
#include <lla/preferences.h>
#include <lla/universe.h>
#include <usbpro_messages.h>

#include "UsbProConfParser.h"
#include "usbprodevice.h"
#include "usbproport.h"

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#define min(a,b) a<b?a:b

static const uint8_t RCMODE_ALWAYS = 0x00;
static const uint8_t RCMODE_CHANGE = 0x01;

static const uint8_t som = 0x7e;
static const uint8_t eom = 0xe7;

enum usbpro_packet_type_e {
	ID_PRMREQ = 0x03,
	ID_PRMREP =	0x03,
	ID_PRMSET = 0x04,
	ID_RDMX	=	0x05,
	ID_SDMX	=	0x06,
	ID_RDM = 	0x07,
	ID_RCMODE =	0x08,
	ID_COS =	0x09,
	ID_SNOREQ =	0x0A,
	ID_SNOREP =	0x0A
} __attribute__( ( packed ) );

typedef enum usbpro_packet_type_e usbpro_packet_type;


/*
 * Create a new device
 *
 * @param owner	the plugin that owns this device
 * @param name	the device name
 * @param dev_path	path to the pro widget
 */
UsbProDevice::UsbProDevice(Plugin *owner, const string &name, const string dev_path) :
	Device(owner, name),
	m_dev_path(dev_path),
	m_fd(-1),
	m_enabled(false) {
}


/*
 * Destroy this device
 */
UsbProDevice::~UsbProDevice() {
	if (m_enabled)
		stop();
}


/*
 * Start this device
 */
int UsbProDevice::start() {
	UsbProPort *port = NULL;
	Port *prt = NULL;
	int debug = 0;
	int ret;
	
	/* set up ports */
	for(int i=0; i < 2; i++) {
		port = new UsbProPort(this,i);

		if(port != NULL) 
			this->add_port(port);
	}
	
	// create new usbpro node, and and set config values
	ret = w_connect(m_dev_path);

	if(ret) {
		Logger::instance()->log(Logger::WARN, "UsbProPlugin: failed to connect to %s", m_dev_path.c_str()  );
		goto e_dev;
	}

	if (w_init()) {
		Logger::instance()->log(Logger::WARN, "UsbProPlugin: init failed , is this an pro widget?" );
		goto e_disconnect;
	}

	// clear buffer
	memset(m_dmx, 0x00, DMX_BUF_LEN);
	m_enabled = true;
	return 0;

e_disconnect:
	w_disconnect();

e_dev:
	for(int i=0; i < port_count() ; i++) {
		prt = get_port(i);
		if(prt != NULL) 
			delete prt;
	}
	return -1;
}


/*
 * Stop this device
 */
int UsbProDevice::stop() {
	Port *prt = NULL;
	Universe *uni;
	
	if (!m_enabled)
		return 0;

	// disconnect from widget
	w_disconnect();
	
	for(int i=0; i < port_count() ; i++) {
		prt = get_port(i);
		if(prt != NULL) {
			uni = prt->get_universe();

			if(uni) 
				uni->remove_port(prt);
				
			delete prt;
		}
	}

	m_enabled = false ;
	return 0;
}


/*
 * return the sd for this device
 */
int UsbProDevice::get_sd() const {
	return m_fd;
}


/*
 * Called when there is activity on our descriptors
 */
int UsbProDevice::fd_action() {
	int unread;

	// check if there is data to be read 
	if ( ioctl( m_fd, FIONREAD , &unread) ) {
		// the device has been removed
		Logger::instance()->log(Logger::WARN, "UsbProPlugin: device removed" );
		return -1;
	}

	while (unread != 0) {
		w_recv();
		ioctl(m_fd, FIONREAD, &unread);
	}
	return 0;
}


/*
 * Send the dmx out the widget
 * called from the UsbProPort
 *
 * @return 	0 on success, non 0 on failure
 */
int UsbProDevice::send_dmx(uint8_t *data, int len) {
	return w_send_dmx(data,len);
}


/*
 * Copy the dmx buffer into the arguments
 * Called from the UsbProPort
 *
 * @return 	the length of the dmx data copied
 */
int UsbProDevice::get_dmx(uint8_t *data, int len) const {

	int l = min(len, DMX_BUF_LEN-1);

	// byte 0 is the start code which we ignore
	memcpy(data,m_dmx+1, l);

	return l;
}


// call this when something changes
// where to store data to ?
// I'm thinking a config file in /etc/llad/llad.conf
int UsbProDevice::save_config() const {


	return 0;
}


/*
 * For now we can't block in a configure call, so we keep
 * a cache of the widget parameters and return those.
 *
 * @param req		the request data
 * @param reql		the request length
 * @param rep		buffer for the reply
 * @param repl		the length of the reply buffer
 *
 * @return	the length of the reply
 */
int UsbProDevice::configure(const void *request, int reql, void *reply, int repl) {
	lla_usbpro_msg *req = (lla_usbpro_msg *) request;
	lla_usbpro_msg *rep = (lla_usbpro_msg*) reply;

	UsbProConfParser parser = UsbProConfParser() ;
	UsbProConfMessage m = parser.parse(request, reql);


/*	switch(req->op) {
		case LLA_USBPRO_MSG_GPRMS:
			return config_get_params(req, reql, rep, repl);
		case LLA_USBPRO_MSG_SER:
			return config_get_serial(req, reql, rep, repl);
		case LLA_USBPRO_MSG_SPRMS:
			return config_set_params(req, reql, rep, repl);
		default:
			Logger::instance()->log(Logger::WARN ,"Invalid request to usbpro configure %i", req->op);
			return -1;
	}
*/
}


//-----------------------------------------------------------------------------
// Private methods used for communicating with the widget

/*
 * Connect to the widget
 */
int UsbProDevice::w_connect(const string &dev) {
	struct termios oldtio, newtio;
	m_fd = open(dev.c_str(), O_RDWR | O_NONBLOCK);

	if(m_fd == -1) {
		return 1 ;
	}

	bzero(&newtio, sizeof(newtio)); // clear struct for new port settings
	tcsetattr(m_fd,TCSANOW,&newtio);
	return 0;
}


/*
 * Disconnect from the widget
 */
int UsbProDevice::w_disconnect() {
	if(m_fd)
		close(m_fd);

	m_fd = -1;
	return 0;
}


/*
 * We perform a couple of tests here to make sure this behaves like we expect
 */
int UsbProDevice::w_init() {
	struct timespec tv;

	tv.tv_sec = 0;
	tv.tv_nsec = 1000000;

	// set a serial number and params request
	w_send_prmreq(0);

	nanosleep(&tv, NULL);

	w_send_snoreq();

	// put us into receiving mode
	w_send_rcmode(1);

	return 0;
}


/*
 * Set the length of the msg
 */
int UsbProDevice::w_set_msg_len(promsg *msg, int len) const {
	msg->len = len & 0xFF;
	msg->len_hi = (len & 0xFF00) >> 8;
}


/*
 * Send the msg
 */
int UsbProDevice::w_send_msg(promsg *msg) const {

	int len = (msg->len_hi << 8) + msg->len ;
	write(m_fd, msg, len+4) ;
	write(m_fd, &eom, sizeof(eom) ) ;
}

/*
 * Send a dmx msg
 */
int UsbProDevice::w_send_dmx(uint8_t *buf, int len) const {
	int l = min(DMX_BUF_LEN, len);
	promsg msg;

	msg.som = som;
	msg.label = ID_SDMX;
	w_set_msg_len(&msg, l+1);

	//start code to 0
	msg.pm_dmx.dmx[0] = 0;
	memcpy(&msg.pm_dmx.dmx[1], buf, l);

	return w_send_msg(&msg);
}


/*
 * Send a rdm msg, rdm support is a bit sucky
 *
 */
int UsbProDevice::w_send_rdm(uint8_t *buf, int len) const {
	promsg msg;
	msg.som = som;
	msg.label = ID_RDM;
	w_set_msg_len(&msg, len);
	memcpy(&msg.pm_rdm.dmx, buf, len);

	return w_send_msg(&msg);
}


/*
 * Send a get param request
 *
 * @param usrsz	size of user configurable memory to fetch
 */
int UsbProDevice::w_send_prmreq(int usrsz) const {
	promsg msg;
	msg.som = som;
	msg.label = ID_PRMREQ;
	w_set_msg_len(&msg, sizeof(pms_prmreq));
	msg.pm_prmreq.len = usrsz & 0xFF;
	msg.pm_prmreq.len_hi = (usrsz & 0xFF) >> 8;
	return w_send_msg(&msg);
}


/*
 * Send a set param request
 *
 * @param usrsz	size of user configurable memory to fetch
 */
int UsbProDevice::w_send_prmset(uint8_t *data, int len, uint8_t brk, uint8_t mab, uint8_t rate) const {
	int l = min(USER_CONFIG_LEN, len);
	promsg msg;
	msg.som = som;
	msg.label = ID_PRMSET;
	w_set_msg_len(&msg, sizeof(pms_prmset) - USER_CONFIG_LEN + l);
	msg.pm_prmset.len = len & 0xFF;
	msg.pm_prmset.len_hi = (len & 0xFF) >> 8;
	msg.pm_prmset.brk = brk;
	msg.pm_prmset.mab = mab;
	msg.pm_prmset.rate = rate;
	memcpy(msg.pm_prmset.user, data, l) ;
	return w_send_msg(&msg);
}


/*
 * Send a mode msg
 * @param mode the mode to change to
 */
int UsbProDevice::w_send_rcmode(int mode) {
	int ret;
	uint8_t m = mode ? RCMODE_CHANGE : RCMODE_ALWAYS;
	promsg msg;

	msg.som = som;
	msg.label = ID_RCMODE;
	w_set_msg_len(&msg, sizeof(pms_rcmode));
	msg.pm_rcmode.mode = mode;

	ret = w_send_msg(&msg);

	if(!ret && m) 
		memset(m_dmx, 0x00, DMX_BUF_LEN);
	return ret;
}


/*
 * Send a serial number request
 */
int UsbProDevice::w_send_snoreq() const {
	promsg msg;
	msg.som = som;
	msg.label = ID_SNOREQ;
	w_set_msg_len(&msg, sizeof(pms_snoreq));
	
	return w_send_msg(&msg);
}


/*
 * Handle the dmx frame
 * Don't do anything as we expect cos messages instead
 */
int UsbProDevice::w_handle_dmx(pms_rdmx *dmx, int len) {

//	printf(" stc %hhx  %hhx %hhx\n", dmx->dmx[0], dmx->dmx[1], dmx->dmx[2]) ;
	return 0;
}


/*
 * Handle the dmx change of state frame
 */
int UsbProDevice::w_handle_cos(pms_cos *cos, int len) {
	int chn_st = cos->start *8;
	int i, offset;
	
	//get our input port
	Port *prt = get_port(0);
	
	// should be checking length here
	offset = 0;
	for(i = 0; i< 40; i++) {

		if(chn_st+i >= DMX_BUF_LEN-1 || offset + 6 >= len)
			break;

		if( cos->changed[i/8] & (1 << (i%8)) ) {
			m_dmx[chn_st + i] = cos->data[offset];
			offset++;
		}
	}

	prt->dmx_changed();
	return 0;
}


/*
 * Handle the param reply
 * 
 * @param rep parameters message
 * @param len length of the message
 */
int UsbProDevice::w_handle_prmrep(pms_prmrep *rep, int len) {

	if( len >= 5 && len <= sizeof(pms_prmrep) ) {
		int frmvr = rep->firmv + (rep->firmv_hi <<8) ;
		memcpy(&m_params, rep, sizeof(pms_prmrep));
	}
}


/*
 * Handle the serial number reply
 *
 * @param rep serial number message
 * @param len length of the message
 */
int UsbProDevice::w_handle_snorep(pms_snorep *rep, int len) {	
	
	if( len == sizeof(pms_snorep) ) {
		// copy into buffer
		memcpy(m_serial , rep->srno, 4);
	}
}

/*
 * Receive data from the widget
 */
int UsbProDevice::w_recv() {
	uint8_t byte = 0x00;
	uint8_t label;
	int cnt, plen, bytes_read;
	pmu buf;
	
	while(byte != 0x7e) {
		cnt = read(m_fd,&byte,1);

		if(cnt != 1) {
			printf("1, read to much %i\n", cnt);
			return -1;
		}
	}

	// try to read the label
	cnt = read(m_fd,&label,1);
	
	if(cnt != 1) {
		printf("2, could not read label %i\n", cnt);
		return 1 ;
	}

	cnt = read(m_fd, &byte,1);
	if (cnt != 1) { 
	    printf("3, could not read len hi%i\n", cnt);
    	return 1;
	}
	plen = byte ;
	
	cnt = read(m_fd, &byte,1);
	if (cnt != 1) { 
	    printf("4, could not read len lo %i\n", cnt);
    	return 1;
	}
	plen += byte<<8;

	for(bytes_read = 0; bytes_read < plen;) {
		bytes_read += read(m_fd, (uint8_t*) &buf +bytes_read, plen-bytes_read);
	}

	// check this is a valid frame with an end byte
	cnt = read(m_fd, &byte,1);
	if (cnt != 1) { 
	    printf("5, read to much %i\n", cnt);
    	return 1;
	}

	if(byte == 0xe7) {
	
		switch(label) {
			case ID_RDMX:
				w_handle_dmx( &buf.pmu_rdmx, plen);
				break;
			case ID_PRMREP:
				w_handle_prmrep( &buf.pmu_prmrep, plen);
				break;
			case ID_COS:
				w_handle_cos( &buf.pmu_cos, plen);
				break;
			case ID_SNOREP:
				w_handle_snorep( &buf.pmu_snorep, plen);
				break;
			default :
				printf("not sure what msg this is\n");
		}
	}
}


/*
 * Handle a params config request
 */
int UsbProDevice::config_get_params(lla_usbpro_msg *msg, int msgl, lla_usbpro_msg *rep, int repl) const {
	rep->op = LLA_USBPRO_MSG_GPRMS_R;
	rep->data.gprmsr.brk = m_params.brtm;
	rep->data.gprmsr.mab = m_params.mabtm;
	rep->data.gprmsr.rate = m_params.rate;
	return sizeof(lla_usbpro_msg_gprms_r) + 1; 
}


/*
 * Handle a serial number config request
 */
int UsbProDevice::config_get_serial(lla_usbpro_msg *msg, int msgl, lla_usbpro_msg *rep, int repl) const {
	rep->op = LLA_USBPRO_MSG_SER;
	memcpy(rep->data.ser.serial , m_serial, 4);
	return sizeof(lla_usbpro_msg_ser) + 1; 
}

/*
 * Handle a set param config request
 */
int UsbProDevice::config_set_params(lla_usbpro_msg *msg, int msgl, lla_usbpro_msg *rep, int repl) {

	rep->op = LLA_USBPRO_MSG_SPRMS_R;

	if(msgl == sizeof(lla_usbpro_msg_sprms_s) + 1) {
		w_send_prmset(NULL, 0, msg->data.sprms.brk, msg->data.sprms.mab, msg->data.sprms.rate);
		w_send_prmreq(0);


	}
	return 0;
}
