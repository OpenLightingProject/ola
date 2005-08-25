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


int main(int argc, char*argv[]) {
	uint8_t buf[512] ;	
	lla_con con ;

	memset(buf, 0x00, 512) ;

	con = lla_connect() ;

	if (!con) {
		printf("error: %s\n", strerror(errno) ) ;
		exit(1) ;
	}

	if( lla_patch(con, 0,0,1,10) ) {
		printf("patch failed\n") ;
	}

	lla_disconnect(con) ;

	return 0;
}
