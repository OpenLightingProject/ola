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

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <dmx/dmxioctl.h>

#include <lla/BaseTypes.h>
#include <lla/Logging.h>
#include <llad/PluginAdaptor.h>
#include <llad/Preferences.h>

#include "Dmx4LinuxPlugin.h"
#include "Dmx4LinuxDevice.h"
#include "Dmx4LinuxPort.h"


/*
 * Entry point to this plugin
 */
extern "C" lla::AbstractPlugin* create(const lla::PluginAdaptor *plugin_adaptor) {
  return new lla::plugin::Dmx4LinuxPlugin(plugin_adaptor);
}

/*
 * Called when the plugin is unloaded
 */
extern "C" void destroy(lla::AbstractPlugin* plugin) {
  delete plugin;
}


namespace lla {
namespace plugin {

static const string DMX4LINUX_OUT_DEVICE = "/dev/dmx";
static const string DMX4LINUX_IN_DEVICE  = "/dev/dmxin";
static const string IN_DEV_KEY = "in_device";
static const string OUT_DEV_KEY = "out_device";
static const char PLUG_NAME[] = "Dmx4LinuxPlugin";
const string Dmx4LinuxPlugin::PLUGIN_NAME = "Dmx4Linux Plugin";
const string Dmx4LinuxPlugin::PLUGIN_PREFIX = "dmx4linux";


Dmx4LinuxPlugin::~Dmx4LinuxPlugin() {
  CleanupSockets();
}


/*
 * Start the plugin
 */
bool Dmx4LinuxPlugin::StartHook() {
  if(!SetupSockets())
    return false;

  if(Setup()) {
    CleanupSockets();
    return -1;
  }

  if (m_devices.size() > 0) {
    m_plugin_adaptor->AddSocket(m_in_socket, NULL);
    return 0;
  } else {
    CleanupSockets();
    return 0;
  }
}


/*
 * Stop the plugin
 *
 * @return true on sucess, false on failure
 */
bool Dmx4LinuxPlugin::StopHook() {
  vector<Dmx4LinuxDevice *>::iterator it;
  m_plugin_adaptor->RemoveSocket(m_in_socket);

  for (it = m_devices.begin(); it != m_devices.end(); ++it) {
    (*it)->Stop();
    m_plugin_adaptor->UnregisterDevice(*it);
    delete *it;
  }
  CleanupSockets();
  m_devices.clear();
  return 0;
}

/*
 * return the description for this plugin
 *
 */
string Dmx4LinuxPlugin::Description() const {
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
int Dmx4LinuxPlugin::SocketReady(lla::select_server::ConnectedSocket *socket) {
  uint8_t buf[DMX_UNIVERSE_SIZE];
  unsigned int data_read;
  socket->Receive((uint8_t*) buf, sizeof(buf), data_read);
  // map d4l_uni to ports
  // prt->new_data(buf, len)
  return 0;
}


/*
 * Send dmx
 */
int Dmx4LinuxPlugin::SendDmx(int d4l_uni, uint8_t *data, int len) {
  int fd = m_out_socket->WriteDescriptor();
  int offset = DMX_UNIVERSE_SIZE * d4l_uni;
  if (lseek(fd, offset, SEEK_SET) == offset) {
    ssize_t r = m_out_socket->Send(data, len);
    if (r != len) {
      LLA_WARN << "only wrote " << r << "/" << len << " bytes: " <<
        strerror(errno);
    }
  } else {
    LLA_WARN << "failed to seek: " << strerror(errno);
  }
  return 0;
}


/*
 * load the plugin prefs and default to sensible values
 */
bool Dmx4LinuxPlugin::SetDefaultPreferences() {
  bool save = false;

  if (!m_preferences)
    return false;

  if (m_preferences->GetValue(IN_DEV_KEY).empty()) {
    m_preferences->SetValue(IN_DEV_KEY, DMX4LINUX_IN_DEVICE);
    save = true;
  }

  if (m_preferences->GetValue(OUT_DEV_KEY).empty()) {
    m_preferences->SetValue(OUT_DEV_KEY, DMX4LINUX_OUT_DEVICE);
    save = true;
  }

  if (save)
    m_preferences->Save();

  if (m_preferences->GetValue(IN_DEV_KEY).empty() ||
      m_preferences->GetValue(OUT_DEV_KEY).empty())
    return false;

  m_in_dev = m_preferences->GetValue(IN_DEV_KEY);
  m_out_dev = m_preferences->GetValue(OUT_DEV_KEY);
  return true;
}


/*
 * open the input and output fds
 */
bool Dmx4LinuxPlugin::SetupSockets() {
  if(!m_in_socket && !m_out_socket) {
    int fd = open(m_out_dev.c_str(), O_WRONLY);

    if (fd < 0) {
      LLA_WARN << "failed to open " << m_out_dev << " " << strerror(errno);
      return false;
    }
    m_in_socket = new ConnectedSocket(fd, fd);

    fd = open(m_in_dev.c_str(), O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
      LLA_WARN << "failed to open " << m_in_dev << " " << strerror(errno);
      CleanupSockets();
      return false;
    }
    m_out_socket = new ConnectedSocket(fd, fd);
    return true;
  }
  return false;
}


/*
 * Close all fds
 */
int Dmx4LinuxPlugin::CleanupSockets() {
  if (m_in_socket) {
    m_in_socket->Close();
    delete m_in_socket;
    m_in_socket = NULL;
  }

  if (m_out_socket) {
    m_out_socket->Close();
    delete m_out_socket;
    m_out_socket = NULL;
  }
  return 0;
}


/*
 * Return the max # of dmx4linux universes
 * @param dir 0 if ouput, 1 for input
 */
int Dmx4LinuxPlugin::GetDmx4LinuxDeviceCount(int dir) {
  struct dmx_info info;

  if (ioctl(m_in_socket->ReadDescriptor(), DMX_IOCTL_GET_INFO, &info) >= 0) {
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
bool Dmx4LinuxPlugin::SetupDevice(string family, int d4l_uni, int dir) {
  Dmx4LinuxPort *prt;
  Dmx4LinuxDevice *dev = new Dmx4LinuxDevice(this, family);

  if (!dev)
    return false;

  if (dev->Start()) {
    LLA_WARN << "couldn't start device";
    delete dev;
    return false;
  }

  prt = new Dmx4LinuxPort(dev, 0, d4l_uni, dir > 0, dir == 0);

  if (!prt) {
    LLA_WARN << "couldn't create port";
    delete dev;
    return false;
  }

  dev->AddPort(prt);
  m_plugin_adaptor->RegisterDevice(dev);
  m_devices.insert(m_devices.end(), dev);
  return true;
}


/*
 * Find all the devices connected and setup ports for them.
 */
bool Dmx4LinuxPlugin::SetupDevices(int dir) {
  struct dmx_capabilities cap;
  int device_count = GetDmx4LinuxDeviceCount(dir);

  if (device_count < 0) {
    LLA_WARN << "failed to fetch universe list";
    return false;
  }

  for (int i = 0; i < device_count; i++) {
    cap.direction = dir ? DMX_DIRECTION_INPUT : DMX_DIRECTION_OUTPUT;
    cap.universe = i;

    if (ioctl(m_in_socket->ReadDescriptor(), DMX_IOCTL_GET_CAP, &cap) >= 0) {
      if (cap.maxSlots > 0) {
        SetupDevice(cap.family, cap.universe, cap.direction);
      }
    }
  }
  return true;
}

/*
 * check what universes are available and setup devices for them
 */
bool Dmx4LinuxPlugin::Setup() {
  bool ret = SetupDevices(DMX_DIRECTION_INPUT);
  ret &= SetupDevices(DMX_DIRECTION_OUTPUT);
  return ret;
}

} //plugin
} //lla
