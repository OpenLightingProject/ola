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
 * stageprofiplugin.h
 * Interface for the dmx4linux plugin class
 * Copyright (C) 2006-2007 Simon Newton
 */

#ifndef DMX4LINUXPLUGIN_H
#define DMX4LINUXPLUGIN_H

#include <vector>
#include <string>
#include <llad/plugin.h>
#include <llad/listener.h>
#include <lla/plugin_id.h>

class Dmx4LinuxDevice;

class Dmx4LinuxPlugin : public Plugin, public Listener {

  public:
    Dmx4LinuxPlugin(const PluginAdaptor *pa, lla_plugin_id id) :
      Plugin(pa, id),
      m_out_fd(-1),
      m_in_fd(-1) {}
    ~Dmx4LinuxPlugin() {}

    string get_name() const { return PLUGIN_NAME; }
    string get_desc() const;
    int action();
    int send_dmx(int d4l_uni, uint8_t *data, int length);

  protected:
    string pref_suffix() const { return PLUGIN_PREFIX; }

  private:
    int start_hook();
    int stop_hook();
    int set_default_prefs();
    int open_fds();
    int close_fds();
    int get_uni_count(int dir);
    int setup_device(string family, int d4l_uni, int dir);
    int setup_devices(int dir);
    int setup();

    vector<Dmx4LinuxDevice *>  m_devices;  // list of out devices
    string m_out_dev;  // path the the dmx device
    string m_in_dev;   // path the the dmx device
    int m_out_fd;      // fd for the output dmx device
    int m_in_fd;       // fd for the input dmx device

    static const string PLUGIN_NAME;
    static const string PLUGIN_PREFIX;
};

#endif
