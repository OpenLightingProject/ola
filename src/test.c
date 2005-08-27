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
 *  Copyright 2004-2005 Simon Newton
 *  nomis52@westnet.com.au
 */


#include <stdio.h>
#include <stdlib.h>
#include <lla/lla.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>


int loop(lla_con con) {
	fd_set r_fds;
	struct timeval tv;
	int maxsd;
	int sd = lla_get_sd(con) ;
	while(1) {
		FD_ZERO(&r_fds);
	  
		FD_SET(sd, &r_fds) ;

		tv.tv_sec = 1;
		tv.tv_usec = 0;
		maxsd = sd;
		
		switch( select(maxsd+1, &r_fds, NULL, NULL, &tv)) {
			case 0:
				// timeout
				break;
			case -1:
				if( errno != EINTR) {
					printf("Select error: %s", strerror(errno)) ;
					return -1;
				}
				break ;
			default:

				if(FD_ISSET(sd, &r_fds)) {
					// got a msg from a client
					if(lla_sd_action(con, 0) == -1) {
						printf("error in action\n") ;
					}
				}
		}
	}
	return sd;
}


int main(int argc, char*argv[]) {
	uint8_t buf[512] ;	
	lla_con con ;

	memset(buf, 0x00, 512) ;

	con = lla_connect() ;

	if (!con) {
		printf("error: %s\n", strerror(errno) ) ;
		exit(1) ;
	}

	if( lla_req_dev_info(con) ) {
		printf("write failed\n") ;
		exit(1);
	}
	
	if( lla_patch(con, 0,0,1,10) ) {
		printf("write failed\n") ;
		exit(1);
	}

	loop(con) ;

	lla_disconnect(con) ;

	return 0;
}
