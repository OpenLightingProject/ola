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
 * messages.h
 * Defines the format of the datagrams passed between liblla and llad 
 * Copyright (C) 2005  Simon Newton
 */

#ifndef MESSAGES_H
#define MESSAGES_H

#include <stdint.h>
#include <netinet/in.h>

#define DMX_LENGTH 512

#ifdef __cplusplus
extern "C" {
#endif

/*
 * op codes
 */
#define	LLA_MSG_SYN 0x01
#define	LLA_MSG_SYN_ACK 0x02
#define	LLA_MSG_FIN 0x03
#define	LLA_MSG_FIN_ACK 0x04
#define	LLA_MSG_PING 0x05
#define	LLA_MSG_PONG 0x06
#define	LLA_MSG_READ_REQ 0x10
#define	LLA_MSG_DMX_DATA 0x11
#define	LLA_MSG_REGISTER 0x12
#define	LLA_MSG_PATCH 0x13
#define	LLA_MSG_PLUGIN_INFO_REQUEST 0x14
#define	LLA_MSG_PLUGIN_INFO 0x15
#define	LLA_MSG_DEVICE_INFO_REQUEST 0x16
#define	LLA_MSG_DEVICE_INFO 0x17
	
/*
 * sent on client connect
 */
struct lla_msg_syn_s {
	uint8_t op;		// op code
}__attribute__( ( packed ) ) ;

typedef struct lla_msg_syn_s lla_msg_syn ;

	
/*
 * server reply to a syn
 */
struct lla_msg_syn_ack_s {
	uint8_t op;		// op code
}__attribute__( ( packed ) ) ;

typedef struct lla_msg_syn_ack_s lla_msg_syn_ack ;



/*
 * sent on client disconnect
 */
struct lla_msg_fin_s {
	uint8_t op;		// op code
}__attribute__( ( packed ) ) ;

typedef struct lla_msg_fin_s lla_msg_fin ;

	
/*
 * server reply to a syn
 */
struct lla_msg_fin_ack_s {
	uint8_t op;		// op code
}__attribute__( ( packed ) ) ;

typedef struct lla_msg_fin_ack_s lla_msg_fin_ack ;



/*
 * heartbeat, clients need to reply with a pong
 */
struct lla_msg_ping_s {
	uint8_t op;		// op code
}__attribute__( ( packed ) ) ;

typedef struct lla_msg_ping_s lla_msg_ping ;

	
/*
 * clients reply to a ping with a pong
 */
struct lla_msg_pong_s {
	uint8_t op;		// op code
}__attribute__( ( packed ) ) ;

typedef struct lla_msg_pong_s lla_msg_pong ;



/*
 * requests a dmx data packet to be sent
 */
struct lla_msg_read_request_s {
	uint8_t op;		// op code
	uint8_t uni;			// uni address
}__attribute__( ( packed ) ) ;

typedef struct lla_msg_read_request_s lla_msg_read_request ;


/*
 * dmx data packet
 *
 */
struct lla_msg_dmx_data_s {
	uint8_t op;		// op code
	uint16_t len;				// data length
	uint8_t uni;				// uni address
	uint8_t	pad1;				// padding
	uint8_t data[DMX_LENGTH];			// dmx data
}__attribute__( ( packed ) ) ;

typedef struct lla_msg_dmx_data_s lla_msg_dmx_data ;



#define	LLA_MSG_REG_REG 0x01
#define	LLA_MSG_REG_UNREG 0x00

/*
 * (un)register interest in a universe
 *
 */
struct lla_msg_register_s{
	uint8_t op;				// op code
	uint8_t uni;			// uni address
	uint8_t	action;			// action ( LLA_MSG_REG_REG or LLA_MSG_REG_UNREG)
}__attribute__( ( packed ) ) ;

typedef struct lla_msg_register_s lla_msg_register ;


#define	LLA_MSG_PATCH_ADD 0x01
#define	LLA_MSG_PATCH_REMOVE 0x00

/*
 * patch a dev:port to a universe
 *
 */
struct lla_msg_patch_s {
	uint8_t op;			// op code
	int dev;			// device id
	int port;			// port id
	uint8_t action;		//action  (LLA_MSG_PATCH_ADD  or LLA_MSG_PATCH_REMOVE )
	int uni;			// universe
}__attribute__( ( packed ) )  ;

typedef struct lla_msg_patch_s lla_msg_patch ; 


/*
 * get info about available plugins
 *
 */
struct lla_msg_plugin_info_request_s {
	uint8_t op;		// op code
}__attribute__( ( packed ) ) ; 

typedef struct lla_msg_plugin_info_request_s lla_msg_plugin_info_request ;




struct lla_msg_plugin_s {
	int id ;				// plugin id
	int devs ;				// number of devices
	char name[30];
};

/*
 * returns info about available plugins
 *
 */
struct lla_msg_plugin_info_s {
	uint8_t op;		// op code
	int count;	// total plugin count
	int offset ;	// offset of this msg
	struct lla_msg_plugin_s plugins[10] ;	//plugin struct
}__attribute__( ( packed ) ) ; 

typedef struct lla_msg_plugin_info_s lla_msg_plugin_info ;



/*
 * get info about a device
 *
 */
struct lla_msg_device_info_request_s {
	uint8_t op;		// op code
	int pid ;

}__attribute__( ( packed ) ) ; 

typedef struct lla_msg_device_info_request_s lla_msg_device_info_request ;






/*
 * return a device, how are we going to do this ?
 *
 
struct lla_msg_info_s  {
	uint8_t op;		// op code
	int dev ;
	char plugin[10] ;
	int nports;
	int offset;
	struct lla_msg_port_s ports[10] ;
}__attribute__( ( packed ) ) ;

typedef struct lla_msg_info_s lla_msg_info;
*/

/* 
 * union of all our messages
 *
 */
typedef union {
	lla_msg_syn syn;
	lla_msg_syn_ack sack;
	lla_msg_fin fin;
	lla_msg_fin_ack fack;
	lla_msg_ping ping;
	lla_msg_pong pong;
		
	lla_msg_read_request rreq;
	lla_msg_dmx_data dmx;
	lla_msg_register reg;
	lla_msg_patch patch;
	lla_msg_plugin_info_request preq;
	lla_msg_plugin_info	pinfo ;
	lla_msg_device_info_request dreq;
} lla_msg_data ;



typedef struct {
	struct sockaddr_in from;
	struct sockaddr_in to;
	int len;
	lla_msg_data data ;

} lla_msg ;

#ifdef __cplusplus
}
#endif


#endif
