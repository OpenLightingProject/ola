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
 * Dmx4LinuxPlugin.cpp
 * The Dmx4Linux plugin for lla
 * Copyright (C) 2006-2007 Simon Newton
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <dmx/dmxioctl.h>

#include <llad/pluginadaptor.h>
#include <llad/preferences.h>
#include <llad/logger.h>

#include "Dmx4LinuxPlugin.h"
#include "Dmx4LinuxDevice.h"
#include "Dmx4LinuxPort.h"

// TODO: clean this up
static const string DMX4LINUX_OUT_DEVICE = "/dev/dmx";
static const string DMX4LINUX_IN_DEVICE  = "/dev/dmxin";
static const string IN_DEV_KEY = "in_device";
static const string OUT_DEV_KEY = "out_device";
static const int CHANNELS_PER_UNI = 512;
static const char PLUG_NAME[] = "Dmx4LinuxPlugin";
const string Dmx4LinuxPlugin::PLUGIN_NAME = "Dmx4Linux Plugin";
const string Dmx4LinuxPlugin::PLUGIN_PREFIX = "dmx4linux";


/*
 * Entry point to this plugin
 */
extern "C" Plugin* create(const PluginAdaptor *pa) {
  return new Dmx4LinuxPlugin(pa, LLA_PLUGIN_DMX4LINUX);
}

/*
 * Called when the plugin is unloaded
 */
extern "C" void destroy(Plugin* plug) {
  delete plug;
}


/*
 * Start the plugin
 */
int Dmx4LinuxPlugin::start_hook() {

  if(open_fds())
    return -1;

  if(setup()) {
    close_fds();
    return -1;
  }

  if (m_devices.size() > 0) {
    m_pa->register_fd(m_in_fd, PluginAdaptor::READ, this, NULL);
    return 0;
  } else {
    close_fds();
    return 0;
  }
}


/*
 * Stop the plugin
 *
 * @return 0 on sucess, -1 on failure
 */
int Dmx4LinuxPlugin::stop_hook() {
  vector<Dmx4LinuxDevice *>::iterator it;

  m_pa->unregister_fd(m_in_fd, PluginAdaptor::READ);

  for (it = m_devices.begin(); it != m_devices.end(); ++it) {
    if ((*it)->stop())
      continue;

    m_pa->unregister_device(*it);
    delete *it;
  }

  close_fds();
  m_devices.clear();
  return 0;
}

/*
 * return the description for this plugin
 *
 */
string Dmx4LinuxPlugin::get_desc() const {
    return
"DMX 4 Linux Plugin\n"
"----------------------------\n"
"\n"
"This plugin exposes DMX 4 Linux devices. For now we just support output.\n"
"\n"
"--- Config file : lla-dmx4linux.conf ---\n"
"\n"
"in_device =  /dev/dmxin\n"
"out_device = /dev/dmx\n";
}


/*
 * Called when there is input for us
 * TODO: get reads working
 * why do we get input on the in_fd when we write ??
 */
int Dmx4LinuxPlugin::action() {
  uint8_t buf[512];
  int r;
  r = read(m_in_fd, buf, sizeof(buf));
  // map d4l_uni to ports
  // prt->new_data(buf, len)
  return 0;
}


/*
 * Send dmx
 */
int Dmx4LinuxPlugin::send_dmx(int d4l_uni, uint8_t *data, int len) {
  if (lseek(m_out_fd, CHANNELS_PER_UNI * d4l_uni, SEEK_SET) == CHANNELS_PER_UNI * d4l_uni) {
    int r = write(m_out_fd, data, len);
    if (r != len) {
      Logger::instance()->log(Logger::WARN, "%s: only wrote %d/%d bytes: %s",
        PLUG_NAME, r, len, strerror(errno));
    }
  } else {
    Logger::instance()->log(Logger::WARN, "%s: failed to seek: %s",
      PLUG_NAME, strerror(errno));
  }
  return 0;
}


/*
 * load the plugin prefs and default to sensible values
 *
 */
int Dmx4LinuxPlugin::set_default_prefs() {
  bool save = false;

  if ( m_prefs == NULL)
    return -1;

  if ( m_prefs->get_val(IN_DEV_KEY) == "") {
    m_prefs->set_val(IN_DEV_KEY, DMX4LINUX_IN_DEVICE);
    save = true;
  }

  if ( m_prefs->get_val(OUT_DEV_KEY) == "") {
    m_prefs->set_val(OUT_DEV_KEY, DMX4LINUX_OUT_DEVICE);
    save = true;
  }

  if (save)
    m_prefs->save();

  if (m_prefs->get_val(IN_DEV_KEY) == "" || m_prefs->get_val(OUT_DEV_KEY) == "")
    return -1;

  m_in_dev = m_prefs->get_val(IN_DEV_KEY);
  m_out_dev = m_prefs->get_val(OUT_DEV_KEY);
  return 0;
}


/*
 * open the input and output fds
 */
int Dmx4LinuxPlugin::open_fds() {
  if(m_in_fd < 0 && m_out_fd < 0) {
    m_out_fd = open(m_out_dev.c_str(), O_WRONLY);

    if (m_out_fd < 0) {
      Logger::instance()->log(Logger::WARN, "%s: failed to open %s %s",
        PLUG_NAME, m_out_dev.c_str(), strerror(errno));
      return -1;
    }

    m_in_fd = open(m_in_dev.c_str(), O_RDONLY | O_NONBLOCK);
    if (m_out_fd < 0) {
      Logger::instance()->log(Logger::WARN, "%s: failed to open %s %s",
        PLUG_NAME, m_in_dev.c_str(), strerror(errno));
      close(m_out_fd);
      return -1;
    }
    return 0;
  }
  return -1;
}


/*
 * Close all fds
 */
int Dmx4LinuxPlugin::close_fds() {
  close(m_in_fd);
  close(m_out_fd);
  return 0;
}


/*
 * Return the max # of dmx4linux universes
 * @param dir 0 if ouput, 1 for input
 */
int Dmx4LinuxPlugin::get_uni_count(int dir) {
  struct dmx_info info;

  if (ioctl(m_in_fd, DMX_IOCTL_GET_INFO, &info) >= 0) {
    return dir ? info.max_in_universes : info.max_out_universes;
  }
  return -1;
}


/*
 * setup a single device
 * @param family the dmx4linux family
 * @param d4l_uni the dmx4linux universe
 * @param dir   in|out
 */
int Dmx4LinuxPlugin::setup_device(string family, int d4l_uni, int dir) {
  Dmx4LinuxPort *prt;
  Dmx4LinuxDevice *dev = new Dmx4LinuxDevice(this, family);

  if (dev == NULL)
    return -1;

  if (dev->start()) {
    Logger::instance()->log(Logger::WARN, "%s: couldn't start device",
      PLUG_NAME);
    delete dev;
    return -1;
  }

  prt = new Dmx4LinuxPort(dev, 0, d4l_uni, dir > 0, dir == 0);

  if (prt == NULL) {
    Logger::instance()->log(Logger::WARN, "%s: couldn't create port",
      PLUG_NAME);
    delete dev;
    return -1;
  }

  dev->add_port(prt);
  m_pa->register_device(dev);
  m_devices.insert(m_devices.end(), dev);

  return 0;
}


/*
 * Find all the devices connected and setup ports for them.
 */
int Dmx4LinuxPlugin::setup_devices(int dir) {
  struct dmx_capabilities cap;
  int unis = get_uni_count(dir);

  if (unis < 0) {
    Logger::instance()->log(Logger::WARN, "%s: failed to fetch universe list",
      PLUG_NAME);
    return -1;
  }

  for (int i=0; i < unis; i++) {
    cap.direction = dir ? DMX_DIRECTION_INPUT : DMX_DIRECTION_OUTPUT;
    cap.universe = i;

    if (ioctl(m_in_fd, DMX_IOCTL_GET_CAP, &cap)>=0) {
      if (cap.maxSlots > 0) {
        setup_device(cap.family, cap.universe, cap.direction);
      }
    }
  }
  return 0;
}

/*
 * check what universes are available and setup devices for them
 */
int Dmx4LinuxPlugin::setup() {
  int r = setup_devices(DMX_DIRECTION_INPUT);
  r |= setup_devices(DMX_DIRECTION_OUTPUT);
  return r;
}
