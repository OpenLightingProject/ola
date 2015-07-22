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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Plugin.cpp
 * Base plugin class for ola
 * Copyright (C) 2005 Simon Newton
 */

#include <string>
#include "ola/Logging.h"
#include "olad/Plugin.h"
#include "olad/PluginAdaptor.h"
#include "olad/Preferences.h"

namespace ola {

using std::string;

const char Plugin::ENABLED_KEY[] = "enabled";

bool Plugin::LoadPreferences() {
  if (m_preferences) {
    return true;
  }

  if (PluginPrefix() == "") {
    OLA_WARN << Name() << ", no prefix provided";
    return false;
  }

  m_preferences = m_plugin_adaptor->NewPreference(PluginPrefix());

  if (!m_preferences) {
    return false;
  }

  m_preferences->Load();

  bool save = m_preferences->SetDefaultValue(
      ENABLED_KEY,
      BoolValidator(),
      DefaultMode());
  if (save) {
    m_preferences->Save();
  }

  if (!SetDefaultPreferences()) {
    OLA_INFO << Name() << ", SetDefaultPreferences failed";
    return false;
  }
  return true;
}

string Plugin::PreferenceConfigLocation() const {
  return m_preferences->ConfigLocation();
}

bool Plugin::IsEnabled() const {
  return m_preferences->GetValueAsBool(ENABLED_KEY);
}

void Plugin::SetEnabledState(bool enable) {
  m_preferences->SetValueAsBool(ENABLED_KEY, enable);
  m_preferences->Save();
}

bool Plugin::Start() {
  string enabled;

  if (m_enabled) {
    return false;
  }

  // setup prefs
  if (!LoadPreferences()) {
    return false;
  }

  if (!StartHook()) {
    return false;
  }

  m_enabled = true;
  return true;
}

bool Plugin::Stop() {
  if (!m_enabled) {
    return false;
  }

  bool ret = StopHook();

  m_enabled = false;
  return ret;
}
}  // namespace ola
