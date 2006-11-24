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
 * lla.c
 * Main file for liblla
 * Copyright (C) 2005  Simon Newton
 */

#include <lla/lla.h>
#include <lla/messages.h>
#include "private.h"

#include <stdio.h>
#include <errno.h>

static int send_msg(lla_connection *con, lla_msg *msg) ;
static int read_msg(lla_connection *con) ;
static int handle_msg(lla_connection *con, lla_msg *msg) ;
static int handle_dmx(lla_connection *con, lla_msg *msg) ;
static int handle_syn_ack(lla_connection *con, lla_msg *msg) ;
static int handle_fin_ack(lla_connection *con, lla_msg *msg) ;
static int handle_dev_info(lla_connection *con, lla_msg *msg) ;
static int handle_plugin_info(lla_connection *con, lla_msg *msg) ;
static int handle_port_info(lla_connection *con, lla_msg *msg) ;
static int handle_plugin_desc(lla_connection *con, lla_msg *msg) ;
static int handle_universe_info(lla_connection *con, lla_msg *msg) ;

static int send_syn(lla_connection *con) ;
static int send_fin(lla_connection *con) ;
static int send_port_info_req(lla_connection *con, lla_device *dev) ;

static int free_plugins(lla_connection *con) ;
static int free_universes(lla_connection *con) ;
static int free_devices(lla_connection *con) ;
static int free_ports(lla_device *dev) ;

static int lla_recv(lla_connection *con, int delay) ;

/*
 * open a connection to the daemon
 *
 * @return lla_con on sucess, NULL on failure
 */
lla_con lla_connect() {
	struct sockaddr_in servaddr ;
	lla_connection *c = malloc(sizeof(lla_connection)) ;
	
	if(c == NULL)
		goto e_malloc ;

	// init members
	c->connected = 0 ;
	c->dmx_c.fh = NULL ;
	c->dmx_c.data = NULL ;
	c->devices = NULL ;
	c->plugins = NULL ;
	c->universes = NULL ;
	c->desc = NULL ;
	c->seq = 0;

	//connect to socket
	c->sd = socket(AF_INET, SOCK_DGRAM, 0) ;

	if(!c->sd)
		goto e_socket ;
	
	memset(&servaddr, 0x00, sizeof(servaddr) );
	servaddr.sin_family = AF_INET ;
	servaddr.sin_port = htons(LLAD_PORT) ;
	inet_aton(LLAD_ADDR, &servaddr.sin_addr) ;
	
	if (-1 == connect(c->sd, (struct sockaddr *) &servaddr, sizeof(servaddr) ) ) 
		goto e_connect ;

	// send syn to server
	send_syn(c) ;

	while(!c->connected) {
		switch(lla_recv(c, 1)) {
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

	return (lla_con) c ;
		
	// failed to connect

e_connect:
	close(c->sd) ;

e_socket:
	free(c) ;
e_malloc:
	return NULL ;
}


/*
 * Close the lla connection
 *
 * @param c lla_con
 * @return 0 on sucess, -1 on failure
 */
int lla_disconnect(lla_con c) {
	lla_connection *con = (lla_connection*) c ;

	return_if_null(con) ;
	
	send_fin(con) ;

	while(con->connected) {
		switch(lla_recv(c, 1)) {
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

	close(con->sd) ;
	free_plugins(con);
	free_devices(con);
	free(con->desc) ;
	free(con) ;

	return 0;
}


/*
 * return the socket descriptor
 *
 * @param c lla_con
 * @return -1 on error, the sd on sucess
 */
int lla_get_sd(lla_con c) {
	lla_connection *con = (lla_connection*) c ;
	
	return_if_null(con) ;

	return con->sd ;
}


/*
 * Call when there is action on the sd
 * 
 * delay sets a delay to wait for IO before returning, note this is a max delay,
 * we may return before this if we were interupted.
 *
 * @param delay	0 if not to block, else a delay to wait for data before returning
 * @return 0 on success, -1 on error
 *
 */
int lla_sd_action(lla_con c, int delay) {
	lla_connection *con = (lla_connection*) c ;
	
	return_if_null(con) ;
	
	while(1) {
		switch(lla_recv(con, delay)) {
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
 *
 *
 */
static int lla_recv(lla_connection *con, int delay) {
	struct timeval tv;
	fd_set rset;
	
	tv.tv_usec = 0 ;
	tv.tv_sec = delay ;

	FD_ZERO(&rset) ;
	FD_SET(con->sd, &rset) ;

	switch ( select( con->sd+1, &rset, NULL, NULL, &tv ) ) {
		case 0:
			// timeout
			return 1;
			break ;
		case -1 :
			//error
			if( errno != EINTR) {
				printf("%s : select error", strerror(errno) ) ;
				return -1;
			}
			// we were interupted
			return -2 ;
			break ;
		default:
			// ok something there to read
			return read_msg(con) ;
			break;
	}
}


/*
 * set the dmx callback
 *
 *
 *
 */
int lla_set_dmx_handler(lla_con c, int (*fh)(lla_con c, int uni, int length, uint8_t *data, void *d), void *data ) {
	lla_connection *con = (lla_connection*) c ;

	return_if_null(con) ;

	if(fh == NULL) {
		con->dmx_c.fh = NULL ;
		con->dmx_c.data = NULL ;
	} else {
		con->dmx_c.fh = fh ;
		con->dmx_c.data = data ;
	}

	return 0;

}

/*

int lla_set_rdm_handler(lla_con c, ) {
	lla_connection *con = (lla_connection*) c ;

	if(con == NULL) 
		return -1 ;



}
*/


/*
 * Register interest in a universe
 *
 */

int lla_reg_uni(lla_con c, int uni, int action) {
	lla_connection *con = (lla_connection*) c ;
	lla_msg reg ;
	
	return_if_null(con) ;

	reg.len = sizeof(lla_msg_register) ;

	// new reg packet
	reg.data.reg.op = LLA_MSG_REGISTER ;
	reg.data.reg.uni = uni ;

	if(action) 
		reg.data.reg.action = LLA_MSG_REG_REG ;
	else
		reg.data.reg.action = LLA_MSG_REG_UNREG ;
	
	// send
	return send_msg(con, &reg) ;

}


/*
 * set the name of this universe
 *
 */
int lla_set_name(lla_con c, int uni, char *name) {
	lla_connection *con = (lla_connection*) c ;
	lla_msg msg ;
	
	return_if_null(con) ;

	msg.len = sizeof(lla_msg_uni_name) ;

	// new reg packet
	msg.data.uniname.op = LLA_MSG_UNI_NAME ;
	msg.data.uniname.uni = uni ;
	
	if(name != NULL) {
		strncpy(msg.data.uniname.name, name , UNIVERSE_NAME_LENGTH) ;
	
		// send
		return send_msg(con, &msg) ;
	} 

	return -1 ;
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
int lla_send_dmx(lla_con c, int uni, uint8_t *data, int length) {
	lla_connection *con = (lla_connection*) c ;
	lla_msg msg ;
	
	return_if_null(con) ;

	msg.len = sizeof(lla_msg_dmx_data) ;
	msg.data.dmx.op = LLA_MSG_DMX_DATA ;
	msg.data.dmx.uni = uni;
	msg.data.dmx.len = min(length, MAX_DMX) ;
	
	memcpy(&msg.data.dmx.data, data, msg.data.dmx.len) ;

	return send_msg(con, &msg) ;
}

/*
 *
 *
 */
/*
int lla_send_rdm(lla_con c, int universe, uint8_t *data, int length) {
	lla_connection *con = (lla_connection*) c ;
	
	if(con == NULL) 
		return -1 ;
}
*/	

/*
 * read dmx data
 *
 * Usually you want to call this in the dmx callback. Or you could
 * poll it I suppose.
 *
 */
int lla_read_dmx(lla_con c, int universe) {
	//send read request
	lla_connection *con = (lla_connection*) c ;
	lla_msg msg ;
	
	return_if_null(con) ;
	
	msg.len = sizeof(lla_msg_uni_name) ;

	// new read request packet
	msg.data.rreq.op = LLA_MSG_READ_REQ ;
	msg.data.rreq.uni = universe ;
	
	// send
	return send_msg(con, &msg) ;
}


/*
 * This is supposed to give us info on what devices are out there
 *
 *
 */
lla_plugin *lla_req_plugin_info(lla_con c) {
	lla_connection *con = (lla_connection*) c ;
	lla_msg msg ;
	
	if(con == NULL)
		return NULL ;
	
	msg.len = sizeof(lla_msg_plugin_info_request) ;
	msg.data.plreq.op = LLA_MSG_PLUGIN_INFO_REQUEST ;
	
	send_msg(con, &msg) ;
	
	free_plugins(con) ;
	
	while(con->plugins == NULL) {
		switch(lla_recv(c, 1)) {
			case 0:
				break;
			case 1:
				// timeout
				printf("failed to get plugin info\n");
				return NULL;
			case -1:
				// error
				return NULL;
			case -2:
				// interupt
				break;
		}
	}

	// this will be null if we didn't get a response in time
	return con->plugins ;
	
}



/*
 * This is supposed to give us info on what devices are out there
 *
 *
 */
lla_device *lla_req_dev_info(lla_con c, lla_plugin_id filter) {
	lla_connection *con = (lla_connection*) c ;
	lla_msg msg ;
	
	if(con == NULL)
		return NULL ;

	msg.len = sizeof(lla_msg_device_info_request) ;
	msg.data.dreq.op = LLA_MSG_DEVICE_INFO_REQUEST ;
	msg.data.dreq.plugin = filter;
	
	send_msg(con, &msg) ;
	
	free_devices(con) ;

	while(con->devices == NULL) {
		switch(lla_recv(c, 1)) {
			case 0:
				break;
			case 1:
				// timeout
				printf("failed to get device info\n");
				return NULL;
			case -1:
				// error
				return NULL;
			case -2:
				// interupt
				break;
		}
	}

	return con->devices;
}


/*
 * This is supposed to give us info on what devices are out there
 *
 *
 */
char *lla_req_plugin_desc(lla_con c, int pid) {
	lla_connection *con = (lla_connection*) c ;
	lla_msg msg ;
	
	if(con == NULL)
		return NULL ;
	
	free(con->desc) ;
	con->desc = NULL;

	msg.len = sizeof(lla_msg_plugin_desc_request) ;
	msg.data.pldreq.op = LLA_MSG_PLUGIN_DESC_REQUEST ;
	msg.data.pldreq.pid = pid ;
	
	send_msg(con, &msg) ;
		
	while(con->desc == NULL) {
		switch(lla_recv(c, 1)) {
			case 0:
				break;
			case 1:
				// timeout
				printf("failed to get plugin desc\n");
				return NULL;
			case -1:
				// error
				return NULL;
			case -2:
				// interupt
				break;
		}
	}

	// this will be null if we didn't get a response in time
	return con->desc ;
	
}


/*
 * This is supposed to give us info on what universes are in use
 *
 * @param	c	the lla connection
 * @param 	head	address of a pointer (result argument)
 *
 * @return 
 */
int lla_req_universe_info(lla_con c, lla_universe **head) {
	lla_connection *con = (lla_connection*) c ;
	lla_msg msg ;
	
	if(con == NULL)
		return -1 ;
	
	msg.len = sizeof(lla_msg_plugin_info_request) ;
	msg.data.unireq.op = LLA_MSG_UNI_INFO_REQUEST ;
	
	send_msg(con, &msg) ;
	
	free_universes(con) ;
	
	while(con->universes == NULL) {
		// this isn't a real timeout, lots of traffic will cause us to loop here
		// without returning
		switch(lla_recv(c, 1)) {
			case 0:
				break;
			case 1:
				// timeout
				printf("failed to get universe info\n");
				return -1;
			case -1:
				// error
				return -1;
			case -2:
				// interupt
				break;
		}
	}

	if( con->universes->id == -1) {
		*head = NULL;
		return 0 ;
	}

	*head = con->universes ;
	return 0;
}


int lla_patch(lla_con c, int dev, int port, int action, int uni) {

	lla_connection *con = (lla_connection*) c ;
	lla_msg msg ;
	
	return_if_null(con) ;

	msg.len = sizeof(lla_msg_patch) ;
	msg.data.patch.op = LLA_MSG_PATCH ;
	msg.data.patch.dev = dev ;
	msg.data.patch.port = port ;
	msg.data.patch.uni = uni ;

	if (action) {
		msg.data.patch.action = LLA_MSG_PATCH_ADD ;
	} else {
		msg.data.patch.action = LLA_MSG_PATCH_REMOVE ;
	}
	return send_msg(con, &msg) ;

	return 0;

}


/*
 * sends a device config request
 *
 */
int lla_device_config(lla_con c, int dev, const uint8_t *req, int len) {

	lla_connection *con = (lla_connection*) c ;
	lla_msg msg ;
	int buf_len = sizeof(msg.data.devreq.req) ;

	return_if_null(con) ;

	if(len > buf_len)
		return 0;

	msg.len = sizeof(lla_msg_device_config_req) - buf_len + len ;
	printf("len issss %i %i %i\n", msg.len, sizeof(lla_msg_device_config_req) , buf_len  ) ;
	msg.data.devreq.op = LLA_MSG_DEV_CONFIG_REQ ;
	msg.data.devreq.len = len ;
	msg.data.devreq.seq = con->seq++ ;
	msg.data.devreq.devid = dev ;
	memcpy(&msg.data.devreq.req, req, len);
	return send_msg(con, &msg) ;

	return 0;

}


// private functions follow
//-----------------------------------------------------------------------------



/*
 * Send an msg on the connection
 *
 * @param con	the lla_connection
 * @param msg	the msg to send
 * 
 * @return 0 on sucess, -1 on failure
 */
static int send_msg(lla_connection *con, lla_msg *msg) {

	if ( 0 >= send(con->sd, &msg->data, msg->len, 0 ) ) {
		printf("send msg failure: %s\n", strerror(errno) ) ;
		return -1 ;
	}
	return 0 ;

}


/*
 * receive a msg on the sd and handle it
 *
 */
static int read_msg(lla_connection *con) {
	lla_msg msg ;
	int len ;

	if ( (len = recv(con->sd, &msg.data, sizeof(lla_msg_data), 0 )< 0) ) {
		// call err func here
		printf("Error in recv from: %s\n", strerror(errno)) ;
		return -1 ;
	}

	return handle_msg(con, &msg);

}

// datagram handlers
//-----------------------------------------------------------------------------


/*
 * handle a msg
 *
 */
static int handle_msg(lla_connection *con, lla_msg *msg) {
	
	// we're only interested in a couple of msgs here
	switch(msg->data.dmx.op) {
		case LLA_MSG_SYN_ACK:
			return handle_syn_ack(con,msg) ;
		case LLA_MSG_FIN_ACK:
			return handle_fin_ack(con,msg) ;
		case LLA_MSG_DMX_DATA :
			return handle_dmx(con, msg) ;
			break;
		case LLA_MSG_PLUGIN_INFO :
			return handle_plugin_info(con, msg) ;
			break ;
		case LLA_MSG_DEVICE_INFO :
			return handle_dev_info(con, msg) ;
			break ;
		case LLA_MSG_PORT_INFO :
			return handle_port_info(con, msg) ;
			break ;
		case LLA_MSG_PLUGIN_DESC :
			return handle_plugin_desc(con, msg) ;
			break ;
		case LLA_MSG_UNI_INFO :
			return handle_universe_info(con, msg) ;
			break ;
		default:
			printf("msg recv'ed but we're not interested, opcode %d\n", msg->data.dmx.op) ;
			break;
	}
	return 0 ;
}

static int handle_dmx(lla_connection *con, lla_msg *msg) {

	if(con->dmx_c.fh != NULL) {
		con->dmx_c.fh((lla_con) con, msg->data.dmx.uni ,msg->data.dmx.len, msg->data.dmx.data , con->dmx_c.data) ;
	}
	return 0 ;
}

/*
 *a syn ack marks us as connected
 */
static int handle_syn_ack(lla_connection *con, lla_msg *msg) {

	if(!con->connected) {
		con->connected = 1 ;
	}
	msg = NULL ;
	return 0 ;
}

/*
 *a fin ack marks us as connected
 */
static int handle_fin_ack(lla_connection *con, lla_msg *msg) {

	if(con->connected) {
		con->connected = 0 ;
	}
	msg = NULL;
	return 0 ;
}


/*
 * Get info on the plugins loaded
 * 
 */
static int handle_plugin_info(lla_connection *con, lla_msg *msg) {
	int i, plugins ;
	lla_plugin *plug, *plug_tail = NULL;

	plugins = min(msg->data.plinfo.nplugins, PLUGINS_PER_DATAGRAM) ;

	free_plugins(con) ;

	for(i=0; i < plugins; i++) {
		plug = malloc(sizeof(lla_plugin)) ;

		if(plug == NULL) {
			printf("malloc failed\n") ;
			return -1 ;
		}

		plug->id = msg->data.plinfo.plugins[i].id ;
		plug->name = strdup(msg->data.plinfo.plugins[i].name) ;
		plug->next = NULL ;
		
		if(con->plugins == NULL) {
			con->plugins = plug ;
			plug_tail = plug ;
		} else {
			plug_tail->next = plug ;
			plug_tail = plug ;
		}
	}
	return 0 ;
}


/*
 * 
 * 
 */
static int handle_universe_info(lla_connection *con, lla_msg *msg) {
	int i, universes ;
	lla_universe *uni, *uni_tail = NULL;

	universes = min(msg->data.uniinfo.nunis, UNIVERSES_PER_DATAGRAM) ;

	free_universes(con) ;

	// this is prob not a good way, we should malloc once straight up
	for(i=0; i < universes; i++) {
		uni = malloc(sizeof(lla_universe)) ;

		if(uni == NULL) {
			printf("malloc failed\n") ;
			return -1 ;
		}

		uni->id = msg->data.uniinfo.universes[i].id ;
		uni->name = strdup(msg->data.uniinfo.universes[i].name) ;
		
		uni->next = NULL ;
		
		if(con->universes == NULL) {
			con->universes = uni ;
			uni_tail = uni ;
		} else {
			uni_tail->next = uni ;
			uni_tail = uni ;
		}
	}

	// we make a dud entry here so the call returns
	if(universes == 0) {
		uni = malloc(sizeof(lla_universe)) ;

		if(uni == NULL) {
			printf("malloc failed\n") ;
			return -1 ;
		}
		uni->id = -1;
		uni->name = NULL ;
		uni->next = NULL ;
		con->universes = uni ;
	}

	return 0 ;
}

/*
 *
 * 
 */
static int handle_dev_info(lla_connection *con, lla_msg *msg) {
	int i, devs ;
	lla_device *dev, *dev_tail = NULL;
	
	devs = min(msg->data.dinfo.ndevs, DEVICES_PER_DATAGRAM) ;

	for(i=0; i < devs; i++) {
		dev = malloc(sizeof(lla_device)) ;

		if(dev == NULL) {
			printf("malloc failed\n") ;
			return -1 ;
		}

		dev->id = msg->data.dinfo.devices[i].id ;
		dev->name = strdup(msg->data.dinfo.devices[i].name) ;
		dev->count = msg->data.dinfo.devices[i].ports ;
		dev->ports = NULL ;
		dev->next = NULL ;
		
		if(con->devices == NULL) {
			con->devices = dev ;
			dev_tail = dev ;
		} else {
			dev_tail->next = dev ;
			dev_tail = dev ;
		}

		send_port_info_req(con, dev) ;
	}
	return 0 ;
}


/*
 *
 * 
 */
static int handle_port_info(lla_connection *con, lla_msg *msg) {
	int i, ports ;
	lla_port *prt, *prt_tail = NULL;
	lla_device *dev ;

	for( dev = con->devices ; dev != NULL; dev = dev->next) {
		if (dev->id == msg->data.prinfo.dev)
			break ;
	}

	if( dev == NULL) {
		printf("Could not locate device %d\n", msg->data.prinfo.dev) ;
		return -1 ;
	}
	
	free_ports(dev) ;
	
	ports = min(msg->data.prinfo.nports, DEVICES_PER_DATAGRAM) ;

	for(i=0; i < ports; i++) {
		prt = malloc(sizeof(lla_port)) ;

		if(prt == NULL) {
			printf("malloc failed\n") ;
			return -1 ;
		}

		prt->id = msg->data.prinfo.ports[i].id ;
		prt->cap = msg->data.prinfo.ports[i].cap ;
		prt->uni = msg->data.prinfo.ports[i].uni ;
		prt->actv = msg->data.prinfo.ports[i].actv ;
		prt->next = NULL ;

		if(dev->ports == NULL) {
			dev->ports = prt ;
			prt_tail = prt ;
		} else {
			prt_tail->next = prt ;
			prt_tail = prt ;
		}
	}
	return 0 ;
}


/*
 * Get info on the plugins loaded
 * This will block until we get a response (or time out and return NULL)
 * 
 */
static int handle_plugin_desc(lla_connection *con, lla_msg *msg) {

	con->desc = malloc(PLUGIN_DESC_LENGTH) ;

	if(con->desc == NULL) {
		printf("malloc failed\n") ;
		return -1 ;
	}
	
	strncpy(con->desc , msg->data.pldesc.desc, PLUGIN_DESC_LENGTH) ;
	
	return 0 ;
}

// datagram senders
//-----------------------------------------------------------------------------



/*
 * send a syn to the server
 */
static int send_syn(lla_connection *con) {
	lla_msg msg ;
	
	msg.len = sizeof(lla_msg_syn) ;
	msg.data.syn.op = LLA_MSG_SYN ;

	return send_msg(con, &msg) ;
}

/*
 * send a fin to the server
 */
static int send_fin(lla_connection *con) {
	lla_msg msg ;
	
	msg.len = sizeof(lla_msg_fin) ;
	msg.data.fin.op = LLA_MSG_FIN ;

	return send_msg(con, &msg) ;
}


/*
 * send a syn to the server
 */
static int send_port_info_req(lla_connection *con, lla_device *dev) {
	lla_msg msg ;
	
	msg.len = sizeof(lla_msg_port_info_request) ;
	
	msg.data.prreq.op = LLA_MSG_PORT_INFO_REQUEST ;
	msg.data.prreq.devid = dev->id ;

	send_msg(con, &msg) ;

	free_ports(dev) ;
	
	while(dev->ports == NULL) {
		switch(lla_recv(con, 1)) {
			case 0:
				break;
			case 1:
				// timeout
				printf("failed to get port info\n");
				return -1;
			case -1:
				// error
				return -1;
			case -2:
				// interupt
				break;
		}
	}

	return 0 ;
}



/*
 * Free the plugin linked list
 *
 */
static int free_plugins(lla_connection *con) {
	lla_plugin *plug_itr, *plug;
		
	plug_itr = con->plugins; 
	con->plugins = NULL ;
	
	while( plug_itr != NULL) {
		plug = plug_itr;
		plug_itr = plug_itr->next ;
		
		free(plug->name);
		free(plug);
	}
	return 0;
}


/*
 * Free the universe linked list
 *
 */
static int free_universes(lla_connection *con) {
	lla_universe *uni_itr, *uni;
		
	uni_itr = con->universes; 
	con->universes = NULL ;
	
	while( uni_itr != NULL) {
		uni = uni_itr;
		uni_itr = uni_itr->next ;
		
		free(uni->name);
		free(uni);
	}
	return 0;
}


/*
 * Free the device linked list (including all ports)
 *
 *
 */
static int free_devices(lla_connection *con) {
	lla_device *dev_itr, *dev;
		
	dev_itr = con->devices; 
	con->devices = NULL ;
	
	while( dev_itr != NULL) {
		dev = dev_itr;
		dev_itr = dev_itr->next ;
		
		free_ports(dev);
		free(dev->name);
		free(dev);
	}
	return 0;
}


/*
 * Free the ports associated with this device
 *
 *
 */
static int free_ports(lla_device *dev) {
	lla_port *prt_itr, *prt;
		
	prt_itr = dev->ports ;
	dev->ports = NULL ;
	
	// free all ports
	while (prt_itr != NULL) {
		prt = prt_itr;
		prt_itr = prt_itr->next ;
	
		free(prt);
	}

	return 0;
}

