/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * LlaClient.h
 * Interface to the LLA Client class
 * Copyright (C) 2005-2007 Simon Newton
 */

#ifndef LLA_CLIENT_H
#define LLA_CLIENT_H

using namespace std;

#include <string>
#include <vector>
#include <stdint.h>

#include <lla/plugin_id.h>
#include <lla/messages.h>
#include <lla/LlaUniverse.h>

class LlaClient {

  public:
    enum PatchAction {PATCH, UNPATCH};
    enum RegisterAction {REGISTER, UNREGISTER};

    LlaClient();
    ~LlaClient();

    int start();
    int stop();
    int fd() const;
    int fd_action(unsigned int delay);

    int set_observer(class LlaClientObserver *o);

    // dmx methods
    int send_dmx(unsigned int universe, uint8_t *data, unsigned int length);
    int fetch_dmx(unsigned int uni);

    // rdm methods
    // int send_rdm(int universe, uint8_t *data, int length);

    int fetch_dev_info(lla_plugin_id filter);
    int fetch_port_info(class LlaDevice *dev);
    int fetch_uni_info();
    int fetch_plugin_info();
    int fetch_plugin_desc(class LlaPlugin *plug);

    int set_uni_name(unsigned int uni, const string &name);
    int set_uni_merge_mode(unsigned int uni, LlaUniverse::merge_mode mode);

    int register_uni(unsigned int uni, LlaClient::RegisterAction action);
    int patch(unsigned int dev, unsigned int port, LlaClient::PatchAction action, unsigned int uni);
    int dev_config(unsigned int dev, const class LlaDevConfMsg *msg);

  private:
    LlaClient(const LlaClient&);
    LlaClient operator=(const LlaClient&);

    static const unsigned int LLAD_PORT = 8898;  // port to connect to
    static const string LLAD_ADDR;        // address to bind to
    static const unsigned int MAX_DMX = 512;

    int receive(unsigned int delay);
    int send_msg(lla_msg *msg);
    int read_msg();
    int handle_msg(lla_msg *msg);
    int handle_dmx(lla_msg *msg);
    int handle_syn_ack(lla_msg *msg);
    int handle_fin_ack(lla_msg *msg);
    int handle_dev_info(lla_msg *msg);
    int handle_plugin_info(lla_msg *msg);
    int handle_port_info(lla_msg *msg);
    int handle_plugin_desc(lla_msg *msg);
    int handle_universe_info(lla_msg *msg);
    int handle_dev_conf(lla_msg *msg);

    int send_syn();
    int send_fin();

    int clear_plugins();
    int clear_universes();
    int clear_devices();
    int lla_recv(unsigned int delay);

    // instance vars
    int m_sd;
    int m_connected;
    class LlaClientObserver *m_observer;
    int m_seq;
    vector<class LlaDevice *> m_devices;
    vector<class LlaPlugin *> m_plugins;
    vector<class LlaUniverse*> m_unis;
    char *desc;
};
#endif
