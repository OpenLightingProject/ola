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
 * Copyright (C) 2005 - 2006 Simon Newton
 */

/*
 * Hopefully this will all become autogen'ed once that tool can support nested structures
 *
 */

#ifndef MESSAGES_H
#define MESSAGES_H

#include <stdint.h>
#include <netinet/in.h>
#include <lla/plugin_id.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * op codes
 */
enum {
	LLA_MSG_SYN = 0x01,
	LLA_MSG_SYN_ACK = 0x02,
	LLA_MSG_FIN = 0x03,
	LLA_MSG_FIN_ACK = 0x04,
	LLA_MSG_PING = 0x05,
	LLA_MSG_PONG = 0x06,

	LLA_MSG_READ_REQ = 0x10,
	LLA_MSG_DMX_DATA = 0x11,
	LLA_MSG_REGISTER = 0x12,
	LLA_MSG_PATCH = 0x13,
	LLA_MSG_UNI_NAME = 0x14,
	LLA_MSG_UNI_MERGE = 0x15,
		
	LLA_MSG_PLUGIN_INFO_REQUEST = 0x24,
	LLA_MSG_PLUGIN_INFO = 0x25,
	LLA_MSG_PLUGIN_DESC_REQUEST = 0x26,
	LLA_MSG_PLUGIN_DESC = 0x27,
	LLA_MSG_DEVICE_INFO_REQUEST = 0x28,
	LLA_MSG_DEVICE_INFO = 0x29,
	LLA_MSG_PORT_INFO_REQUEST = 0x2A,
	LLA_MSG_PORT_INFO = 0x2B,
	LLA_MSG_UNI_INFO_REQUEST = 0x2C,
	LLA_MSG_UNI_INFO = 0x2D,

	LLA_MSG_DEV_CONFIG_REQ = 0x31,
	LLA_MSG_DEV_CONFIG_REP = 0x32

};


// mtu on the loopback is going to be around 16k
// so we increase these if it becomes an issue
enum { PLUGINS_PER_DATAGRAM = 30};			// sets datagram to 1213 bytes
enum { DEVICES_PER_DATAGRAM = 30};			// sets datagram to 1217 bytes
enum { PORTS_PER_DATAGRAM =	60};			// sets datagram to 497 bytes
enum { UNIVERSES_PER_DATAGRAM = 512};

enum { DMX_LENGTH = 512};
enum { PLUGIN_DESC_LENGTH = 1024};
enum { PLUGIN_NAME_LENGTH = 30};
enum { DEVICE_NAME_LENGTH = 30};
enum { UNIVERSE_NAME_LENGTH = 30};


enum uni_merge_mode {
	UNI_MERGE_MODE_HTP,
	UNI_MERGE_MODE_LTP
};

/*
 * sent on client connect
 */
struct lla_msg_syn_s {
	uint8_t op;		// op code
}__attribute__( ( packed ) );

typedef struct lla_msg_syn_s lla_msg_syn;

	
/*
 * server reply to a syn
 */
struct lla_msg_syn_ack_s {
	uint8_t op;		// op code
}__attribute__( ( packed ) );

typedef struct lla_msg_syn_ack_s lla_msg_syn_ack;



/*
 * sent on client disconnect
 */
struct lla_msg_fin_s {
	uint8_t op;		// op code
}__attribute__( ( packed ) );

typedef struct lla_msg_fin_s lla_msg_fin;

	
/*
 * server reply to a syn
 */
struct lla_msg_fin_ack_s {
	uint8_t op;		// op code
}__attribute__( ( packed ) );

typedef struct lla_msg_fin_ack_s lla_msg_fin_ack;



/*
 * heartbeat, clients need to reply with a pong
 */
struct lla_msg_ping_s {
	uint8_t op;		// op code
}__attribute__( ( packed ) );

typedef struct lla_msg_ping_s lla_msg_ping;

	
/*
 * clients reply to a ping with a pong
 */
struct lla_msg_pong_s {
	uint8_t op;		// op code
}__attribute__( ( packed ) );

typedef struct lla_msg_pong_s lla_msg_pong;



/*
 * requests a dmx data packet to be sent
 */
struct lla_msg_read_request_s {
	uint8_t op;		// op code
	uint8_t uni;			// uni address
}__attribute__( ( packed ) );

typedef struct lla_msg_read_request_s lla_msg_read_request;


/*
 * dmx data packet
 *
 */
struct lla_msg_dmx_data_s {
	uint8_t op;					// op code
	uint8_t uni;				// uni address
	uint16_t len;				// data length
	uint8_t data[DMX_LENGTH];	// dmx data
}__attribute__( ( packed ) );

typedef struct lla_msg_dmx_data_s lla_msg_dmx_data;



static const uint8_t LLA_MSG_REG_REG = 0x01;
static const uint8_t LLA_MSG_REG_UNREG = 0x00;

/*
 * (un)register interest in a universe
 *
 */
struct lla_msg_register_s{
	uint8_t op;				// op code
	uint8_t uni;			// uni address
	uint8_t	action;			// action ( LLA_MSG_REG_REG or LLA_MSG_REG_UNREG)
}__attribute__( ( packed ) );

typedef struct lla_msg_register_s lla_msg_register;


static const uint8_t LLA_MSG_PATCH_REMOVE = 0x00;
static const uint8_t LLA_MSG_PATCH_ADD = 0x01;

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
}__attribute__( ( packed ) ) ;

typedef struct lla_msg_patch_s lla_msg_patch; 


/*
 * set the name of a universe
 *
 */
struct lla_msg_uni_name_s {
	uint8_t op;			// op code
	int uni;			// universe
	char name[UNIVERSE_NAME_LENGTH];		// universe name
}__attribute__( ( packed ) ) ;


typedef struct lla_msg_uni_name_s lla_msg_uni_name; 


/*
 * set the merge mode of a universe
 *
 */
struct lla_msg_uni_merge_s {
	uint8_t op;			// op code
	int uni;			// universe
	int mode;			// universe merge mode
}__attribute__( ( packed ) ) ;


typedef struct lla_msg_uni_merge_s lla_msg_uni_merge; 


/*
 * request info about available plugins
 *
 */
struct lla_msg_plugin_info_request_s {
	uint8_t op;		// op code
}__attribute__( ( packed ) ); 

typedef struct lla_msg_plugin_info_request_s lla_msg_plugin_info_request;


/*
 * request the description for a plugins
 *
 */
struct lla_msg_plugin_desc_request_s {
	uint8_t op;		// op code
	int pid;		// plugin id
}__attribute__( ( packed ) ); 

typedef struct lla_msg_plugin_desc_request_s lla_msg_plugin_desc_request;



/*
 * Request info on all available devices
 *
 */
struct lla_msg_device_info_request_s {
	uint8_t op;				// op code
	lla_plugin_id plugin;	// device filter
}__attribute__( ( packed ) ); 

typedef struct lla_msg_device_info_request_s lla_msg_device_info_request;

/*
 * Request info about ports for a device
 *
 */
struct lla_msg_port_info_request_s {
	uint8_t op;		// op code
	int devid;		// device id
}__attribute__( ( packed ) ); 

typedef struct lla_msg_port_info_request_s lla_msg_port_info_request;


/*
 * Request info about all universes
 *
 */
struct lla_msg_uni_info_request_s {
	uint8_t op;		// op code
}__attribute__( ( packed ) ); 

typedef struct lla_msg_uni_info_request_s lla_msg_uni_info_request;




/*
 * represents a plugin
 *
 */
struct lla_msg_plugin_s {
	int id;								// plugin id
	char name[PLUGIN_NAME_LENGTH];			// plugin name
};



/*
 * represents a device
 */
struct lla_msg_device_s {
	int id;				// device id
	int ports;				// number of ports
	lla_plugin_id	plugin;	// the id of the owner
	char name[30];			// name
};


static const uint8_t LLA_MSG_PORT_CAP_IN = 0x01;
static const uint8_t LLA_MSG_PORT_CAP_OUT = 0x02;

/*
 * represents a port
 */
struct lla_msg_port_s {
	int id;				// port id
	int uni;				// universe
	uint8_t cap;			// capability ?
	uint8_t	actv;
};



/*
 * represents a universe
 */
struct lla_msg_info_s {
	int id;								// universe id
	int merge;							// merge mode
	char name[UNIVERSE_NAME_LENGTH];	//name
};


/*
 * returns info about available plugins
 *
 */
struct lla_msg_plugin_info_s {
	uint8_t op;		// op code
	int nplugins;	// total plugin count
	int offset;	// offset of this msg
	int count;		// number of plugins in this msg
	struct lla_msg_plugin_s plugins[PLUGINS_PER_DATAGRAM];	//plugin struct
}__attribute__( ( packed ) ); 

typedef struct lla_msg_plugin_info_s lla_msg_plugin_info;


/*
 * returns the description for the plugin
 *
 */
struct lla_msg_plugin_desc_s {
	uint8_t op;						// op code
	int pid;
	char desc[PLUGIN_DESC_LENGTH];	//desc
}__attribute__( ( packed ) ); 

typedef struct lla_msg_plugin_desc_s lla_msg_plugin_desc;


/*
 * return info about available devices
 *
 */
struct lla_msg_device_info_s  {
	uint8_t op;		// op code
	int ndevs;		// number of devices in total
	int offset;		// offset of this msg
	int count;		// number of ports in this msg
	struct lla_msg_device_s devices[DEVICES_PER_DATAGRAM];
}__attribute__( ( packed ) );

typedef struct lla_msg_device_info_s lla_msg_device_info;



/*
 * return info about available ports
 *
 */
struct lla_msg_port_info_s  {
	uint8_t op;		// op code
	int dev;		// device id
	int nports;		// number of ports in total
	int offset;		// offset of this msg
	int count;		// number of ports in this msg
	struct lla_msg_port_s ports[PORTS_PER_DATAGRAM];
}__attribute__( ( packed ) );

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
	struct lla_msg_info_s universes[UNIVERSES_PER_DATAGRAM];
}__attribute__( ( packed ) );

typedef struct lla_msg_uni_info_s lla_msg_uni_info;


/*
 * a device configuration request
 *
 */
struct lla_msg_device_config_req_s {
	uint8_t op;			// op code
	uint8_t	pad;		// padding
	uint16_t seq;		// sequence number
	uint32_t len;		// request length
	int devid;			// device id
	uint8_t	req[1400];	// request data
}__attribute__( ( packed ) );

typedef struct lla_msg_device_config_req_s lla_msg_device_config_req;


/*
 * A device configuration reply
 *
 */
struct lla_msg_device_config_rep_s {
	uint8_t op;			// op code
	uint8_t status;		// error code
	uint16_t seq;		// sequence number
	int	dev;			// device id
	uint32_t len;		// request length
	uint8_t	rep[1400];	// reply data
}__attribute__( ( packed ) );

typedef struct lla_msg_device_config_rep_s lla_msg_device_config_rep;


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
	lla_msg_uni_name uniname;
	lla_msg_uni_merge unimerge;
	
	lla_msg_plugin_info_request plreq;
	lla_msg_plugin_info	plinfo;
	lla_msg_device_info_request dreq;
	lla_msg_device_info dinfo;
	lla_msg_port_info_request prreq;
	lla_msg_port_info prinfo;
	lla_msg_plugin_desc_request pldreq;
	lla_msg_plugin_desc pldesc;
	lla_msg_uni_info_request unireq;
	lla_msg_uni_info uniinfo;
	lla_msg_device_config_req devreq;
	lla_msg_device_config_rep devrep;
} lla_msg_data;



typedef struct {
	struct sockaddr_in from;
	struct sockaddr_in to;
	int len;
	lla_msg_data data;

} lla_msg;

#ifdef __cplusplus
}
#endif


#endif
