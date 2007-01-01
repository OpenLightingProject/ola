/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * LlaClient.cpp
 * Implementation of LlaClient
 * Copyright (C) 2005-2006 Simon Newton
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>	
#include <arpa/inet.h>
#include <unistd.h>

#include <lla/messages.h>
#include <lla/LlaDevConfMsg.h>

#include "LlaClient.h"
#include "LlaClientObserver.h"
#include "LlaPlugin.h"
#include "LlaDevice.h"
#include "LlaPort.h"
#include "LlaUniverse.h"


#define min(a,b)    ((a) < (b) ? (a) : (b))
#define max(a,b)    ((a) > (b) ? (a) : (b))

const string LlaClient::LLAD_ADDR("127.0.0.1");


LlaClient::~LlaClient() {
	if(m_connected)
		stop();
}


/*
 * Open a connection to the LLA daemon. This call may block.
 *
 * @return 0 on sucess, non-0 on failure
 */
int LlaClient::start() {
	struct sockaddr_in servaddr;
	
	// init members
	m_connected = 0;
	m_seq = 0;

	//connect to socket
	m_sd = socket(AF_INET, SOCK_DGRAM, 0);

	if(!m_sd)
		goto e_socket;
	
	memset(&servaddr, 0x00, sizeof(servaddr) );
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(LLAD_PORT);
	inet_aton(LLAD_ADDR.c_str(), &servaddr.sin_addr);
	
	if (-1 == connect(m_sd, (struct sockaddr *) &servaddr, sizeof(servaddr) ) ) 
		goto e_connect;

	// send syn to server
	send_syn();

	while(!m_connected) {
		switch (receive(1)) {
			case 0:
				break;
			case 1:
				// timeout
				printf("failed to get syn ack\n");
				goto e_connect;
			case -1:
				// error
				goto e_connect;
			case -2:
				// interupt
				break;
		}
	}
	return 0;
		
e_connect:
	close(m_sd);

e_socket:
	return -1;
}


/*
 * Close the lla connection. This call may block.
 *
 * @return 0 on sucess, -1 on failure
 */
int LlaClient::stop() {
	
	send_fin();

	while(m_connected) {
		switch(receive(1)) {
			case 0:
				break;
			case 1:
				// timeout
				printf("failed to get fin ack\n");
				return -1;
			case -1:
				// error
				return -1;
			case -2:
				// interupt
				break;
		}
	}

	close(m_sd);
	clear_plugins();
	clear_devices();
	clear_universes();
	m_connected = false;
	return 0;
}


/*
 * return the socket descriptor
 *
 * @return -1 on error, the sd on sucess
 */
int LlaClient::fd() const {
	return m_sd;
}


/*
 * Call when there is action on the sd
 * 
 * delay sets a delay to wait for IO before returning, note this is a max delay,
 * we will return before this if we got data.
 *
 * @param delay		0 to non-block, else the delay to wait for data before returning
 * @return 0 on success, -1 on error
 */
int LlaClient::fd_action(unsigned int delay) {
	
	while(1) {
		switch(receive(delay)) {
			case 0:
				break;
			case 1:
				// timeout
				return 0;
			case -1:
				// error
				return -1;
			case -2:
				// interupt
				break;
		}
	}
	return 0;
}


/*
 * set the observer object
 */
int LlaClient::set_observer(LlaClientObserver *o) {
	// should we delete the old one or leave it to the client?
	m_observer = o;
	return 0;
}


/*
 * Write some dmx data
 *
 * @param 	c 		lla_con
 * @param 	uni 	universe to send to
 * @param	data	dmx data
 * @param 	length	length of dmx data
 * @return	0 on sucess, -1 on failure
 */
int LlaClient::send_dmx(unsigned int uni, uint8_t *data, unsigned int length) {
	lla_msg msg;
	
	msg.len = sizeof(lla_msg_dmx_data);
	msg.data.dmx.op = LLA_MSG_DMX_DATA;
	msg.data.dmx.uni = uni;
	msg.data.dmx.len = min(length, MAX_DMX);
	
	memcpy(&msg.data.dmx.data, data, msg.data.dmx.len);

	return send_msg(&msg);
}


/*
 * read dmx data
 *
 * Usually you want to call this in the dmx callback. Or you could
 * poll it I suppose.
 *
 */
int LlaClient::fetch_dmx(unsigned int universe) {
	//send read request
	lla_msg msg;
	
	msg.len = sizeof(lla_msg_uni_name);
	msg.data.rreq.op = LLA_MSG_READ_REQ;
	msg.data.rreq.uni = universe;
	
	return send_msg(&msg);
}

/*
 * Request a listing of what devices are attached
 *
 *
 */
int LlaClient::fetch_dev_info(lla_plugin_id filter) {
	lla_msg msg;
	
	msg.len = sizeof(lla_msg_device_info_request);
	msg.data.dreq.op = LLA_MSG_DEVICE_INFO_REQUEST;
	msg.data.dreq.plugin = filter;
	
	send_msg(&msg);
	
	clear_devices();
	return 0;
}


/*
 * send a port info request
 */
int LlaClient::fetch_port_info(LlaDevice *dev) {
	lla_msg msg;

	if(dev == NULL)
		return -1;
	
	msg.len = sizeof(lla_msg_port_info_request);
	
	msg.data.prreq.op = LLA_MSG_PORT_INFO_REQUEST;
	msg.data.prreq.devid = dev->get_id();
	send_msg(&msg);

	return 0;
}


/*
 * Request a universe listing
 *
 * @return 
 */
int LlaClient::fetch_uni_info() {
	lla_msg msg;
	
	msg.len = sizeof(lla_msg_plugin_info_request);
	msg.data.unireq.op = LLA_MSG_UNI_INFO_REQUEST;
	
	send_msg(&msg);
	
	clear_universes();
	return 0;
}


/*
 * Request a plugin info listing
 *
 *
 */
int LlaClient::fetch_plugin_info() {
	lla_msg msg;
	
	msg.len = sizeof(lla_msg_plugin_info_request);
	msg.data.plreq.op = LLA_MSG_PLUGIN_INFO_REQUEST;
	
	send_msg(&msg);
	
	clear_plugins();
	return 0;	
}


/*
 * This is supposed to give us info on what devices are out there
 *
 *
 */
int LlaClient::fetch_plugin_desc(LlaPlugin *plug) {
	lla_msg msg;

	if (plug == NULL)
		return -1;
	
	msg.len = sizeof(lla_msg_plugin_desc_request);
	msg.data.pldreq.op = LLA_MSG_PLUGIN_DESC_REQUEST;
	msg.data.pldreq.pid = plug->get_id();
	
	send_msg(&msg);
	return 0;
}


/*
 * Set the name of a universe
 *
 */
int LlaClient::set_uni_name(unsigned int uni, const string &name) {
	lla_msg msg;
	int l = min(name.length(), UNIVERSE_NAME_LENGTH);

	msg.len = sizeof(lla_msg_uni_name);
	msg.data.uniname.op = LLA_MSG_UNI_NAME;
	msg.data.uniname.uni = uni;
	
	strncpy(msg.data.uniname.name, name.c_str(), l);
	return send_msg(&msg);
}


/*
 * Set the merge mode of a universe
 *
 */
int LlaClient::set_uni_merge_mode(unsigned int uni, LlaUniverse::merge_mode mode) {
	lla_msg msg;

	msg.len = sizeof(lla_msg_uni_merge);
	msg.data.unimerge.op = LLA_MSG_UNI_MERGE;
	msg.data.unimerge.uni = uni;
	msg.data.unimerge.mode = mode;
	
	return send_msg(&msg);
}


/*
 * Register our interest in a universe, the observer object will then
 * be notifed when the dmx values in this universe change.
 *
 * @param uni	the universe id
 * @param action
 */
int LlaClient::register_uni(unsigned int uni, LlaClient::RegisterAction action) {
	lla_msg reg;
	
	reg.len = sizeof(lla_msg_register);
	reg.data.reg.op = LLA_MSG_REGISTER;
	reg.data.reg.uni = uni;

	if(action == REGISTER)
		reg.data.reg.action = LLA_MSG_REG_REG;
	else
		reg.data.reg.action = LLA_MSG_REG_UNREG;
	
	return send_msg(&reg);
}


/*
 * (Un)Patch a port to a universe
 *
 * @param dev 		the device id
 * @param port		the port id
 * @param action	LlaClient::PATCH or LlaClient::UNPATCH
 * @param uni		universe id
 */
int LlaClient::patch(unsigned int dev, unsigned int port, LlaClient::PatchAction action, unsigned int uni) {

	lla_msg msg;
	
	msg.len = sizeof(lla_msg_patch);
	msg.data.patch.op = LLA_MSG_PATCH;
	msg.data.patch.dev = dev;
	msg.data.patch.port = port;
	msg.data.patch.uni = uni;

	if (action == PATCH) {
		msg.data.patch.action = LLA_MSG_PATCH_ADD;
	} else {
		msg.data.patch.action = LLA_MSG_PATCH_REMOVE;
	}
	return send_msg(&msg);
}


/*
 * Sends a device config request
 *
 * @param dev 	the device id
 * @param req	the request buffer
 * @param len	the length of the request buffer
 */
int LlaClient::dev_config(unsigned int dev, const class LlaDevConfMsg *dmsg) {

	lla_msg msg;
	memset(&msg, 0x00, sizeof(msg));
	int l;
	int buf_len = sizeof(msg.data.devreq.req);

	if(dmsg == NULL)
		return -1;

	l = dmsg->pack(msg.data.devreq.req, sizeof(msg.data.devreq.req));

	if( l < 0)
		return -1;

	msg.data.devreq.op = LLA_MSG_DEV_CONFIG_REQ;
	msg.data.devreq.len = l;
	msg.data.devreq.devid = dev;
	msg.len = sizeof(lla_msg_device_config_req) - buf_len + l;
	msg.data.devreq.seq = m_seq++;

	return send_msg(&msg);
}


// private methods follow
//-----------------------------------------------------------------------------

/*
 * Do the receive thing...
 *
 * @param delay		the max delay to wait
 */
int LlaClient::receive(unsigned int delay) {
	struct timeval tv;
	fd_set rset;
	
	tv.tv_usec = 0;
	tv.tv_sec = delay;

	FD_ZERO(&rset);
	FD_SET(m_sd, &rset);

	switch ( select( m_sd+1, &rset, NULL, NULL, &tv) ) {
		case 0:
			// timeout
			return 1;
			break;
		case -1 :
			//error
			if( errno != EINTR) {
				printf("%s : select error", strerror(errno) );
				return -1;
			}
			// we were interupted
			return -2;
			break;
		default:
			// something to read
			return read_msg();
			break;
	}
}


/*
 * Send an msg on the connection
 *
 * @param msg	the msg to send
 * 
 * @return 0 on sucess, -1 on failure
 */
int LlaClient::send_msg(lla_msg *msg) {

	if ( 0 >= send(m_sd, &msg->data, msg->len, 0 ) ) {
		printf("send msg failure: %s\n", strerror(errno) );
		return -1;
	}
	return 0;
}


/*
 * receive a msg on the sd and handle it
 *
 */
int LlaClient::read_msg() {
	lla_msg msg;
	int len;

	if ( (len = recv(m_sd, &msg.data, sizeof(lla_msg_data), 0 )< 0) ) {
		// call err func here
		printf("Error in recv from: %s\n", strerror(errno));
		return -1;
	}

	return handle_msg(&msg);
}


// datagram handlers
//-----------------------------------------------------------------------------

/*
 * handle a msg
 *
 */
int LlaClient::handle_msg(lla_msg *msg) {
	
	// we're only interested in a couple of msgs here
	switch(msg->data.dmx.op) {
		case LLA_MSG_SYN_ACK:
			return handle_syn_ack(msg);
		case LLA_MSG_FIN_ACK:
			return handle_fin_ack(msg);
		case LLA_MSG_DMX_DATA :
			return handle_dmx(msg);
			break;
		case LLA_MSG_PLUGIN_INFO :
			return handle_plugin_info(msg);
			break;
		case LLA_MSG_DEVICE_INFO :
			return handle_dev_info(msg);
			break;
		case LLA_MSG_PORT_INFO :
			return handle_port_info(msg);
			break;
		case LLA_MSG_PLUGIN_DESC :
			return handle_plugin_desc(msg);
			break;
		case LLA_MSG_UNI_INFO :
			return handle_universe_info(msg);
			break;
		case LLA_MSG_DEV_CONFIG_REP:
			return handle_dev_conf(msg);
			break;
		default:
			printf("msg recv'ed but we're not interested, opcode %d\n", msg->data.dmx.op);
			break;
	}
	return 0;
}


/*
 *
 */
int LlaClient::handle_dmx(lla_msg *msg) {

	// notify observer
	if(m_observer != NULL) {
		// TODO: keep a map of registrations and filter
		m_observer->new_dmx(msg->data.dmx.uni ,msg->data.dmx.len, msg->data.dmx.data);
	}
	return 0;
}


/*
 * A syn ack marks us as connected
 */
int LlaClient::handle_syn_ack(lla_msg *msg) {
	if(!m_connected) {
		m_connected = 1;
	}
	msg = NULL;
	return 0;
}


/*
 * A fin ack marks us as connected
 */
int LlaClient::handle_fin_ack(lla_msg *msg) {

	if(m_connected) {
		m_connected = 0;
	}
	msg = NULL;
	return 0;
}


/*
 * Get info on the plugins loaded
 * TODO: support framenation
 */
int LlaClient::handle_plugin_info(lla_msg *msg) {
	int i, plugins;
	LlaPlugin *plug = NULL;

	plugins = min(msg->data.plinfo.nplugins, PLUGINS_PER_DATAGRAM);

	clear_plugins();

	for(i=0; i < plugins; i++) {
		int id = msg->data.plinfo.plugins[i].id;
		string name(msg->data.plinfo.plugins[i].name);
		plug = new LlaPlugin(id, name);
		m_plugins.push_back(plug);
	}

	if(m_observer != NULL) 
		m_observer->plugins(m_plugins);

	return 0;
}


/*
 * Handle a universe info message
 * TODO: support fragmentation
 * 
 */
int LlaClient::handle_universe_info(lla_msg *msg) {
	int i, universes;
	LlaUniverse *uni = NULL;

	universes = min(msg->data.uniinfo.nunis, UNIVERSES_PER_DATAGRAM);

	clear_universes();

	for(i=0; i < universes; i++) {
		int id = msg->data.uniinfo.universes[i].id;
		LlaUniverse::merge_mode mode = msg->data.uniinfo.universes[i].merge == UNI_MERGE_MODE_HTP ? LlaUniverse::MERGE_HTP : LlaUniverse::MERGE_LTP;
		string name(msg->data.uniinfo.universes[i].name);

		uni = new LlaUniverse(id, mode, name);

		if(uni == NULL) {
			printf("malloc failed\n");
			return -1;
		}
		m_unis.push_back(uni);
	}

	if(m_observer != NULL) 
		m_observer->universes(m_unis);

	return 0;
}


/*
 * Handle a device info message
 * TODO: support fragmentation
 * 
 */
int LlaClient::handle_dev_info(lla_msg *msg) {
	int i, devs;
	LlaDevice *dev = NULL;
	
	devs = min(msg->data.dinfo.ndevs, DEVICES_PER_DATAGRAM);

	for(i=0; i < devs; i++) {
		int id = msg->data.dinfo.devices[i].id;
		int pid = msg->data.dinfo.devices[i].plugin;
		string name(msg->data.dinfo.devices[i].name);
		int count = msg->data.dinfo.devices[i].ports;
		dev = new LlaDevice(id, count, name, pid) ;

		m_devices.push_back(dev);
	}

	if(m_observer != NULL) 
		m_observer->devices(m_devices);

	return 0;
}


/*
 * port info message
 * 
 */
int LlaClient::handle_port_info(lla_msg *msg) {
	int i, ports;
	LlaPort *prt  = NULL;
	LlaDevice *dev = NULL;
	vector<LlaDevice *>::iterator iter;

	for( iter = m_devices.begin(); iter != m_devices.end(); iter++) {
		if ((*iter)->get_id() == msg->data.prinfo.dev) {
			dev = *iter;
			break;
		}
	}

	if( dev == NULL) {
		printf("Could not locate device %d\n", msg->data.prinfo.dev);
		return -1;
	}
	
	dev->reset_ports();
	
	ports = min(msg->data.prinfo.nports, DEVICES_PER_DATAGRAM);

	for(i=0; i < ports; i++) {
		int id = msg->data.prinfo.ports[i].id;
		LlaPort::PortCapability cap = msg->data.prinfo.ports[i].cap == LLA_MSG_PORT_CAP_IN ? LlaPort::LLA_PORT_CAP_IN : LlaPort::LLA_PORT_CAP_OUT;
		int uni = msg->data.prinfo.ports[i].uni;
		int active = msg->data.prinfo.ports[i].actv;

		prt = new LlaPort(id, cap, uni, active);

		if(prt == NULL) {
			printf("malloc failed\n");
			return -1;
		}
		dev->add_port(prt);
	}

	if(m_observer != NULL) 
		m_observer->ports(dev);

	return 0;
}


/*
 * handle plugin desc response 
 */
int LlaClient::handle_plugin_desc(lla_msg *msg) {
	LlaPlugin *plug  = NULL;
	vector<LlaPlugin *>::iterator iter;

	for( iter = m_plugins.begin(); iter != m_plugins.end(); iter++) {
		if ((*iter)->get_id() == msg->data.pldesc.pid) {
			plug = *iter;
			break;
		}
	}

	if( plug == NULL) {
		printf("Could not locate plugin %d\n", msg->data.pldesc.pid);
		return -1;
	}

	string desc(msg->data.pldesc.desc, PLUGIN_DESC_LENGTH);
	plug->set_desc(desc);
	
	if(m_observer != NULL) 
		m_observer->plugin_desc(plug);

	return 0;
}



/*
 * handle a device config response
 * 
 */
int LlaClient::handle_dev_conf(lla_msg *msg) {
	if(m_observer != NULL)
		m_observer->dev_config(msg->data.devrep.dev, msg->data.devrep.rep, msg->data.devrep.len);

	return 0;
}

// datagram senders
//-----------------------------------------------------------------------------



/*
 * send a syn to the server
 */
int LlaClient::send_syn() {
	lla_msg msg;
	
	msg.len = sizeof(lla_msg_syn);
	msg.data.syn.op = LLA_MSG_SYN;

	return send_msg(&msg);
}

/*
 * send a fin to the server
 */
int LlaClient::send_fin() {
	lla_msg msg;
	
	msg.len = sizeof(lla_msg_fin);
	msg.data.fin.op = LLA_MSG_FIN;

	return send_msg(&msg);
}


/*
 * clear the plugin list
 *
 */
int LlaClient::clear_plugins() {
	vector<LlaPlugin *>::iterator iter;

	for(iter = m_plugins.begin(); iter != m_plugins.end(); ++iter) {
		delete *iter;
	}
	m_plugins.clear();
	return 0;
}


/*
 * clear the universe list
 *
 */
int LlaClient::clear_universes() {
	vector<LlaUniverse *>::iterator iter;

	for(iter = m_unis.begin(); iter != m_unis.end(); ++iter) {
		delete *iter;
	}
	m_unis.clear();
	return 0;
}


/*
 * Free the device (including all ports)
 *
 */
int LlaClient::clear_devices() {
	vector<LlaDevice *>::iterator iter;

	for(iter = m_devices.begin(); iter != m_devices.end(); ++iter) {
		delete *iter;
	}
	m_devices.clear();
	return 0;
}
