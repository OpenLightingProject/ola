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

#include <lla/LlaClient.h>
#include <lla/LlaClientObserver.h>
#include <lla/usbpro/UsbProConfParser.h>
#include <lla/usbpro/UsbProConfMsgs.h>

/*
 * the mode is determined by the name in which we were called
 */
typedef enum {
	NONE,
	GET_PRM,
	GET_SER,
	SET_PRM
} mode;


typedef struct {
	mode m;				// mode
	char *cmd;			// argv[0]
	int dev;			// device id
	int help;			// help ?
	int verbose;		// verbose
	int brk;			// brk
	int mab;			// mab
	int rate;			// rate
} options;

/*
 * The observer class which repsonds to events
 */
class Observer : public LlaClientObserver {

	public:
		Observer(options *opts, LlaClient *cli);
		~Observer() { delete m_parser; }

		int spin();
		int get_params();
		int get_serial();
		int dev_config(unsigned int dev, uint8_t *res, unsigned int len);
		void display_params(int dev, UsbProConfMsgPrmRep *rep);
		int send_set_params(int dev, UsbProConfMsgPrmRep *rep);
		int handle_serial(int dev, UsbProConfMsgSerRep *rep);
		int handle_params(int dev, UsbProConfMsgPrmRep *rep);

	private:
		int m_term;
		options *m_opts;
		LlaClient *m_cli;
		UsbProConfParser *m_parser;
};


/*
 * Observer constructor
 */
Observer::Observer(options *opts, LlaClient *cli) :
	m_term(0),
	m_opts(opts),
	m_cli(cli) {
	m_parser = new UsbProConfParser();
};


/*
 * Loop calling select until we terminate.
 */
int Observer::spin() {
	struct timeval tv;
	fd_set rd_fds;
	int fd = m_cli->fd();
	int n;

	while(! m_term) {
		FD_ZERO(&rd_fds);
		FD_SET(fd, &rd_fds);
		tv.tv_sec = 1;
		tv.tv_usec = 0;

		n = select(fd+1, &rd_fds, NULL, NULL, &tv);

		switch(n) {
			case 0:
				// terminate on timeout
				m_term = 1;
			break ;
			case -1:
			 	printf("select error\n") ;
				break ;
			default:
				if ( FD_ISSET(fd, &rd_fds)) {
					m_cli->fd_action(0);
				}
		}
	}
	return 0;
}


/*
 * send a fetch params request
 * @param cli	the lla client
 * @param opts	the options
 */
int Observer::get_params() {
	UsbProConfMsgPrmReq req;
	
	m_cli->dev_config(m_opts->dev , &req) ;

	return 0;
}


/*
 * send a fetch serial request
 * @param cli	the lla client
 * @param opts	the options
 */
int Observer::get_serial() {
	UsbProConfMsgSerReq req;
	
	m_cli->dev_config(m_opts->dev , &req) ;

	return 0;
}


/*
 * Display the widget parameters
 */
void Observer::display_params(int dev, UsbProConfMsgPrmRep *rep) {
	uint16_t firm = rep->get_firmware();
	printf("Device %i\n", dev);
	printf(" Firmware: %hhi.%hhi\n", (firm & 0xFF00) >> 8 , firm & 0x00FF);
	printf(" Breaktime: %0.2f us\n", rep->get_brk() * 10.67);
	printf(" MAB Time: %0.2f us\n", rep->get_mab() * 10.67);
	printf(" Frame Rate: %i frames/sec\n", rep->get_rate());
}


int Observer::send_set_params(int dev, UsbProConfMsgPrmRep *rep) {
	UsbProConfMsgSprmReq req;

	if( m_opts->brk != -1)
		req.set_brk(m_opts->brk);
	else
		req.set_brk(rep->get_brk());

	if( m_opts->mab != -1)
		req.set_mab(m_opts->mab);
	else
		req.set_mab(rep->get_mab());

	if( m_opts->rate != -1)
		req.set_rate(m_opts->rate);
	else
		req.set_rate(rep->get_rate());

	m_cli->dev_config(dev, &req);
	return 0;
}


/*
 * Handle a serial response
 */
int Observer::handle_serial(int dev, UsbProConfMsgSerRep *rep) {
	uint8_t ser[UsbProConfMsgSerRep::SERIAL_SIZE];

	rep->get_serial(ser, UsbProConfMsgSerRep::SERIAL_SIZE);
	printf("Device %i\n", dev);
	printf(" Serial %i:%i:%i:%i\n", ser[0], ser[1], ser[2], ser[3]);
	return 0;
}


/*
 * Handle a param response
 */
int Observer::handle_params(int dev, UsbProConfMsgPrmRep *rep) {
	
	if( m_opts->m == GET_PRM) {
		display_params(dev,rep); 
	} else {
		// set a set request
		send_set_params(dev,rep);
	}
	return 0;
}


/*
 * This is called when we recieve a device config response
 * @param dev the device id
 * @param res the response buffer
 * @param len the length of the response buffer
 */
int Observer::dev_config(unsigned int dev, uint8_t *rep, unsigned int len) {
	UsbProConfMsg *m = m_parser->parse(rep, len);
	struct timespec tv;

	if( m == NULL) {
		printf("bad response\n");
		return 0;
	}

	switch(m->type()) {

		case LLA_USBPRO_CONF_MSG_PRM_REP:
			handle_params(dev, (UsbProConfMsgPrmRep*) m);
			m_term = 1;
			break;
		case LLA_USBPRO_CONF_MSG_SER_REP:
			handle_serial(dev, (UsbProConfMsgSerRep*) m);
			m_term = 1;
			break;
		case LLA_USBPRO_CONF_MSG_SPRM_REP:
			m_opts->m = GET_PRM;
			tv.tv_sec = 0;
			tv.tv_nsec = 30000000;
			// wait for the widget to update
			nanosleep(&tv, NULL);
			get_params();
			break;
		default:
			delete m;
			return 0;
	}
	delete m;

	return 0;
}


//-----------------------------------------------------------------------------

/*
 * Init options
 */
void init_options(options *opts) {
	opts->m = NONE;
	opts->dev = -1;
	opts->help = 0 ;
	opts->verbose = 0;
	opts->brk = -1;
	opts->mab = -1;
	opts->rate = -1;
}


/*
 * parse our cmd line options
 *
 */
int parse_options(int argc, char *argv[], options *opts) {
	static struct option long_options[] = {
			{"brk", 		required_argument, 	0, 'k'},
			{"dev", 		required_argument, 	0, 'd'},
			{"help", 		no_argument, 		0, 'h'},
			{"mab", 		required_argument, 	0, 'm'},
			{"params", 		required_argument, 	0, 'p'},
			{"rate", 		required_argument,	0, 'r'},
			{"serial", 		required_argument,	0, 's'},
			{"verbose", 	no_argument, 		0, 'v'},			
			{0, 0, 0, 0}
		};

	int c;
	int option_index = 0;
	
	while (1) {
     
		c = getopt_long(argc, argv, "b:d:hm:pr:sv", long_options, &option_index);
		
		if (c == -1)
			break;

		switch (c) {
			case 0:
				break;
			case 'b':
				opts->brk = atoi(optarg);
				break;
			case 'd':
				opts->dev = atoi(optarg);
				break;
			case 'h':
				opts->help = 1;
				break;
			case 'm':
				opts->mab = atoi(optarg);
				break;
			case 'p':
				opts->m = GET_PRM;
				break;
			case 'r':
				opts->rate = atoi(optarg);
				break;
			case 's':
				opts->m = GET_SER;
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


/*
 * Display the help message
 */
void display_help_and_exit(options *opts) {
	printf(
"Usage: %s -d <dev_id> [--params | --serial | -b <brk> -m <mab> -r <rate> ]\n"
"\n"
"Configure Enttec Usb Pro Devices managed by LLA.\n"
"\n"
"  -b, --brk <brk>     Set the break time (9 - 127)\n"
"  -d, --dev <dev_id>  The device id to configure\n"
"  -h, --help          Display this help message and exit.\n"
"  -m, --mab <mab>     Set the make after-break-time (1 - 127)\n"
"  -p, --params        Get the parameters.\n"
"  -r, --rate <rate>   Set the transmission rate (1 - 40).\n"
"  -s, --serial	       Get the serial number.\n"
"  -v, --verbose       Display this message.\n"
"\n",
	opts->cmd) ;

	exit(0);
}


void check_options(options *opts) {

	// check for valid parameters
	if(opts->brk != -1 || opts->mab != -1 || opts->rate != -1)
		opts->m = SET_PRM;

	if (opts->brk != -1 && (opts->brk < 9 || opts->brk > 127) )
		opts->m = NONE;

	if (opts->mab != -1 && (opts->mab < 1 || opts->mab > 127) )
		opts->m = NONE;

	if (opts->rate != -1 && (opts->rate < 1 || opts->rate > 40) )
		opts->m = NONE;
}


/*
 *
 */
int main(int argc, char*argv[]) {
	LlaClient lla;
	Observer *ob = NULL;
	options opts;

	init_options(&opts);
	opts.cmd = argv[0];
	parse_options(argc, argv, &opts) ;
	check_options(&opts);

	if(opts.help || opts.dev < 0 || opts.m == NONE)
		display_help_and_exit(&opts) ;

	// this handles the lla events
	ob = new Observer(&opts, &lla);
	lla.set_observer(ob);

	// connect
	if ( lla.start() ) {
		printf("error: %s\n", strerror(errno) ) ;
		exit(1) ;
	}

	if(opts.m == GET_PRM || opts.m == SET_PRM) {
		ob->get_params();
	} else {
		ob->get_serial();
	}
	
	// loop
	ob->spin();

	lla.stop();
	delete ob;

	return 0;
}
