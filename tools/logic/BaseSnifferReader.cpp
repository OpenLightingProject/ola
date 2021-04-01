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
 * BaseSnifferReader.cpp
 * Common RDM Sniffer code for logic analyzer based sniffers.
 * Copyright (C) 2021 Peter Newman
 */

#include <ola/base/Flags.h>
#include <ola/Callback.h>
#include <ola/Constants.h>
#include <ola/Clock.h>
#include <ola/io/SelectServer.h>
#include <ola/Logging.h>
#include <ola/rdm/CommandPrinter.h>
#include <ola/rdm/PidStoreHelper.h>
#include <ola/rdm/RDMCommand.h>
#include <ola/rdm/RDMPacket.h>
#include <ola/rdm/RDMResponseCodes.h>
#include <ola/StringUtils.h>

#include <iostream>
#include <memory>

#include "tools/logic/BaseSnifferReader.h"

using std::auto_ptr;
using std::cerr;
using std::cout;
using std::endl;
using ola::io::SelectServer;
using ola::rdm::CommandPrinter;
using ola::rdm::PidStoreHelper;
using ola::rdm::RDMCommand;
using ola::strings::ToHex;

using ola::NewSingleCallback;

DEFINE_default_bool(display_asc, false,
                    "Display non-RDM alternate start code frames.");
DEFINE_s_default_bool(full_rdm, r, false, "Unpack RDM parameter data.");
// TODO(Peter): Implement this!
// DEFINE_s_default_bool(timestamp, t, false, "Include timestamps.");
DEFINE_s_default_bool(display_dmx, d, false,
                      "Display DMX Frames. Defaults to false.");
DEFINE_uint16(dmx_slot_limit, ola::DMX_UNIVERSE_SIZE,
              "Only display the first N slots of DMX data.");
DEFINE_string(pid_location, "",
              "The directory containing the PID definitions.");

BaseSnifferReader::BaseSnifferReader(SelectServer *ss,
                                     unsigned int sample_rate)
  : m_ss(ss),
    m_signal_processor(ola::NewCallback(this, &BaseSnifferReader::FrameReceived),
                       sample_rate),
    m_pid_helper(FLAGS_pid_location.str(), 4),
    m_command_printer(&cout, &m_pid_helper) {
  if (!m_pid_helper.Init()) {
    OLA_WARN << "Failed to init PidStore";
  }
}


BaseSnifferReader::~BaseSnifferReader() {
  m_ss->DrainCallbacks();
}

void BaseSnifferReader::FrameReceived(const uint8_t *data, unsigned int length) {
  if (!length) {
    return;
  }

  switch (data[0]) {
    case ola::DMX512_START_CODE:
      DisplayDMXFrame(data + 1, length - 1);
      break;
    case ola::rdm::START_CODE:
      DisplayRDMFrame(data + 1, length - 1);
      break;
    default:
      DisplayAlternateFrame(data, length);
  }
}

void BaseSnifferReader::DisplayDMXFrame(const uint8_t *data, unsigned int length) {
  if (!FLAGS_display_dmx) {
    return;
  }

  cout << "DMX " << std::dec;
  cout << length << ":" << std::hex;
  DisplayRawData(data, length);
}

void BaseSnifferReader::DisplayRDMFrame(const uint8_t *data, unsigned int length) {
  auto_ptr<RDMCommand> command(RDMCommand::Inflate(data, length));
  if (command.get()) {
    if (FLAGS_full_rdm) {
      cout << "---------------------------------------" << endl;
    }
    command->Print(&m_command_printer, !FLAGS_full_rdm, true);
  } else {
    cout << "RDM " << std::dec;
    cout << length << ":" << std::hex;
    DisplayRawData(data, length);
  }
}


void BaseSnifferReader::DisplayAlternateFrame(const uint8_t *data,
                                        unsigned int length) {
  if (!FLAGS_display_asc || length == 0) {
    return;
  }

  unsigned int slot_count = length - 1;
  cout << "SC " << ToHex(static_cast<int>(data[0]))
       << " " << slot_count << ":";
  DisplayRawData(data + 1, slot_count);
}


/**
 * Dump out the raw data if we couldn't parse it correctly.
 */
void BaseSnifferReader::DisplayRawData(const uint8_t *data, unsigned int length) {
  for (unsigned int i = 0; i < length; i++) {
    cout << std::hex << std::setw(2) << std::setfill('0')
         << static_cast<int>(data[i]) << " ";
  }
  cout << endl;
}
