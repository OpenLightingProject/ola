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
 *
 * usbprodevice.h
 * Interface for the usbpro device
 * Copyright (C) 2006  Simon Newton
 */

#ifndef USBPRODEVICE_H
#define USBPRODEVICE_H

#include <string> 
#include <stdint.h>
#include <lla/device.h>
#include <lla/fdlistener.h>

#define DMX_BUF_LEN 	513
#define USER_CONFIG_LEN	508

typedef struct {
	uint8_t dmx[DMX_BUF_LEN] ;
} pms_dmx ;

typedef struct {
	uint8_t status;
	uint8_t dmx[DMX_BUF_LEN] ;
} pms_rdmx ;


typedef struct {
	uint8_t dmx[DMX_BUF_LEN] ;
} pms_rdm ;


typedef struct {
	uint8_t len ;
	uint8_t	len_hi ;
} pms_prmreq;

typedef struct {
	uint8_t firmv ;
	uint8_t firmv_hi;
	uint8_t brtm ;
	uint8_t mabtm;
	uint8_t rate;
	uint8_t user[USER_CONFIG_LEN];
} pms_prmrep;


typedef struct {
	uint8_t	mode ;
} pms_rcmode;

typedef struct {
	uint8_t	start;
	uint8_t	changed[5];
	uint8_t data[40];
} pms_cos;


typedef struct {

} pms_snoreq;


typedef struct {
	uint8_t srno[4] ;
} pms_snorep;


typedef union {
		pms_dmx 		pmu_dmx ;
		pms_rdmx 		pmu_rdmx ;
		pms_rdm 		pmu_rdm ;
		pms_prmreq 		pmu_prmreq;
		pms_prmrep		pmu_prmrep;
		pms_prmrep		pmu_prmset;
		pms_rcmode		pmu_rcmode;
		pms_cos			pmu_cos;
		pms_snoreq		pmu_snoreq;
		pms_snorep		pmu_snorep;
} pmu;


typedef struct {
	uint8_t som;
	uint8_t label;
	uint8_t len;
	uint8_t len_hi;
	pmu pm_pmu;
} promsg ;

#define pm_dmx		pm_pmu.pmu_dmx
#define pm_rdmx		pm_pmu.pmu_rdmx
#define pm_rdm		pm_pmu.pmu_rdm
#define pm_prmreq	pm_pmu.pmu_prmreq
#define pm_prmrep	pm_pmu.pmu_prmrep
#define pm_prmset	pm_pmu.pmu_prmset
#define pm_rcmode	pm_pmu.pmu_rcmode
#define pm_cos		pm_pmu.pmu_cos
#define pm_snoreq	pm_pmu.pmu_snoreq
#define pm_snorep	pm_pmu.pmu_snorep


class UsbProDevice : public Device, public FDListener {

	public:
		UsbProDevice(Plugin *owner, const char *name, string dev_path) ;
		~UsbProDevice() ;

		int start() ;
		int stop() ;
		int get_sd() const ;
		int fd_action() ;
		int save_config();
		int configure(void *req, int len) ;
		int send_dmx(uint8_t *data, int len) ;
		int get_dmx(uint8_t *data, int len) ;

	private:
		// these methods are for communicating with the device
		int w_connect(const char *dev) ;
		int w_disconnect() ;
		int w_init() ;
		int w_set_msg_len(promsg *msg, int len);
		int w_send_msg(promsg *msg);
		int w_send_dmx(uint8_t *buf, int len);
		int w_send_rdm(uint8_t *buf, int len);
		int w_send_prmreq(int usrsz)  ;
		int w_send_rcmode(int mode) ;
		int w_send_snoreq();
		int w_handle_dmx(pms_rdmx *dmx, int len) ;
		int w_handle_cos(pms_cos *cos, int len);
		int w_handle_prmrep(pms_prmrep *rep, int len);
		int w_handle_snorep(pms_snorep *rep, int len);
		int w_recv() ;

		string m_dev_path ;
		int m_fd ;
		uint8_t	m_dmx[DMX_BUF_LEN] ;
		bool m_enabled ;
};

#endif
