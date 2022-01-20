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
 * E131Plugin.cpp
 * The E1.31 plugin for ola
 * Copyright (C) 2007 Simon Newton
 */

#include <set>
#include <string>

#include "ola/Logging.h"
#include "ola/network/NetworkUtils.h"
#include "ola/StringUtils.h"
#include "ola/acn/CID.h"
#include "olad/PluginAdaptor.h"
#include "olad/Preferences.h"
#include "plugins/e131/E131Device.h"
#include "plugins/e131/E131Plugin.h"
#include "plugins/e131/E131PluginDescription.h"

namespace ola {
namespace plugin {
namespace e131 {

using ola::acn::CID;
using std::string;

const char E131Plugin::CID_KEY[] = "cid";
const unsigned int E131Plugin::DEFAULT_DSCP_VALUE = 0;
const char E131Plugin::DSCP_KEY[] = "dscp";
const char E131Plugin::DRAFT_DISCOVERY_KEY[] = "draft_discovery";
const char E131Plugin::IGNORE_PREVIEW_DATA_KEY[] = "ignore_preview";
const char E131Plugin::INPUT_PORT_COUNT_KEY[] = "input_ports";
const char E131Plugin::IP_KEY[] = "ip";
const char E131Plugin::OUTPUT_PORT_COUNT_KEY[] = "output_ports";
const char E131Plugin::PLUGIN_NAME[] = "E1.31 (sACN)";
const char E131Plugin::PLUGIN_PREFIX[] = "e131";
const char E131Plugin::PREPEND_HOSTNAME_KEY[] = "prepend_hostname";
const char E131Plugin::REVISION_0_2[] = "0.2";
const char E131Plugin::REVISION_0_46[] = "0.46";
const char E131Plugin::REVISION_KEY[] = "revision";
const unsigned int E131Plugin::DEFAULT_PORT_COUNT = 5;


/*
 * Start the plugin
 */
bool E131Plugin::StartHook() {
  CID cid = CID::FromString(m_preferences->GetValue(CID_KEY));
  string ip_addr = m_preferences->GetValue(IP_KEY);

  E131Device::E131DeviceOptions options;
  options.use_rev2 = (m_preferences->GetValue(REVISION_KEY) == REVISION_0_2);
  options.ignore_preview = m_preferences->GetValueAsBool(
      IGNORE_PREVIEW_DATA_KEY);
  options.enable_draft_discovery = m_preferences->GetValueAsBool(
      DRAFT_DISCOVERY_KEY);
  if (m_preferences->GetValueAsBool(PREPEND_HOSTNAME_KEY)) {
    std::ostringstream str;
    str << ola::network::Hostname() << "-" << m_plugin_adaptor->InstanceName();
    options.source_name = str.str();
  } else {
    options.source_name = m_plugin_adaptor->InstanceName();
  }

  unsigned int dscp;
  if (!StringToInt(m_preferences->GetValue(DSCP_KEY), &dscp)) {
    OLA_WARN << "Can't convert dscp value " <<
      m_preferences->GetValue(DSCP_KEY) << " to int";
    options.dscp = 0;
  } else {
    // shift 2 bits left
    options.dscp = dscp << 2;
  }

  if (!StringToInt(m_preferences->GetValue(INPUT_PORT_COUNT_KEY),
                   &options.input_ports)) {
    OLA_WARN << "Invalid value for input_ports";
  }

  if (!StringToInt(m_preferences->GetValue(OUTPUT_PORT_COUNT_KEY),
                   &options.output_ports)) {
    OLA_WARN << "Invalid value for output_ports";
  }

  m_device = new E131Device(this, cid, ip_addr, m_plugin_adaptor, options);

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
    return plugin_description;
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
      UIntValidator(0, 63),
      DEFAULT_DSCP_VALUE);

  save |= m_preferences->SetDefaultValue(
      DRAFT_DISCOVERY_KEY,
      BoolValidator(),
      false);

  save |= m_preferences->SetDefaultValue(
      IGNORE_PREVIEW_DATA_KEY,
      BoolValidator(),
      true);

  save |= m_preferences->SetDefaultValue(
      INPUT_PORT_COUNT_KEY,
      UIntValidator(0, 512),
      DEFAULT_PORT_COUNT);

  save |= m_preferences->SetDefaultValue(
      OUTPUT_PORT_COUNT_KEY,
      UIntValidator(0, 512),
      DEFAULT_PORT_COUNT);

  save |= m_preferences->SetDefaultValue(IP_KEY, StringValidator(true), "");

  save |= m_preferences->SetDefaultValue(
      PREPEND_HOSTNAME_KEY,
      BoolValidator(),
      true);

  std::set<string> revision_values;
  revision_values.insert(REVISION_0_2);
  revision_values.insert(REVISION_0_46);

  save |= m_preferences->SetDefaultValue(
      REVISION_KEY,
      SetValidator<string>(revision_values),
      REVISION_0_46);

  if (save) {
    m_preferences->Save();
  }

  // check if this saved correctly
  // we don't want to use it if null
  string revision = m_preferences->GetValue(REVISION_KEY);
  if (m_preferences->GetValue(CID_KEY).empty() ||
      (revision != REVISION_0_2 && revision != REVISION_0_46)) {
    return false;
  }

  return true;
}
}  // namespace e131
}  // namespace plugin
}  // namespace ola
