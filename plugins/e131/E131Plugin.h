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
 * E131Plugin.h
 * Interface for the E1.131 plugin class
 * Copyright (C) 2007-2009 Simon Newton
 */

#ifndef OLA_E131PLUGIN_H
#define OLA_E131PLUGIN_H

#include <string>
#include <olad/plugin.h>
#include <ola/plugin_id.h>

namespace ola {
namespace e131 {

class E131Plugin: public ola::Plugin {
  public:
    E131Plugin(const ola::PluginAdaptor *plugin_adaptor):
      ola::Plugin(plugin_adaptor),
      m_device(NULL) {}
    ~E131Plugin() {}

    string Name() const { return PLUGIN_NAME; }
    ola_plugin_id Id() const { return OLA_PLUGIN_E131; }
    string Description() const;
    string PluginPrefix() const { return PLUGIN_PREFIX; }

  private:
    bool StartHook();
    bool StopHook();

    class E131Device *m_device;
    static const string PLUGIN_NAME;
    static const string PLUGIN_PREFIX;
};

} // e131
} // ola
#endif

