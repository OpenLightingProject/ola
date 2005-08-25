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

static int send_syn(lla_connection *con) ;
static int send_fin(lla_connection *con) ;

/*
 * open a connection to the daemon
 *
 * @return lla_con on sucess, NULL on failure
 */
lla_con lla_connect() {
	struct sockaddr_in servaddr ;
	lla_connection *c = malloc(sizeof(lla_con)) ;
	
	if(c == NULL)
		goto e_malloc ;

	// init members
	c->connected = 0 ;
	c->dmx_c.fh = NULL ;
	c->dmx_c.data = NULL ;
	
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

	// wait up to 1 second for reply
	lla_sd_action((lla_con) c, 1) ;
	
	if(c->connected)
		return (lla_con) c ;
		
	// failed to connect
	printf("failed to get syn ack\n");

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
	if(con == NULL)
		return -1 ;
	
	send_fin(con) ;

	// wait up to 1 second for reply
	lla_sd_action((lla_con) c, 1) ;

	if(con->connected) {
		printf("failed to get fin ack\n");
		return -1 ;
	}

	close(con->sd) ;
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

	if(con == NULL) 
		return -1 ;

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
	fd_set rset;
	struct timeval tv;
	
	if(con == NULL) 
		return -1 ;
	
	while(1) {
		tv.tv_usec = 0 ;
		tv.tv_sec = delay ;

		FD_ZERO(&rset) ;
		FD_SET(con->sd, &rset) ;

		switch ( select( con->sd+1, &rset, NULL, NULL, &tv ) ) {
			case 0:
				// timeout
				return 0;
				break ;
			case -1 :
				//error
				if( errno != EINTR) {
					printf("%s : select error", strerror(errno) ) ;
					return -1;
				}
				// we were interupted
				return 0 ;
				break ;
			default:
				// ok something there to read
				printf("got reply\n");
				return read_msg(con) ;
				break;
		}
	}
	
	return 0;
}


/*
 * set the dmx callback
 *
 *
 *
 */
int lla_set_dmx_handler(lla_con c, int (*fh)(lla_con c, int uni, void *d), void *data ) {
	lla_connection *con = (lla_connection*) c ;

	if(con == NULL) 
		return -1 ;

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
	
	if(con == NULL) 
		return -1 ;

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
	
	if(con == NULL) 
		return -1 ;

	msg.len = sizeof(lla_msg_dmx_data) ;
	msg.data.dmx.op = LLA_MSG_DMX_DATA ;
	msg.data.dmx.uni = uni;
	msg.data.dmx.pad1 = 0;
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
int lla_read_dmx(lla_con c, int universe, uint8_t *data, int length) {
	//send request
	lla_connection *con = (lla_connection*) c ;
	
	if(con == NULL) 
		return -1 ;
	
	universe++ ;
	length++;
	data++;

	
	//read reply
	return 0;
}

/*
 * This is supposed to give us info on what devices are out there
 *
 *
 */
int lla_get_info(lla_con c) {
	lla_connection *con = (lla_connection*) c ;
	lla_msg msg ;
	
	if(con == NULL) 
		return -1 ;

	//msg.len = sizeof(lla_msg_info_request) ;
//	msg.data.dmx.op = LLA_MSG_INFO_REQUEST ;
	
	return send_msg(con, &msg) ;

	return 0;

}


int lla_patch(lla_con c, int dev, int port, int action, int uni) {

	lla_connection *con = (lla_connection*) c ;
	lla_msg msg ;
	
	if(con == NULL) 
		return -1 ;

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
				
//		case LLA_MSG_INFO :
//			printf("got msg info datagram\n") ;
//			break ;

		default:
			printf("msg recv'ed but we're not interested, opcode %d\n", msg->data.dmx.op) ;
			break;
	}
	return 0 ;
}

static int handle_dmx(lla_connection *con, lla_msg *msg) {

	// now we want to store the dmx data in the buffer
	// only if we're interested in it
	con = NULL ;
	msg = NULL ;

	return 0 ;
}

/*
 *a syn ack marks us as connected
 */
static int handle_syn_ack(lla_connection *con, lla_msg *msg) {

	if(!con->connected) {
		printf("got syn ack!\n");
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
		printf("got fin ack!\n");
		con->connected = 0 ;
	}
	msg = NULL;
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
