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
 * DummyPlugin.h
 * Interface for the dummyplugin class
 * Copyright (C) 2005-2007 Simon Newton
 */

#ifndef DUMMYPLUGIN_H
#define DUMMYPLUGIN_H

#include <llad/Plugin.h>
#include <lla/plugin_id.h>

namespace lla {
namespace plugin {

using lla::Plugin;
using lla::PluginAdaptor;

class DummyDevice;

class DummyPlugin: public Plugin {
  public:
    DummyPlugin(const PluginAdaptor *plugin_adaptor):
      Plugin(plugin_adaptor),
      m_device(NULL) {}

    string Name() const { return PLUGIN_NAME; }
    string Description() const;
    lla_plugin_id Id() const { return LLA_PLUGIN_DUMMY; }

  protected:
    string PreferencesSuffix() const { return PLUGIN_PREFIX; }

  private:
    bool StartHook();
    bool StopHook();
    bool SetDefaultPreferences() { return true; }

    DummyDevice *m_device; // the dummy device
    static const string PLUGIN_NAME;
    static const string PLUGIN_PREFIX;
    static const string DEVICE_NAME;
};

} // plugin
} // lla

#endif
