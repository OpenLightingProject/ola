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
 * plugin.cpp
 * Base plugin class for lla
 * Copyright (C) 2005-2007 Simon Newton
 */

#include <llad/plugin.h>
#include <llad/preferences.h>
#include <llad/logger.h>

const string Plugin::ENABLED_KEY = "enabled";
const string Plugin::DEBUG_KEY = "debug";

/*
 * Start the plugin. Calls start_hook() which can be over-ridden by the
 * derrived classes.
 */
int Plugin::start() {
  string enabled, debug;

  if (m_enabled)
    return -1;

  // setup prefs
  if (load_prefs())
    return -1;

  enabled = m_prefs->get_val(ENABLED_KEY);
  if(enabled == "false") {
    Logger::instance()->log(Logger::INFO, "Plugin: %s disabled", get_name().c_str());
    delete m_prefs;
    return 0;
  }

  debug = m_prefs->get_val(DEBUG_KEY);
  if(debug == "true") {
    Logger::instance()->log(Logger::INFO, "Plugin: %s debug on", get_name().c_str());
    m_debug = true;
  }

  if(start_hook()) {
    delete m_prefs;
    return -1;
  }

  m_enabled = true;
  return 0;
}


/*
 * Stop the plugin. Called stop_hook which can be over-ridden by the
 * derrived classes.
 */
int Plugin::stop() {

 if (!m_enabled)
     return -1;

  stop_hook();

  m_enabled = false;
  delete m_prefs;
  return 0;
}


/*
 * Load the preferences and set defaults
 */
int Plugin::load_prefs() {
  if (pref_suffix() == "") {
    Logger::instance()->log(Logger::WARN, "Plugin: no suffix provided");
    return -1;
  }

  if (m_prefs != NULL)
    delete m_prefs;

  m_prefs = new Preferences(pref_suffix());

  if (m_prefs == NULL)
    return -1;

  m_prefs->load();

  if (set_default_prefs()) {
    delete m_prefs;
    Logger::instance()->log(Logger::INFO, "%s:set_default_prefs failed", get_name().c_str());
    return -1;
  }

  return 0;
}
