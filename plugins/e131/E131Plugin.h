/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * E131Plugin.h
 * Interface for the E1.131 plugin class
 * Copyright (C) 2007-2009 Simon Newton
 */

#ifndef PLUGINS_E131_E131PLUGIN_H_
#define PLUGINS_E131_E131PLUGIN_H_

#include <string>
#include "olad/Plugin.h"
#include "ola/plugin_id.h"

namespace ola {
namespace plugin {
namespace e131 {

class E131Plugin: public ola::Plugin {
  public:
    explicit E131Plugin(ola::PluginAdaptor *plugin_adaptor):
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
    bool SetDefaultPreferences();

    class E131Device *m_device;
    static const char CID_KEY[];
    static const char DEFAULT_DSCP_VALUE[];
    static const char DEFAULT_PORT_COUNT[];
    static const char DSCP_KEY[];
    static const char IGNORE_PREVIEW_DATA_KEY[];
    static const char INPUT_PORT_COUNT_KEY[];
    static const char IP_KEY[];
    static const char OUTPUT_PORT_COUNT_KEY[];
    static const char PLUGIN_NAME[];
    static const char PLUGIN_PREFIX[];
    static const char PREPEND_HOSTNAME_KEY[];
    static const char REVISION_0_2[];
    static const char REVISION_0_46[];
    static const char REVISION_KEY[];
};
}  // e131
}  // plugin
}  // ola
#endif  // PLUGINS_E131_E131PLUGIN_H_
