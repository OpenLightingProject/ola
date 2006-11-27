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
 *  lla-dev-info.cpp
 *  Displays the available devices and ports 
 *  Copyright (C) 2005-2006 Simon Newton
 */

using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <getopt.h>

#include <string>
#include <vector>

#include <lla/LlaClient.h>
#include <lla/LlaDevice.h>
#include <lla/LlaPort.h>
#include <lla/LlaPlugin.h>
#include <lla/LlaUniverse.h>
#include <lla/LlaClientObserver.h>

typedef enum {
	DEV_INFO,
	PLUGIN_INFO,
	PLUGIN_DESC,
	UNI_INFO,
	UNI_NAME,
	SET_DMX,
} mode;


typedef struct {
	mode m;
	string cmd;
	string uni_name;
	string dmx;
	int uni;
	int pid;
	int help;
	int verbose;
} options;

/*
 * The observer class which repsonds to events
 */
class Observer : public LlaClientObserver {

	public:
		Observer(options *opts, LlaClient *cli) : m_opts(opts), m_cli(cli) {};

		int new_dmx(int uni,int length, uint8_t *data) {};
		int universes(const vector <class LlaUniverse *> unis);
		int plugins(const vector <class LlaPlugin *> plugins);
		int devices(const vector <class LlaDevice *> devices);
		int ports(class LlaDevice *dev);
		int plugin_desc(class LlaPlugin *plug) ;

	private:
		options *m_opts;
		LlaClient *m_cli;
};


/*
 *
 */
int Observer::universes(const vector <class LlaUniverse *> unis) {
	vector<LlaUniverse *>::const_iterator iter;

	printf("   ID\t%30s\t\tMerge Mode\n", "Name");
	printf("----------------------------------------------------------\n");

	for(iter = unis.begin(); iter != unis.end(); ++iter) {
		printf("%5d\t%30s\n", (*iter)->get_id(), (*iter)->get_name().c_str()) ;
	}
	printf("----------------------------------------------------------\n");
	return 0;
}


/*
 *
 */
int Observer::plugin_desc(LlaPlugin *plug) {
	printf("%s", plug->get_desc().c_str() ) ;
	return 0;
}


/*
 *
 */
int Observer::plugins(const vector <class LlaPlugin *> plugins) {
	vector<LlaPlugin *>::const_iterator iter;

	if(m_opts->m == PLUGIN_DESC) {
		for(iter = plugins.begin(); iter != plugins.end(); ++iter) {
			if((*iter)->get_id() == m_opts->pid) 
				m_cli->fetch_plugin_desc((*iter));
		}

	} else {
		printf("   ID\tDevice Name\n");
		printf("--------------------------------------\n");

		for(iter = plugins.begin(); iter != plugins.end(); ++iter) {
			printf("%5d\t%s\n", (*iter)->get_id(), (*iter)->get_name().c_str()) ;
		}
		printf("--------------------------------------\n");
	}
	return 0;
}

/*
 *
 *
 */
int Observer::devices(const vector <LlaDevice *> devices) {
	vector<LlaDevice *>::const_iterator iter;

	// get the ports for each device
	for(iter = devices.begin(); iter != devices.end(); ++iter) {
		m_cli->fetch_port_info(*iter);
	}
	return 0;
}


/*
 *
 *
 */
int Observer::ports(LlaDevice *dev) {
	vector<LlaPort *> ports;
	vector<LlaPort *>::const_iterator iter;

	printf("Device %d: %s\n", dev->get_id(),  dev->get_name().c_str() ) ;

	ports = dev->get_ports();

	for( iter = ports.begin(); iter != ports.end(); ++iter) {
		printf("  port %d, cap ", (*iter)->get_id()) ;

		if((*iter)->get_capability() == LlaPort::LLA_PORT_CAP_IN)
			printf("IN");
		else
			printf("OUT");

		if((*iter)->is_active())
			printf(", universe %d", (*iter)->get_uni()) ;
		printf("\n");
	}
}


//-----------------------------------------------------------------------------
//


/*
 * Init options
 */
void init_options(options *opts) {
	opts->m = DEV_INFO;
	opts->uni = -1;
	opts->pid = -1 ;
	opts->help = 0 ;
	opts->verbose = 0;
}


/*
 *
 */
void set_mode(options *opts) {
	int pos = opts->cmd.find_last_of("/");

	if(pos != string::npos) {
		opts->cmd = opts->cmd.substr(pos+1);
	}

	if( opts->cmd == "lla_plugin_info") {
		if( opts->pid == -1) 
			opts->m = PLUGIN_INFO;
		else
			opts->m = PLUGIN_DESC;
	} else if ( opts->cmd == "lla_uni_info") {
		opts->m = UNI_INFO;
	} else if ( opts->cmd == "lla_uni_name") {
		opts->m = UNI_NAME;
	} else if ( opts->cmd == "lla_set_dmx") {
		opts->m = SET_DMX;
	}

}


/*
 * parse our cmd line options
 *
 */
int parse_options(int argc, char *argv[], options *opts) {
	static struct option long_options[] = {
			{"pid", 		required_argument, 	0, 'p'},
			{"help", 		no_argument, 		0, 'h'},
			{"name", 		required_argument, 	0, 'n'},
			{"universe", 	required_argument, 	0, 'u'},
			{"dmx", 		required_argument,	0, 'x'},
			{"verbose", 	no_argument, 		0, 'v'},			
			{0, 0, 0, 0}
		};

	int c;
	int option_index = 0;
	
	while (1) {
     
		c = getopt_long(argc, argv, "x:n:u:p:hv", long_options, &option_index);
		
		if (c == -1)
			break;

		switch (c) {
			case 0:
				break;
			case 'p':
				opts->pid = atoi(optarg);
				break;
			case 'h':
				opts->help = 1;
				break;
			case 'n':
				opts->uni_name = optarg;
				break;
			case 'u':
				opts->uni = atoi(optarg);
				break;
			case 'x':
				opts->dmx = optarg;
		        break;
			case 'v':
				opts->verbose = 1;
				break;
			case '?':
				break;  
			default:
				;
		}
	}
	return 0;
}



void display_dev_info_help(options *opts) {
	printf(
"Usage: %s [--pid <pid> ]\n"
"\n"
"Get info on the devices loaded by llad.\n"
"\n"
"  -h, --help          Display this help message and exit.\n"
"\n",
	opts->cmd.c_str()) ;
}


void display_plugin_info_help(options *opts) {
	printf(
"Usage: %s [--pid <pid> ]\n"
"\n"
"Get info on the plugins loaded by llad. Called without arguments this will\n"
"display the plugins loaded by llad. When used with --pid this wilk display\n"
"the specified plugin's description\n"
"\n"
"  -h, --help          Display this help message and exit.\n"
"  -p, --pid <pid>     Id of the plugin to fetch the description of.\n"
"\n",
	opts->cmd.c_str()) ;
}


/*
 *
 */
void display_uni_info_help(options *opts) {

	printf(
"Usage: %s\n"
"\n"
"Shows info on the active universes in use.\n"
"\n"
"  -h, --help          Display this help message and exit.\n"
"\n",
	opts->cmd.c_str()) ;
}


/*
 * Display the help message
 */
void display_uni_name_help(options *opts) {

	printf(
"Usage: %s --name <name> [--universe <uni>]\n"
"\n"
"Set a name for the specified universe\n"
"\n"
"  -h, --help               Display this help message and exit.\n"
"  -n, --name <name>        Name for the universe.\n"
"  -u, --universe <uni>     Id of the universe to patch to (default 0).\n"
"\n",
	opts->cmd.c_str()) ;
}


/*
 *
 */
void display_set_dmx_help(options *opts) {
	printf(
"Usage: %s --universe <universe> --dmx 0,255,0,255\n"
"\n"
"Sets the DMX values for a universe.\n"
"\n"
"  -h, --help                      Display this help message and exit.\n"
"  -u, --universe <universe>      Universe number.\n"
"  -v, --verbose                   Be verbose.\n"
"  -x, --dmx <values>              Comma separated DMX values.\n"
"\n",
	opts->cmd.c_str()) ;
}


/*
 * Display the help message
 */
void display_help_and_exit(options *opts) {
	switch(opts->m) {
		case DEV_INFO:
			display_dev_info_help(opts);
			break;
		case PLUGIN_INFO:
			display_plugin_info_help(opts);
			break;
		case PLUGIN_DESC:
			display_plugin_info_help(opts);
			break;
		case UNI_INFO:
			display_uni_info_help(opts);
			break;
		case UNI_NAME:
			display_uni_name_help(opts);
			break;
		case SET_DMX:
			display_set_dmx_help(opts);
			break;
		default:
			;
	}
	exit(0);
}


int set_uni_name(LlaClient *cli, options *opts) {

	if(opts->uni_name == "") {
		display_uni_name_help(opts);
		exit(1) ;
	}

	if (cli->set_uni_name(opts->uni, opts->uni_name.c_str())) {
		printf("Failed to set name\n") ;
	}
}


int set_dmx(LlaClient *cli, options *opts) {
	int i=0;
	char *s;
	uint8_t buf[512];
	char *str = strdup(opts->dmx.c_str());

	if( opts->uni < 0 ) {
		display_set_dmx_help(opts) ;
		exit(1);
	}

	for( s = strtok(str, ",") ; s != NULL ; s = strtok(NULL, ",") ) {
		int v  = atoi(s) ;
		buf[i++] = v > 255 ? 255 : v;
	}

	if( cli->send_dmx(opts->uni, buf, i)) {
		printf("Send DMX failed:\n") ;
		return 1;
	}
	free(str);
	return 0;
}


/*
 * Connect, fetch the device listing and display
 *
 *
 */
int main(int argc, char*argv[]) {
	LlaClient lla;
	Observer *ob = NULL;;
	options opts;

	init_options(&opts);
	opts.cmd = argv[0];
	parse_options(argc, argv, &opts) ;
	set_mode(&opts);

	if(opts.help)
		display_help_and_exit(&opts) ;

	ob = new Observer(&opts, &lla);

	// connect
	if ( lla.start() ) {
		printf("error: %s\n", strerror(errno) ) ;
		exit(1) ;
	}

	lla.set_observer(ob);

	if(opts.m == DEV_INFO) {
		lla.fetch_dev_info(LLA_PLUGIN_ALL);
		lla.fd_action(1);
	} else if (opts.m == PLUGIN_INFO || opts.m == PLUGIN_DESC) {
		lla.fetch_plugin_info();
		lla.fd_action(1);
	} else if (opts.m == UNI_INFO) {
		lla.fetch_uni_info();
		lla.fd_action(1);
	} else if (opts.m == UNI_NAME) {
		set_uni_name(&lla, &opts);
	} else if (opts.m == SET_DMX) {
		set_dmx(&lla, &opts);
	}

	return 0;
}
