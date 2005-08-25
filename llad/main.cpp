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
 * main.cpp
 * The main daemon file
 * Copyright (C) 2005  Simon Newton
 *
 */

#include "llad.h" 
#include <lla/logger.h>

#include <signal.h>
#include <stdio.h>

using namespace std;

Llad *llad ;

/*
 * Terminate cleanly on interrupt
 */
static void sig_interupt(int signo) {
	llad->terminate() ;
}


/*
 * Set up the interrupt signal
 *
 */
static int install_signal() {
	struct sigaction act, oact;

	act.sa_handler = sig_interupt;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;

	if (sigaction(SIGINT, &act, &oact) < 0) {
		Logger::instance()->log(Logger::WARN, "Failed to install signal") ;
		return -1 ;
	}

	return 0;
}


/*
 * parse our cmd line options
 *
 */
int parse_options(int argc, char *argv[]) {
	int optc, ll ;
	
	Logger::Output log_output = Logger::STDERR ;
	Logger::Level log_level = Logger::CRIT ;
	
	while ((optc = getopt (argc, argv, "sd:")) != EOF) {
		switch (optc) {
			case 's':
				// log to syslog instead
				log_output = Logger::SYSLOG ;
		        break;
			case 'd':
				ll = atoi(optarg) ;

				switch(ll) {
					case 0:
						// nothing is written at this level
						// so this turns logging off
						log_level = Logger::EMERG ;
						break ;
					case 1:
						log_level = Logger::CRIT ;
						break ;
					case 2:
						log_level = Logger::WARN ;
						break;
					case 3:
						log_level = Logger::INFO ;
						break ;
					case 4:
						log_level = Logger::DEBUG ;
						break ;
					default :
						break;
				}
				break ;
			default:
				break;
    	}
	}

	Logger::instance(log_level, log_output) ;
}

/*
 * Main
 *
 * need to sort out logging here
 */
int main(int argc, char*argv[]) {

	parse_options(argc, argv) ;
	
	llad = new Llad() ;
	
	install_signal() ;

	if(llad->init() == 0 ) {
		llad->run() ;
	}
	delete llad;

	return 0 ;
}
