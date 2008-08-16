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
 * UsbProDevice.h
 * Interface for the usbpro device
 * Copyright (C) 2006-2007 Simon Newton
 */

#ifndef USBPRODEVICE_H
#define USBPRODEVICE_H

#include <string>
#include <stdint.h>
#include <llad/device.h>
#include <llad/listener.h>
#include <llad/timeoutlistener.h>
#include "usbpro_conf_messages.h"

#include "UsbProWidgetListener.h"
#include "UsbProWidget.h"


class UsbProDevice : public Device, public Listener, public UsbProWidgetListener {

  public:

    UsbProDevice(Plugin *owner, const string &name, const string &dev_path);
    ~UsbProDevice();

    int start();
    int stop();
    int get_sd() const;
    int action();
    int save_config() const;
    class LlaDevConfMsg *configure(const uint8_t *request, int reql);
    int send_dmx(uint8_t *data, int len);
    int get_dmx(uint8_t *data, int len) const;
    void new_dmx();
    int recv_mode();

  private:
    class UsbProConfMsg *config_get_params(class UsbProConfMsgPrmReq *req) const;
    class UsbProConfMsg *config_get_serial(class UsbProConfMsgSerReq *req) const;
    class UsbProConfMsg *config_set_params(class UsbProConfMsgSprmReq *req);

    enum {
      SEND_MODE,
      RECV_MODE
    };

    // instance variables
    string m_path;
    bool m_enabled;            // are we enabled
    bool m_mode;              // are we sending or receiving?
    class UsbProConfParser *m_parser; // parser for config msgs
    UsbProWidget *m_widget;
};

#endif
