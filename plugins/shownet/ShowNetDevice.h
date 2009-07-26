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
 * ShowNetDevice.h
 * Interface for the ShowNet device
 * Copyright (C) 2005-2009 Simon Newton
 */

#ifndef SHOWNETDEVICE_H
#define SHOWNETDEVICE_H

#include <llad/Device.h>
#include <llad/Plugin.h>

namespace lla {
namespace shownet {

using lla::Plugin;

class ShowNetDevice: public lla::Device {
  public:
    ShowNetDevice(Plugin *owner, const string &name,
                  class Preferences *preferences,
                  const class PluginAdaptor *plugin_adaptor);
    ~ShowNetDevice() {}

    bool Start();
    bool Stop();
    string DeviceId() const { return "1"; }
    class ShowNetNode *GetNode() const { return m_node; }

  private:
    class Preferences *m_preferences;
    const class PluginAdaptor *m_plugin_adaptor;
    class ShowNetNode *m_node;
    bool m_enabled;

    static const std::string IP_KEY;
};

} //plugin
} //lla
#endif
