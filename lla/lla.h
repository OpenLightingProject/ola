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
 * lla.h
 * Header file for liblla 
 * Copyright (C) 2005  Simon Newton
 */

#include <stdint.h>
#include <lla/plugin_id.h>

#define	LLA_PORT_CAP_IN 0x01
#define	LLA_PORT_CAP_OUT 0x02

#define	LLA_PORT_ACTION_PATCH 0x01
#define	LLA_PORT_ACTION_UNPATCH 0x00


typedef void * lla_con ;

struct lla_plugin_s {
	int id;			// id of this plugin
	char *name;		// plugin name
	struct lla_plugin_s *next;	// pointer to next plugins
};

typedef struct lla_plugin_s lla_plugin ;



struct lla_universe_s {
	int id;			// id of this universe
	char *name;
	struct lla_universe_s *next;	// pointer to next plugins
};

typedef struct lla_universe_s lla_universe ;

struct lla_port_s {
	int id;			// id of this port
	int cap;		// port capability
	int uni;		// universe
	int actv;		// active
	struct lla_port_s *next;	// pointer to next port
};

typedef struct lla_port_s lla_port ;


struct lla_device_s {
	int id ;
	int count;
	char *name;
	lla_plugin_id plugin;
	struct lla_port_s *ports;
	struct lla_device_s *next;
};

typedef struct lla_device_s lla_device ;

extern lla_con lla_connect() ;
extern int lla_disconnect(lla_con c) ;
extern int lla_get_sd(lla_con c) ;
extern int lla_sd_action(lla_con c, int delay) ;


extern int lla_set_dmx_handler(lla_con c, int (*fh)(lla_con c, int uni, int length, uint8_t *data, void *d), void *data ) ;
// extern int lla_set_rdm_handler(lla_con c, int (*fh)(lla_con c, int uni, void *d), void *data ) ;
extern int lla_reg_uni(lla_con c, int uni, int action) ;

extern int lla_set_name(lla_con c, int uni, char *name) ;

extern int lla_send_dmx(lla_con c, int universe, uint8_t *data, int length) ;
extern int lla_read_dmx(lla_con c, int universe) ;

//read/write rdm ?


extern lla_device *lla_req_dev_info(lla_con c, lla_plugin_id filter) ;
extern lla_plugin *lla_req_plugin_info(lla_con c) ;
extern int lla_req_universe_info(lla_con c, lla_universe **head) ;

extern char *lla_req_plugin_desc(lla_con c, int pid) ;

extern int lla_patch(lla_con c, int dev, int port, int action, int uni) ;

