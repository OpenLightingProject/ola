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
 * The Dmx4Linux plugin for ola
 * Copyright (C) 2006-2009 Simon Newton
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

#include <string>
#include <vector>

#include "ola/BaseTypes.h"
#include "ola/Logging.h"
#include "ola/StringUtils.h"
#include "olad/PluginAdaptor.h"
#include "olad/Preferences.h"

#include "plugins/dmx4linux/Dmx4LinuxDevice.h"
#include "plugins/dmx4linux/Dmx4LinuxPlugin.h"
#include "plugins/dmx4linux/Dmx4LinuxPort.h"
#include "plugins/dmx4linux/Dmx4LinuxSocket.h"


/*
 * Entry point to this plugin
 */
extern "C" ola::AbstractPlugin* create(
    const ola::PluginAdaptor *plugin_adaptor) {
  return new ola::plugin::dmx4linux::Dmx4LinuxPlugin(plugin_adaptor);
}

/*
 * Called when the plugin is unloaded
 */
extern "C" void destroy(ola::AbstractPlugin* plugin) {
  delete plugin;
}


namespace ola {
namespace plugin {
namespace dmx4linux {

const char Dmx4LinuxPlugin::DMX4LINUX_OUT_DEVICE[] = "/dev/dmx";
const char Dmx4LinuxPlugin::DMX4LINUX_IN_DEVICE[] = "/dev/dmxin";
const char Dmx4LinuxPlugin::IN_DEV_KEY[] = "in_device";
const char Dmx4LinuxPlugin::OUT_DEV_KEY[] = "out_device";
const char Dmx4LinuxPlugin::PLUGIN_NAME[] = "Dmx4Linux Plugin";
const char Dmx4LinuxPlugin::PLUGIN_PREFIX[] = "dmx4linux";


Dmx4LinuxPlugin::~Dmx4LinuxPlugin() {
  CleanupSockets();
}


/*
 * Start the plugin
 */
bool Dmx4LinuxPlugin::StartHook() {
  if (!SetupSockets())
    return false;

  if (!SetupDevices()) {
    CleanupSockets();
    return false;
  }

  if (m_devices.size() > 0) {
    m_in_socket->SetOnData(
        ola::NewClosure(this, &Dmx4LinuxPlugin::SocketReady));
    m_plugin_adaptor->AddSocket(m_in_socket);
    return true;
  } else {
    CleanupSockets();
    return false;
  }
}


/*
 * Stop the plugin
 *
 * @return true on sucess, false on failure
 */
bool Dmx4LinuxPlugin::StopHook() {
  vector<Dmx4LinuxDevice*>::iterator it;
  m_plugin_adaptor->RemoveSocket(m_in_socket);

  for (it = m_devices.begin(); it != m_devices.end(); ++it) {
    m_plugin_adaptor->UnregisterDevice(*it);
    (*it)->Stop();
    delete *it;
  }
  CleanupSockets();
  m_devices.clear();
  m_in_ports.clear();
  return 0;
}


/*
 * Return the description for this plugin
 */
string Dmx4LinuxPlugin::Description() const {
    return
"DMX 4 Linux Plugin\n"
"----------------------------\n"
"\n"
"This plugin exposes DMX 4 Linux devices.\n"
"\n"
"--- Config file : ola-dmx4linux.conf ---\n"
"\n"
"in_device =  /dev/dmxin\n"
"out_device = /dev/dmx\n";
}


/*
 * Called when there is input for us
 */
int Dmx4LinuxPlugin::SocketReady() {
  vector<Dmx4LinuxInputPort*>::iterator iter;
  unsigned int data_read, offset;
  int ret;

  if (lseek(m_in_socket->ReadDescriptor(), 0, SEEK_SET) != 0) {
    OLA_WARN << "failed to seek: " << strerror(errno);
    return -1;
  }
  ret = m_in_socket->Receive(reinterpret_cast<uint8_t*>(m_in_buffer),
                             DMX_UNIVERSE_SIZE * m_in_devices_count,
                             data_read);
  iter = m_in_ports.begin();
  offset = 0;
  while (offset < data_read && iter != m_in_ports.end()) {
    (*iter)->UpdateData(m_in_buffer + offset, data_read - offset);
    iter++;
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

  save |= m_preferences->SetDefaultValue(IN_DEV_KEY, DMX4LINUX_IN_DEVICE);
  save |= m_preferences->SetDefaultValue(OUT_DEV_KEY, DMX4LINUX_OUT_DEVICE);

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
 * Open the input and output fds
 */
bool Dmx4LinuxPlugin::SetupSockets() {
  if (!m_in_socket && !m_out_socket) {
    int fd = open(m_out_dev.c_str(), O_WRONLY);

    if (fd < 0) {
      OLA_WARN << "failed to open " << m_out_dev << " " << strerror(errno);
      return false;
    }
    m_out_socket = new Dmx4LinuxSocket(fd);

    fd = open(m_in_dev.c_str(), O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
      OLA_WARN << "failed to open " << m_in_dev << " " << strerror(errno);
      CleanupSockets();
      return false;
    }
    m_in_socket = new Dmx4LinuxSocket(fd);
    return true;
  }
  return false;
}


/*
 * Close all fds
 */
int Dmx4LinuxPlugin::CleanupSockets() {
  if (m_in_socket) {
    delete m_in_socket;
    m_in_socket = NULL;
  }

  if (m_out_socket) {
    delete m_out_socket;
    m_out_socket = NULL;
  }

  if (m_in_buffer) {
    delete[] m_in_buffer;
    m_in_buffer = NULL;
  }
  return 0;
}


/*
 * setup a single device
 * @param family the dmx4linux family
 * @param d4l_uni the dmx4linux universe
 * @param dir   in|out
 */
bool Dmx4LinuxPlugin::SetupDevice(string family, int d4l_uni, int dir) {
  string device_id = IntToString((d4l_uni << 1) + dir);
  string name = string("dmx4linux_");
  name.append(family);
  name.append(dir ? "_in_" : "_out_");
  name.append(IntToString(d4l_uni));

  OLA_INFO << "Dmx4LinuxPlugin creates a device : name = " << name <<
      " / uni = " << d4l_uni << " / dir = " << dir;
  Dmx4LinuxDevice *dev = new Dmx4LinuxDevice(this, name, device_id);

  if (dir == DMX_DIRECTION_INPUT) {
    Dmx4LinuxInputPort *port = new Dmx4LinuxInputPort(dev);
    m_in_ports.push_back(port);
    dev->AddPort(port);
  } else {
    Dmx4LinuxOutputPort *port = new Dmx4LinuxOutputPort(dev,
                                                        m_out_socket,
                                                        d4l_uni);
    dev->AddPort(port);
  }
  m_plugin_adaptor->RegisterDevice(dev);
  m_devices.push_back(dev);
  return true;
}


/*
 * Find all the devices connected and setup ports for them.
 */
bool Dmx4LinuxPlugin::SetupDevices() {
  struct dmx_capabilities cap;
  struct dmx_info info;

  if (ioctl(m_in_socket->ReadDescriptor(), DMX_IOCTL_GET_INFO, &info) < 0) {
    OLA_WARN << "failed to fetch universe list";
    return false;
  }

  if (info.max_in_universes > 0) {
    m_in_devices_count = info.max_in_universes;
    try {
      m_in_buffer = new uint8_t[DMX_UNIVERSE_SIZE * info.max_in_universes];
    } catch (std::bad_alloc& ex) {
      m_in_buffer = NULL;
    }
  }

  for (int i = 0; i < info.max_in_universes; i++) {
    cap.direction = DMX_DIRECTION_INPUT;
    cap.universe = i;

    if (ioctl(m_in_socket->ReadDescriptor(), DMX_IOCTL_GET_CAP, &cap) >= 0) {
      if (cap.maxSlots > 0) {
        SetupDevice(cap.family, cap.universe, cap.direction);
      }
    }
  }

  for (int i = 0; i < info.max_out_universes; i++) {
    cap.direction = DMX_DIRECTION_OUTPUT;
    cap.universe = i;

    if (ioctl(m_in_socket->ReadDescriptor(), DMX_IOCTL_GET_CAP, &cap) >= 0) {
      if (cap.maxSlots > 0) {
        SetupDevice(cap.family, cap.universe, cap.direction);
      }
    }
  }
  return true;
}
}  // dmx4linux
}  // plugin
}  // ola
