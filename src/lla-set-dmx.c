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
 *  lla-set-dmx.c
 *  The the dmx values for a particular universe
 *  Copyright (C) 2006 Simon Newton
 */

#include <stdio.h>
#include <stdlib.h>
#include <lla/lla.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>

#include <getopt.h>

typedef struct {
	int verbose ;
	int help;
	char *dmx ;
	int uni ;
} opts_t; 


/*
 * Parse command lines options and save in opts_s struct
 *
 */
void parse_args(opts_t *ops, int argc, char *argv[]) {
	
	static struct option long_options[] = {
			{"dmx", 		required_argument,	0, 'd'},
			{"universe", 	required_argument,	0, 'u'},
			{"help", 		no_argument, 		0, 'h'},
			{"verbose", 	no_argument, 		0, 'v'},
			{0, 0, 0, 0}
		};

	int c ;
	int option_index = 0;
	
	while (1) {
     
		c = getopt_long(argc, argv, "d:u:vh", long_options, &option_index);
		
		if (c == -1)
			break;

		switch (c) {
			case 0:
				break;
				
			case 'd':
				ops->dmx = (char *) strdup(optarg)  ;	
		        break;
			case 'u':
				ops->uni = atoi(optarg);	
		        break;
			case 'h':
				ops->help = 1 ;
				break;
			case 'v':
				ops->verbose = 1 ;
				break;

			case '?':
				break;
     
			default:
				;
		}
	}
}


/*
 *
 */
void display_help_and_exit(opts_t *ops, char *argv[]) {

	printf(
"Usage: %s --universe <universe> --dmx 0,255,0,255\n"
"\n"
"Sets the DMX values for a universe.\n"
"\n"
"  -d, --dmx <values>              Comma separated DMX values.\n"
"  -h, --help                      Display this help message and exit.\n"
"  -u, --universe <universe>      Universe number.\n"
"  -v, --verbose                   Be verbose.\n"
"\n",
	argv[0] ) ;
	
	exit(0);
}



/*
 * Set our default options, command line args will overide this
 */
void init_ops(opts_t *ops) {
	
	ops->verbose = 0;
	ops->help = 0;
	ops->dmx = NULL ;
	ops->uni = -1 ;
}



/*
 * Connect, fetch the device listing and display
 *
 *
 */
int main(int argc, char*argv[]) {
	lla_con con ;
	opts_t ops;
	uint8_t buf[512];
	char *s; 

	init_ops(&ops) ;
	parse_args(&ops, argc, argv) ;

	// do some checks
	if( ops.help)
		display_help_and_exit(&ops, argv) ;

	if( ops.uni < 0 ) {
		display_help_and_exit(&ops, argv) ;
	}

	// connect
	con = lla_connect() ;

	if (!con) {
		printf("error: %s\n", strerror(errno) ) ;
		exit(1) ;
	}

	int i=0;
	for( s = strtok(ops.dmx, ",") ; s != NULL ; s = strtok(NULL, ",") ) {
		int v  = atoi(s) ;
		buf[i++] = v > 255 ? 255 : v;
	}

	int ret = lla_send_dmx(con, ops.uni, buf, i) ;

	if( ret) {
		printf("Send DMX failed:\n") ;
	}

	lla_disconnect(con) ;

	return 0;
}
