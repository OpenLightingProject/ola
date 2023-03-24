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
 * ola-patcher-remap.cpp
 * A simple tool to remap channels from one universe to another.
 * Copyright (C) 2023 Peter Newman
 */

/*
{
	100 => {
		1 => [1:1, 2:1, 1:3],
		1 => [4:1],
		2 => [5:1, 6:1, 6:2],
	},
	101 => {
		1 => [11:1, 12:1],
		2 => [15:1, 16:1],
	}
}

Remap internally to (sets):
{
	100 => {
		1 => {
			1 => [1, 3],
			2 => [1],
			4 => [1],
		}
		2 => {
			5 => [1],
			6 => [1, 2],
		}
	},
	101 => {
		1 => {
			11 => [1],
			12 => [1],
		}
		2 => {
			15 => [1],
			16 => [1],
		}
	}
}
*/

#include <ola/base/Flags.h>
#include <ola/base/Init.h>
#include <ola/dmx/UniverseChannelAddress.h>
#include <ola/io/SelectServer.h>
#include <ola/stl/STLUtils.h>
#include <ola/Callback.h>
#include <ola/DmxBuffer.h>
#include <ola/Logging.h>
#include <ola/StringUtils.h>
#include <ola/client/ClientWrapper.h>
#include <olad/Preferences.h>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <utility>

using ola::DmxBuffer;
using ola::client::OlaClient;
using ola::client::OlaClientWrapper;
using ola::client::Result;
using ola::StringSplit;
using ola::StringTrim;
using ola::STLFindOrNull;
using ola::STLLookupOrInsertNew;
using ola::dmx::UniverseChannelAddressOneBased;
using ola::io::SelectServer;
using std::string;
using std::vector;

DEFINE_s_string(config, c, "ola-patcher-remap.conf", "The config file to use.");

/*
 * The remap class which responds to data
 */
class DmxRemap {
 public:
  typedef std::set<unsigned int> ChansSet;
  typedef std::map<unsigned int, ChansSet*> OutUniChansMap;
  typedef std::map<unsigned int, OutUniChansMap*>
      InChanOutUniChansMap;
  typedef std::map<unsigned int, InChanOutUniChansMap*>
      InUniInChanOutUniChansMap;

  explicit DmxRemap(
      const DmxRemap::InUniInChanOutUniChansMap universe_remaps)
    : m_universe_remaps(universe_remaps) {
  }

  ~DmxRemap() {
    // TODO(Peter): Delete everything
  }

  bool Init();
  void Run() { m_client.GetSelectServer()->Run(); }
  void NewDmx(const ola::client::DMXMetadata &meta,
              const ola::DmxBuffer &buffer);
  void RegisterComplete(const Result &result);

  InUniInChanOutUniChansMap m_universe_remaps;

 private:
  OlaClientWrapper m_client;
  typedef std::map<unsigned int, ola::DmxBuffer*> OutBufferMap;
  OutBufferMap m_out_universes;
};

bool DmxRemap::Init() {
  /* set up ola connection */
  if (!m_client.Setup()) {
    printf("error: %s", strerror(errno));
    return false;
  }

  InUniInChanOutUniChansMap::iterator in_uni_iter = m_universe_remaps.begin();
  for (; in_uni_iter != m_universe_remaps.end(); ++in_uni_iter) {
    OLA_DEBUG << "Have mapping for input universe " << in_uni_iter->first;

    InChanOutUniChansMap::iterator in_chan_iter = in_uni_iter->second->begin();
    for (; in_chan_iter != in_uni_iter->second->end(); ++in_chan_iter) {
      OLA_DEBUG << "\tHave mapping for input channel " << in_chan_iter->first;

      OutUniChansMap::iterator out_uni_iter = in_chan_iter->second->begin();
      for (; out_uni_iter != in_chan_iter->second->end(); ++out_uni_iter) {
        OLA_DEBUG << "\t\tHave mapping for output universe "
                  << out_uni_iter->first << " to channel(s) "
                  << ola::StringJoin(", ", *(out_uni_iter->second));
        STLLookupOrInsertNew(&m_out_universes, out_uni_iter->first);
      }
    }
  }

  OutBufferMap::iterator out_buffer_iter = m_out_universes.begin();
  for (; out_buffer_iter != m_out_universes.end(); ++out_buffer_iter) {
    OLA_DEBUG << "Blacking out output buffer for universe "
              << out_buffer_iter->first;
    out_buffer_iter->second->Blackout();
  }

  OlaClient *client = m_client.GetClient();
  client->SetDMXCallback(ola::NewCallback(this, &DmxRemap::NewDmx));
  in_uni_iter = m_universe_remaps.begin();
  for (; in_uni_iter != m_universe_remaps.end(); ++in_uni_iter) {
    OLA_DEBUG << "Registering input universe " << in_uni_iter->first;
    client->RegisterUniverse(
        in_uni_iter->first,
        ola::client::REGISTER,
        ola::NewSingleCallback(this, &DmxRemap::RegisterComplete));
  }

  return true;
}

// Called when universe registration completes.
void DmxRemap::RegisterComplete(const ola::client::Result& result) {
  if (!result.Success()) {
    OLA_WARN << "Failed to register universe: " << result.Error();
  }
}

// Called when new DMX data arrives.
void DmxRemap::NewDmx(const ola::client::DMXMetadata &metadata,
                      const ola::DmxBuffer &data) {
  OLA_DEBUG << "Received " << data.Size()
            << " channels for universe " << metadata.universe;

  InChanOutUniChansMap *in_chan_map = STLFindOrNull(m_universe_remaps,
                                                    metadata.universe);
  if (in_chan_map) {
    OLA_DEBUG << "Successfully found mapping for input universe "
              << metadata.universe;

    InChanOutUniChansMap::iterator in_chan_iter = in_chan_map->begin();
    for (; in_chan_iter != in_chan_map->end(); ++in_chan_iter) {
      OLA_DEBUG << "\tApplying mapping for input channel "
                << in_chan_iter->first;

      OutUniChansMap::iterator out_uni_iter = in_chan_iter->second->begin();
      for (; out_uni_iter != in_chan_iter->second->end(); ++out_uni_iter) {
        OLA_DEBUG << "\t\tApplying mapping for output universe "
                  << out_uni_iter->first;
        ChansSet::iterator out_chan_iter = out_uni_iter->second->begin();
        for (; out_chan_iter != out_uni_iter->second->end(); ++out_chan_iter) {
          OLA_DEBUG << "\t\t\tApplying mapping for output channel "
                    << *out_chan_iter;
          DmxBuffer *buffer = STLFindOrNull(m_out_universes,
                                            out_uni_iter->first);
          if (!buffer) {
            OLA_WARN << "Failed to find buffer for universe "
                     << out_uni_iter->first;
            delete buffer;
          }
          buffer->SetChannel(*out_chan_iter, data.Get(in_chan_iter->first));
        }
      }
    }
  } else {
    OLA_WARN << "Couldn't find mapping for input universe "
             << metadata.universe;
  }

  OlaClient *client = m_client.GetClient();
  OutBufferMap::iterator out_iter = m_out_universes.begin();
  for (; out_iter != m_out_universes.end(); ++out_iter) {
    OLA_DEBUG << "Sending universe " << out_iter->first;
    client->SendDMX(out_iter->first,
                    *out_iter->second,
                    ola::client::SendDMXArgs());
  }
}


int main(int argc, char *argv[]) {
  ola::AppInit(&argc, argv, "[options]",
               "Remap DMX512 channel data within OLA.");

  ola::client::OlaClientWrapper wrapper;
  if (!wrapper.Setup()) {
    std::cerr << "Setup failed" << std::endl;
    exit(1);
  }

  ola::FileBackedPreferences *preferences = new ola::FileBackedPreferences(
      "", "patcher-remap", NULL);
  preferences->Clear();
  preferences->LoadFromFile(FLAGS_config.str());

  vector<string> values = preferences->GetMultipleValue("remap-channel");
  OLA_DEBUG << "Got " << values.size() << " remap-channel options";

  DmxRemap::InUniInChanOutUniChansMap universe_remaps;

  vector<string>::iterator pref_iter = values.begin();
  for (; pref_iter != values.end(); ++pref_iter) {
    OLA_DEBUG << "Found base config " << *pref_iter;
    vector<string> tokens;
    StringSplit(*pref_iter, &tokens, "-");

    if (tokens.size() != 2) {
      OLA_WARN << "Skipping config section, incorrect number of tokens: "
               << *pref_iter;
      continue;
    }

    string key = tokens[0];
    string value = tokens[1];
    StringTrim(&key);
    StringTrim(&value);
    OLA_DEBUG << "Got raw remap from " << key << " to " << value;
    UniverseChannelAddressOneBased source;
    UniverseChannelAddressOneBased::FromString(key, &source);
    OLA_DEBUG << "Got remap from " << source.Universe() << "\\"
              << source.Channel();

    DmxRemap::InUniInChanOutUniChansMap::iterator in_uni_map_iter =
        STLLookupOrInsertNew(&universe_remaps, source.Universe());

    DmxRemap::InChanOutUniChansMap::iterator in_chan_map_iter =
        STLLookupOrInsertNew(in_uni_map_iter->second, source.ChannelZeroBased());

    vector<string> value_tokens;
    StringSplit(value, &value_tokens, ",");

    vector<string>::iterator pref_value_iter = value_tokens.begin();
    for (; pref_value_iter != value_tokens.end(); ++pref_value_iter) {
      string value_token = *pref_value_iter;
      StringTrim(&value_token);
      UniverseChannelAddressOneBased destination;
      UniverseChannelAddressOneBased::FromString(value_token, &destination);
      OLA_DEBUG << "\tGot remap to " << destination.Universe() << "\\"
                << destination.Channel();
      DmxRemap::OutUniChansMap::iterator out_uni_map_iter =
          STLLookupOrInsertNew(in_chan_map_iter->second,
                               destination.Universe());
      out_uni_map_iter->second->insert(destination.ChannelZeroBased());
    }
  }

  delete preferences;

  DmxRemap *dmx_remap = new DmxRemap(universe_remaps);
  if (!dmx_remap->Init()) {
    std::cerr << "Failed to initialise DmxRemap" << std::endl;
    return 1;
  }

  dmx_remap->Run();
  delete dmx_remap;
  dmx_remap = NULL;
  return 0;
}
