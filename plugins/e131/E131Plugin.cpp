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
 * E131Plugin.cpp
 * The E1.31 plugin for ola
 * Copyright (C) 2007-2009 Simon Newton
 */

#include "plugins/e131/e131/E131Includes.h"  //  NOLINT, this has to be first
#include <stdlib.h>
#include <stdio.h>
#include <set>
#include <string>

#include "ola/Logging.h"
#include "ola/StringUtils.h"
#include "olad/PluginAdaptor.h"
#include "olad/Preferences.h"
#include "plugins/e131/E131Device.h"
#include "plugins/e131/E131Plugin.h"
#include "plugins/e131/e131/CID.h"

/*
 * Entry point to this plugin
 */
extern "C" ola::AbstractPlugin* create(const ola::PluginAdaptor *adaptor) {
  return new ola::plugin::e131::E131Plugin(adaptor);
}

namespace ola {
namespace plugin {
namespace e131 {

const char E131Plugin::CID_KEY[] = "cid";
const char E131Plugin::DSCP_KEY[] = "dscp";
const char E131Plugin::IGNORE_PREVIEW_DATA_KEY[] = "ignore_preview";
const char E131Plugin::IP_KEY[] = "ip";
const char E131Plugin::PLUGIN_NAME[] = "E1.31 (sACN)";
const char E131Plugin::PLUGIN_PREFIX[] = "e131";
const char E131Plugin::PREPEND_HOSTNAME_KEY[] = "prepend_hostname";
const char E131Plugin::REVISION_0_2[] = "0.2";
const char E131Plugin::REVISION_0_46[] = "0.46";
const char E131Plugin::REVISION_KEY[] = "revision";
const char E131Plugin::DEFAULT_DSCP_VALUE[] = "0";


/*
 * Start the plugin
 */
bool E131Plugin::StartHook() {
  CID cid = CID::FromString(m_preferences->GetValue(CID_KEY));
  string ip_addr = m_preferences->GetValue(IP_KEY);
  string revision = m_preferences->GetValue(REVISION_KEY);
  bool use_rev2 = revision == REVISION_0_2 ? true : false;
  bool prepend_hostname = m_preferences->GetValueAsBool(PREPEND_HOSTNAME_KEY);
  bool ignore_preview = m_preferences->GetValueAsBool(IGNORE_PREVIEW_DATA_KEY);
  unsigned int dscp;
  if (!StringToUInt(m_preferences->GetValue(DSCP_KEY), &dscp)) {
    OLA_WARN << "Can't convert dscp value " <<
      m_preferences->GetValue(DSCP_KEY) << " to int";
    dscp = 0;
  } else {
    // shift 2 bits left
    dscp = dscp << 2;
  }

  m_device = new E131Device(this,
                            cid,
                            ip_addr,
                            m_plugin_adaptor,
                            use_rev2,
                            prepend_hostname,
                            ignore_preview,
                            dscp);

  if (!m_device->Start()) {
    delete m_device;
    return false;
  }

  m_plugin_adaptor->RegisterDevice(m_device);
  return true;
}


/*
 * Stop the plugin
 * @return true on success, false on failure
 */
bool E131Plugin::StopHook() {
  if (m_device) {
    m_plugin_adaptor->UnregisterDevice(m_device);
    bool ret = m_device->Stop();
    delete m_device;
    return ret;
  }
  return true;
}


/*
 * Return the description for this plugin
 */
string E131Plugin::Description() const {
    return
"E1.31 (Streaming DMX over ACN) Plugin\n"
"----------------------------\n"
"\n"
"This plugin creates a single device with eight input and eight output ports.\n"
"\n"
"Each port can be assigned to a diffent E1.31 Universe.\n"
"\n"
"--- Config file : ola-e131.conf ---\n"
"\n"
"cid = 00010203-0405-0607-0809-0A0B0C0D0E0F\n"
"The CID to use for this device\n"
"\n"
"dscp = [int]\n"
"The DSCP value to tag the packets with, range is 0 to 63.\n"
"\n"
"ignore_preview = [true|false]\n"
"Ignore preview data.\n"
"\n"
"ip = [a.b.c.d|<interface_name>]\n"
"The ip address or interface name to bind to. If not specified it will\n"
"use the first non-loopback interface.\n"
"\n"
"prepend_hostname = [true|false]\n"
"Prepend the hostname to the source name when sending packets.\n"
"\n"
"revision = [0.2|0.46]\n"
"Select which revision of the standard to use when sending data. 0.2 is the\n"
" standardized revision, 0.46 (default) is the ANSI standard version.\n"
"\n";
}


/*
 * Load the plugin prefs and default to sensible values
 *
 */
bool E131Plugin::SetDefaultPreferences() {
  if (!m_preferences)
    return false;

  bool save = false;

  CID cid = CID::FromString(m_preferences->GetValue(CID_KEY));
  if (cid.IsNil()) {
    cid = CID::Generate();
    m_preferences->SetValue(CID_KEY, cid.ToString());
    save = true;
  }

  save |= m_preferences->SetDefaultValue(
      DSCP_KEY,
      IntValidator(0, 63),
      DEFAULT_DSCP_VALUE);

  save |= m_preferences->SetDefaultValue(
      IGNORE_PREVIEW_DATA_KEY,
      BoolValidator(),
      BoolValidator::TRUE);

  save |= m_preferences->SetDefaultValue(IP_KEY, IPv4Validator(), "");

  save |= m_preferences->SetDefaultValue(
      PREPEND_HOSTNAME_KEY,
      BoolValidator(),
      BoolValidator::TRUE);

  set<string> revision_values;
  revision_values.insert(REVISION_0_2);
  revision_values.insert(REVISION_0_46);

  save |= m_preferences->SetDefaultValue(
      REVISION_KEY,
      SetValidator(revision_values),
      REVISION_0_46);

  if (save)
    m_preferences->Save();

  // check if this saved correctly
  // we don't want to use it if null
  string revision = m_preferences->GetValue(REVISION_KEY);
  if (m_preferences->GetValue(CID_KEY).empty() ||
      (revision != REVISION_0_2 && revision != REVISION_0_46))
    return false;

  return true;
}
}  // e131
}  // plugin
}  // ola
