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
 * universe.hpp
 * Header file for the Universe class
 * Copyright (C) 2005  Simon Newton
 *
 */


#ifndef UNIVERSE_H
#define UNIVERSE_H

#include <stdint.h>
#include <llad/port.h>
#include <lla/messages.h>

#include <vector>
#include <map>
#include <string>

using namespace std;

class Universe {

  public:

    enum merge_mode {
      MERGE_HTP,
      MERGE_LTP
    };

    ~Universe();
    int add_port(Port *prt);
    int remove_port(Port *prt);
    int get_num_ports() const;

    int add_client(class Client *cli);
    int remove_client(class Client *cli);

    int set_dmx(uint8_t *dmx, int length);
    int get_dmx(uint8_t *dmx, int length);
    int get_uid() const;
    int port_data_changed(Port *prt);
    bool in_use() const;
    string get_name() const;
    void set_name(const string &name, bool save = true);
    int send_dmx(class Client *cli);

    void set_merge_mode(Universe::merge_mode mode, bool save = true);
    Universe::merge_mode get_merge_mode();

    static Universe *get_universe(int uid);
    static Universe *get_universe_or_create(int uid);
    static int  universe_count();
    static Universe *get_universe_at_pos(int index);

    static int clean_up();
    static void check_for_unused();
    static vector<Universe *> *get_list();
    static int set_net(class Network *net);
    static int set_store(class UniverseStore *store);

  protected :
    Universe(int uid);

  private:
    Universe(const Universe&);
    Universe& operator=(const Universe&);
    int update_dependants();

    void merge();                         // HTP merge the merge and data buffers
    int m_uid;
    enum merge_mode m_merge_mode;         // merge mode
    vector<Port*> ports_vect;             // ports assigned to the universe
    vector<class Client *> clients_vect;  // clients listening to this universe
    uint8_t  m_data[DMX_LENGTH];          // buffer for this universe
    uint8_t  m_merge[DMX_LENGTH];         // merge buffer for this universe

    int m_length;   // length of valid data in m_data
    int m_mlength;  // length of valid data in m_merge
    string m_name;  // name of this universe

    static map<int, Universe *> uni_map;  // map of uid to universes
    static Network *c_net;                // network object
    static UniverseStore *c_uni_store;    // the universe store object
};

#endif
