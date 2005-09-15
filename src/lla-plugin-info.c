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
 *  lla-plugin-info.c
 *  Displays the loaded plugins
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
	int pid;
	int help;
} options;

/*
 * parse our cmd line options
 *
 */
int parse_options(int argc, char *argv[], options *opts) {
	static struct option long_options[] = {
			{"pid", 		required_argument, 	0, 'p'},
			{"help", 		no_argument, 		0, 'h'},
			{0, 0, 0, 0}
		};

	int c ;
	int option_index = 0;
	
	while (1) {
     
		c = getopt_long(argc, argv, "p:h", long_options, &option_index);
		
		if (c == -1)
			break;

		switch (c) {
			case 0:
				break;
				
			case 'p':
				opts->pid = atoi(optarg) ;
				break ;
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
	opts->pid = -1 ;
	opts->help = 0 ;
}


/*
 * Display the help message
 */
void display_help_and_exit() {

	printf(
"Usage: lla_plugin_info [--pid <pid> ]\n"
"\n"
"Get info on the plugins loaded by llad. Called without arguments this will\n"
"display the plugins loaded by llad. When used with --pid this wilk display\n"
"the specified plugin's description\n"
"\n"
"  -h, --help          Display this help message and exit.\n"
"  -p, --pid <pid>     Id of the plugin to fetch the description of.\n"
"\n"
	) ;
	
	exit(0);
}



/*
 * Connect, fetch plugin listing, and display
 *
 */
int main(int argc, char*argv[]) {
	lla_con con ;
	lla_plugin *plugitr ;
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

	if(opts.pid == -1) {

		if( (plugitr =  lla_req_plugin_info(con)) ) {
			printf("   ID\tDevice Name\n");
			printf("--------------------------------------\n");
			while( plugitr != NULL) {
				printf("%5d\t%s\n", plugitr->id, plugitr->name) ;
				plugitr = plugitr->next ;
			}
			
			printf("--------------------------------------\n");
		} else {
			printf("Failed to fetch plugin info!\n") ;
		}
	
	
	} else {
		printf("%s", lla_req_plugin_desc(con, opts.pid) ) ;

	}
	lla_disconnect(con) ;

	return 0;
}
