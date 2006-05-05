/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * 
 * lla-latency2.cpp
 * Measures the time differential between two universe
 * Copyright (C) 2005-2006  Simon Newton
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include <getopt.h>
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>

#include <lla/lla.h>


#define CHANNELS 512

int universe1 = 0;
int universe2 = 1;


static lla_con con ;
uint8_t	dmx1[CHANNELS] ;
uint8_t	dmx2[CHANNELS] ;

struct timeval tv1;
struct timeval tv2;

long worst = 0;
long best = 9999999 ;
long count = 0; 
long total = 0 ;
int behind1 = 0 ;
int behind2 = 0;

int term = 0;


/*
 * Terminate cleanly on interrupt
 */
static void sig_interupt(int signo) {
	term = 1 ;
}

/*
 * Set up the interrupt signal
 *
 * @return 0 on success, non 0 on failure
 */
static int install_signal() {
	struct sigaction act, oact;

	act.sa_handler = sig_interupt;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;

	if (sigaction(SIGINT, &act, &oact) < 0) {
		printf("Failed to install signal") ;
		return -1 ;
	}
	
	if (sigaction(SIGTERM, &act, &oact) < 0) {
		printf("Failed to install signal") ;
		return -1 ;
	}

	return 0;
}


/*
 * called on recv
 */
int dmx_handler(lla_con c, int uni, int length, uint8_t *sdmx, void *d ) {
	int len = length > CHANNELS ? CHANNELS : length ;
	long delay ;

	if(uni == universe1) {
		memcpy(dmx1, sdmx, len) ;
		gettimeofday(&tv1, NULL) ;

		
		printf("%i %li %li 0x%hhx 0x%hhx 0x%hhx 0x%hhx 0x%hhx\n", universe1 ,  tv1.tv_sec , tv1.tv_usec, sdmx[0] , sdmx[1], sdmx[2] , sdmx[3], sdmx[4] ) ;

		if( memcmp(sdmx, dmx2, len) == 0 ) {
			delay = tv1.tv_usec - tv2.tv_usec ;
			printf("universe %i is %li behind \n", universe1 , delay) ;
			count ++ ;
			total += delay ;
			behind1 ++ ;
		}
	} else if ( uni == universe2) {
		memcpy(dmx2, sdmx, len) ;
		gettimeofday(&tv2, NULL) ;

		printf("%i %li %li 0x%hhx 0x%hhx 0x%hhx 0x%hhx 0x%hhx\n",universe2,   tv2.tv_sec , tv2.tv_usec,  sdmx[0] , sdmx[1], sdmx[2] , sdmx[3], sdmx[4] ) ;

		if( memcmp(sdmx, dmx1, len) == 0 ) {
			delay = tv2.tv_usec - tv1.tv_usec ;
			printf("universe %i is %li behind \n", universe2 , delay) ;
			count++;
			total+= delay;
			behind2++ ;
		}

		

	}

	return 0 ;
}


int main (int argc, char *argv[]) {
	int optc ;
	int lla_sd ;
//	struct timeval tv2 ;
	
	install_signal() ;
	
	memset(dmx1, 0x00, CHANNELS) ;
	memset(dmx2, 0x00, CHANNELS) ;

	// parse options 
	while ((optc = getopt (argc, argv, "u:v:")) != EOF) {
		switch (optc) {
			case 'u':
				universe1 = atoi(optarg) ;
				break ;
			case 'v':
				universe2 = atoi(optarg) ;
				break ;
		default:
			break;
		}
	}

	/* set up lla connection */
	con = lla_connect() ; ;
	
	if(con == NULL) {
		printf("Unable to connect\n") ;
		return 1 ;
	}

	if(lla_set_dmx_handler(con, dmx_handler, NULL) ) {
		printf("Failed to install handler\n") ;
		return 1 ;
	}

	if(lla_reg_uni(con, universe1, 1) ) {
		printf("REgister uni %d failed\n", universe1) ;
		return 1 ;
	}
	
	if(lla_reg_uni(con, universe2, 1) ) {
		printf("REgister uni %d failed\n", universe2) ;
		return 1 ;
	}

	// store the sds
	lla_sd = lla_get_sd(con) ;
  
	/* main loop */
	while (! term) {
		int n, max;
		fd_set rd_fds;
		struct timeval tv;

		FD_ZERO(&rd_fds);
		FD_SET(lla_sd, &rd_fds) ;

		max = lla_sd ;

		tv.tv_sec = 0;
		tv.tv_usec = 40000;

		n = select(max+1, &rd_fds, NULL, NULL, &tv);
		if(n>0) {
			if (FD_ISSET(lla_sd, &rd_fds) ) {
//				gettimeofday(&tv2, NULL) ;
//				printf(" got read %ld %ld\n", tv2.tv_sec, tv2.tv_usec) ;		
	    		lla_sd_action(con,0);
			}
		}

	}
	lla_disconnect(con) ;

	printf("1: %i 2: %i Avg %ld\n", behind1, behind2, total/count) ;


	return 0;
}
