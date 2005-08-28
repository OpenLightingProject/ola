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
#define	LLA_MSG_PLUGIN_DESC_REQUEST 0x16
#define	LLA_MSG_PLUGIN_DESC 0x17
#define	LLA_MSG_DEVICE_INFO_REQUEST 0x18
#define	LLA_MSG_DEVICE_INFO 0x19
#define	LLA_MSG_PORT_INFO_REQUEST 0x1A
#define	LLA_MSG_PORT_INFO 0x1B
#define	LLA_MSG_UNI_INFO_REQUEST 0x1C
#define	LLA_MSG_UNI_INFO 0x1D


// defines

// mtu on the loopback is going to be around 16k
#define PLUGINS_PER_DATAGRAM		30			// sets datagram to 1213 bytes
#define DEVICES_PER_DATAGRAM		30			// sets datagram to 1217 bytes
#define PORTS_PER_DATAGRAM			60			// sets datagram to 497 bytes
#define UNIVERSES_PER_DATAGRAM		512			// 
		
#define DMX_LENGTH 				512
#define PLUGIN_DESC_LENGTH 		1024
#define PLUGIN_NAME_LENGTH		30
#define DEVICE_NAME_LENGTH		30
		
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
 * request info about available plugins
 *
 */
struct lla_msg_plugin_info_request_s {
	uint8_t op;		// op code
}__attribute__( ( packed ) ) ; 

typedef struct lla_msg_plugin_info_request_s lla_msg_plugin_info_request ;


/*
 * request the description for a plugins
 *
 */
struct lla_msg_plugin_desc_request_s {
	uint8_t op;		// op code
	int pid ;
}__attribute__( ( packed ) ) ; 

typedef struct lla_msg_plugin_desc_request_s lla_msg_plugin_desc_request ;



/*
 * Request info on all available devices
 *
 */
struct lla_msg_device_info_request_s {
	uint8_t op;		// op code
}__attribute__( ( packed ) ) ; 

typedef struct lla_msg_device_info_request_s lla_msg_device_info_request ;

/*
 * Request info about ports for a device
 *
 */
struct lla_msg_port_info_request_s {
	uint8_t op;		// op code
	int devid ;		// device id
}__attribute__( ( packed ) ) ; 

typedef struct lla_msg_port_info_request_s lla_msg_port_info_request ;


/*
 * Request info about all universes
 *
 */
struct lla_msg_uni_info_request_s {
	uint8_t op;		// op code
}__attribute__( ( packed ) ) ; 

typedef struct lla_msg_uni_info_request_s lla_msg_uni_info_request ;




/*
 * represents a plugin
 *
 */
struct lla_msg_plugin_s {
	int id ;								// plugin id
	char name[PLUGIN_NAME_LENGTH];			// plugin name
};



/*
 * represents a device
 */
struct lla_msg_device_s {
	int id ;				// device id
	int ports ;				// number of ports
	char name[30];			// name
};


#define	LLA_MSG_PORT_CAP_IN 0x01
#define	LLA_MSG_PORT_CAP_OUT 0x02

/*
 * represents a port
 */
struct lla_msg_port_s {
	int id ;				// port id
	int uni;				// universe
	uint8_t cap ;			// capability ?
	uint8_t	actv;
};



/*
 * represents a universe
 */
struct lla_msg_info_s {
	int id;				// universe id
//	int mode				// merge mode
};


/*
 * returns info about available plugins
 *
 */
struct lla_msg_plugin_info_s {
	uint8_t op;		// op code
	int nplugins;	// total plugin count
	int offset ;	// offset of this msg
	int count ;		// number of plugins in this msg
	struct lla_msg_plugin_s plugins[PLUGINS_PER_DATAGRAM] ;	//plugin struct
}__attribute__( ( packed ) ) ; 

typedef struct lla_msg_plugin_info_s lla_msg_plugin_info ;


/*
 * returns the description for the plugin
 *
 */
struct lla_msg_plugin_desc_s {
	uint8_t op;						// op code
	int pid;
	char desc[PLUGIN_DESC_LENGTH];	//desc
}__attribute__( ( packed ) ) ; 

typedef struct lla_msg_plugin_desc_s lla_msg_plugin_desc ;


/*
 * return info about available devices
 *
 */
struct lla_msg_device_info_s  {
	uint8_t op;		// op code
	int ndevs;		// number of ports in total
	int offset;		// offset of this msg
	int count;		// number of ports in this msg
	struct lla_msg_device_s devices[DEVICES_PER_DATAGRAM] ;
}__attribute__( ( packed ) ) ;

typedef struct lla_msg_device_info_s lla_msg_device_info;



/*
 * return info about available ports
 *
 */
struct lla_msg_port_info_s  {
	uint8_t op;		// op code
	int dev ;		// device id
	int nports;		// number of ports in total
	int offset;		// offset of this msg
	int count;		// number of ports in this msg
	struct lla_msg_port_s ports[PORTS_PER_DATAGRAM] ;
}__attribute__( ( packed ) ) ;

typedef struct lla_msg_port_info_s lla_msg_port_info;



/*
 * return info about available ports
 *
 */
struct lla_msg_uni_info_s  {
	uint8_t op;		// op code
	int nunis;		// number of universe in total
	int offset;		// offset of this msg
	int count;		// number of ports in this msg
	struct lla_msg_info_s universes[UNIVERSES_PER_DATAGRAM] ;
}__attribute__( ( packed ) ) ;

typedef struct lla_msg_uni_info_s lla_msg_uni_info;



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
	lla_msg_plugin_info_request plreq;
	lla_msg_plugin_info	plinfo ;
	lla_msg_device_info_request dreq;
	lla_msg_device_info dinfo;
	lla_msg_port_info_request prreq;
	lla_msg_port_info prinfo;
	lla_msg_plugin_desc_request pldreq;
	lla_msg_plugin_desc pldesc ;
	lla_msg_uni_info_request unireq;
	lla_msg_uni_info uniinfo ;

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
