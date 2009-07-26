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
 * EspNetDevice.h
 * Interface for the EspNet device
 * Copyright (C) 2005-2008 Simon Newton
 */

#ifndef ESPNETDEVICE_H
#define ESPNETDEVICE_H

#include <olad/Device.h>
#include <olad/Plugin.h>

namespace ola {
namespace espnet {

using ola::Plugin;

class EspNetDevice: public ola::Device {
  public:
    EspNetDevice(Plugin *owner,
                 const std::string &name,
                 class Preferences *prefs,
                 const class PluginAdaptor *plugin_adaptor);
    ~EspNetDevice() {}

    bool Start();
    bool Stop();
    string DeviceId() const { return "1"; }
    class EspNetNode *GetNode() const { return m_node; }

  private:
    class Preferences *m_preferences;
    const class PluginAdaptor *m_plugin_adaptor;
    class EspNetNode *m_node;
    bool m_enabled;

    static const std::string IP_KEY;
};

} //espnet
} //ola

#endif
