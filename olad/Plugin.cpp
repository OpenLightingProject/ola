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
 * Plugin.cpp
 * Base plugin class for ola
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <string>
#include "ola/Logging.h"
#include "olad/Plugin.h"
#include "olad/PluginAdaptor.h"
#include "olad/Preferences.h"

namespace ola {

const char Plugin::ENABLED_KEY[] = "enabled";
const char Plugin::DEBUG_KEY[] = "debug";

/*
 * Start the plugin. Calls start_hook() which can be over-ridden by the
 * derrived classes.
 *
 * @returns true if started sucessfully, false otherwise.
 */
bool Plugin::Start() {
  string enabled, debug;

  if (m_enabled)
    return false;

  // setup prefs
  if (!LoadPreferences())
    return false;

  enabled = m_preferences->GetValue(ENABLED_KEY);
  if (enabled == "false") {
    OLA_INFO << Name() << " disabled";
    return false;
  }

  debug = m_preferences->GetValue(DEBUG_KEY);
  if (debug == "true") {
    OLA_INFO << Name() << " debug on";
    m_debug = true;
  }

  if (!StartHook()) {
    return false;
  }

  m_enabled = true;
  return true;
}


/*
 * Stop the plugin. Calls stop_hook which can be over-ridden by the
 * derrived classes.
 *
 * @returns true if stopped sucessfully, false otherwise.
 */
bool Plugin::Stop() {
  if (!m_enabled)
    return false;

  bool ret = StopHook();

  m_enabled = false;
  return ret;
}


/*
 * Load the preferences and set defaults
 */
bool Plugin::LoadPreferences() {
  if (PluginPrefix() == "") {
    OLA_WARN << Name() << ", no prefix provided";
    return false;
  }

  m_preferences = m_plugin_adaptor->NewPreference(PluginPrefix());

  if (!m_preferences)
    return false;

  m_preferences->Load();

  if (!SetDefaultPreferences()) {
    OLA_INFO << Name() << ", SetDefaultPreferences failed";
    return false;
  }

  return true;
}
}  // ola
