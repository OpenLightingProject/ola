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
 * usbprodevice.h
 * Interface for the usbpro device
 * Copyright (C) 2006  Simon Newton
 */

#ifndef USBPROWIDGET_H
#define USBPROWIDGET_H

using namespace std;

#include <string>
#include <stdint.h>

#include "UsbProWidgetListener.h"

enum { DMX_BUF_LEN = 513 };
enum { USER_CONFIG_LEN = 508 };

// dmx message
typedef struct {
  uint8_t dmx[DMX_BUF_LEN];
} pms_dmx;

//
typedef struct {
  uint8_t status;
  uint8_t dmx[DMX_BUF_LEN];
} pms_rdmx;

//
typedef struct {
  uint8_t dmx[DMX_BUF_LEN];
} pms_rdm;

// param request
typedef struct {
  uint8_t len;
  uint8_t  len_hi;
} pms_prmreq;

// param reply
typedef struct {
  uint8_t firmv;
  uint8_t firmv_hi;
  uint8_t brtm;
  uint8_t mabtm;
  uint8_t rate;
  uint8_t user[USER_CONFIG_LEN];
} pms_prmrep;

// param set
typedef struct {
  uint8_t len;
  uint8_t len_hi;
  uint8_t brk;
  uint8_t mab;
  uint8_t rate;
  uint8_t user[USER_CONFIG_LEN];
} pms_prmset;

// change recv mode
typedef struct {
  uint8_t  mode;
} pms_rcmode;

// change of state message
typedef struct {
  uint8_t  start;
  uint8_t  changed[5];
  uint8_t data[40];
} pms_cos;

// serial number request
struct pms_snoreq_s {
  uint8_t i[];    // required for a struct size of 0
}__attribute__( ( packed ) );

typedef struct pms_snoreq_s pms_snoreq;

// serial number reply
typedef struct {
  uint8_t srno[4];
} pms_snorep;

// union of all messages
typedef union {
    pms_dmx     pmu_dmx;
    pms_rdmx     pmu_rdmx;
    pms_rdm     pmu_rdm;
    pms_prmreq     pmu_prmreq;
    pms_prmrep    pmu_prmrep;
    pms_prmset    pmu_prmset;
    pms_rcmode    pmu_rcmode;
    pms_cos      pmu_cos;
    pms_snoreq    pmu_snoreq;
    pms_snorep    pmu_snorep;
} pmu;

// the entire message
typedef struct {
  uint8_t som;
  uint8_t label;
  uint8_t len;
  uint8_t len_hi;
  pmu pm_pmu;
} promsg;

#define pm_dmx    pm_pmu.pmu_dmx
#define pm_rdmx    pm_pmu.pmu_rdmx
#define pm_rdm    pm_pmu.pmu_rdm
#define pm_prmreq  pm_pmu.pmu_prmreq
#define pm_prmrep  pm_pmu.pmu_prmrep
#define pm_prmset  pm_pmu.pmu_prmset
#define pm_rcmode  pm_pmu.pmu_rcmode
#define pm_cos    pm_pmu.pmu_cos
#define pm_snoreq  pm_pmu.pmu_snoreq
#define pm_snorep  pm_pmu.pmu_snorep


class UsbProWidget {

  public:
    UsbProWidget() {};
    ~UsbProWidget() {};

    // these methods are for communicating with the device
    int connect(const string &path);
    int disconnect();
    int fd() {return m_fd;}
    int send_dmx(uint8_t *buf, unsigned int len) const;
    int send_rdm(uint8_t *buf, unsigned int len) const;
    int set_params(uint8_t *data, unsigned int len, uint8_t brk, uint8_t mab, uint8_t rate);
    void get_params(uint16_t *firmware, uint8_t *brk, uint8_t *mab, uint8_t *rate) const;
    void get_serial(uint8_t *serial, unsigned int len) const;
    int get_dmx(uint8_t *data, unsigned int len);
    int recv_mode();
    void set_listener(UsbProWidgetListener *l);
    int recv();

  private:
    int init();
    int send_msg(promsg *msg) const;
    int set_msg_len(promsg *msg, int len) const;
    int send_rcmode(int mode);
    int send_snoreq() const;
    int send_prmreq(int usrsz) const;
    int handle_dmx(pms_rdmx *dmx, int len);
    int handle_cos(pms_cos *cos, int len);
    int handle_prmrep(pms_prmrep *rep, unsigned int len);
    int handle_snorep(pms_snorep *rep, int len);
    int do_recv();

    // instance variables
    int m_fd;            // file descriptor
    uint8_t  m_dmx[DMX_BUF_LEN-1];  // dmx buffer
    uint8_t m_serial[4];      // serial number
    pms_prmrep m_params;      // widget params
    bool m_enabled;          // are we enabled
    UsbProWidgetListener *m_listener;
};

#endif
