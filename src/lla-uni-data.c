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
 *  lla-uni-data.c
 *  Displays the active universes
 *  Copyright (C) 2005 Simon Newton
 */


#include <stdio.h>
#include <stdlib.h>
#include <lla/lla.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>


typedef struct {
	int help;
} options;

/*
 * parse our cmd line options
 *
 */
int parse_options(int argc, char *argv[], options *opts) {
	static struct option long_options[] = {
			{"help", 		no_argument, 		0, 'h'},
			{0, 0, 0, 0}
		};

	int c ;
	int option_index = 0;
	
	while (1) {
     
		c = getopt_long(argc, argv, "h", long_options, &option_index);
		
		if (c == -1)
			break;

		switch (c) {
			case 0:
				break;
			case 'h':
				opts->help = 1 ;
				break;
			case '?':
				break;
     
			default:
				;
		}
	}

	return 0;
}

/*
 * Init options
 */
void init_options(options *opts) {
	opts->help = 0 ;
}


/*
 * Display the help message
 */
void display_help_and_exit() {

	printf(
"Usage: lla_uni_data\n"
"\n"
"Shows info on the active universes in use.\n"
"\n"
"  -h, --help          Display this help message and exit.\n"
"\n"
	) ;
	
	exit(0);
}

int dmx_handler(lla_con c, int uni, int len , uint8_t *dmx, void *data) {


	printf("dmx handler\n") ;
	return 0 ;
}


/*
 * Connect, fetch plugin listing, and display
 *
 */
int main(int argc, char*argv[]) {
	lla_con con ;
	options opts ;

	init_options(&opts);
	parse_options(argc, argv, &opts) ;

	if(opts.help)
		display_help_and_exit() ;

	// connect
	con = lla_connect() ;

	if (!con) {
		printf("error: %s\n", strerror(errno) ) ;
		exit(1) ;
	}

	if( lla_set_dmx_handler(con, dmx_handler, NULL) ) {
		printf("Failed to set dmx handler\n") ;
	}
	
	lla_read_dmx(con, 0) ;

	lla_sd_action(con, 1) ;
	
	lla_disconnect(con) ;

	return 0;
}
