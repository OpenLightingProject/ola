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
 *  lla-dev-info.c
 *  Displays the available devices and ports 
 *  Copyright (C) 2005 Simon Newton
 */

#include <stdio.h>
#include <stdlib.h>
#include <lla/lla.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>


/*
 * Connect, fetch the device listing and display
 *
 *
 */
int main(int argc, char*argv[]) {
	lla_con con ;
	lla_device *devitr ;
	lla_port *prtitr ;
	
	// connect
	con = lla_connect() ;

	if (!con) {
		printf("error: %s\n", strerror(errno) ) ;
		exit(1) ;
	}

	if( (devitr = lla_req_dev_info(con, LLA_PLUGIN_ALL)) ) {
		while( devitr != NULL) {
			printf("Device %d: %s\n", devitr->id,  devitr->name ) ;

			for( prtitr = devitr->ports; prtitr != NULL; prtitr = prtitr->next ) {
				printf("  port %d, cap ", prtitr->id) ;
				if (prtitr->cap & LLA_PORT_CAP_IN) 
					printf("IN");

				if ((prtitr->cap & LLA_PORT_CAP_IN) && (prtitr->cap & LLA_PORT_CAP_OUT) ) 
					printf("/");
				
				if (prtitr->cap & LLA_PORT_CAP_OUT) 
					printf("OUT");

				if(prtitr->actv)
					printf(", universe %d", prtitr->uni) ;
				
				printf("\n");
			}
			
			devitr = devitr->next ;
		}
			
	} else {
		printf("lla_reg_dev_info failed\n") ;
	}

	lla_disconnect(con) ;

	return 0;
}
