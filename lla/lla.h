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


typedef void * lla_con ;


extern lla_con lla_connect() ;
extern int lla_disconnect(lla_con c) ;
extern int lla_get_sd(lla_con c) ;
extern int lla_sd_action(lla_con c, int delay) ;


extern int lla_set_dmx_handler(lla_con c, int (*fh)(lla_con c, int uni, void *d), void *data ) ;
// extern int lla_set_rdm_handler(lla_con c, int (*fh)(lla_con c, int uni, void *d), void *data ) ;
extern int lla_reg_uni(lla_con c, int uni, int action) ;


extern int lla_send_dmx(lla_con c, int universe, uint8_t *data, int length) ;
extern int lla_read_dmx(lla_con c, int universe, uint8_t *data, int length) ;

//read/write rdm ?


extern int lla_get_info(lla_con c) ;
extern int lla_patch(lla_con c, int dev, int port, int action, int uni) ;

