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
 * rdm-sniffer.cpp
 * RDM Sniffer software for the ENTTEC RDM Pro.
 * Copyright (C) 2010 Simon Newton
 */

#include <string.h>
#include <time.h>

#include <ola/Callback.h>
#include <ola/Clock.h>
#include <ola/Constants.h>
#include <ola/Logging.h>
#include <ola/StringUtils.h>
#include <ola/base/Flags.h>
#include <ola/base/Init.h>
#include <ola/base/SysExits.h>
#include <ola/base/Macro.h>
#include <ola/io/SelectServer.h>
#include <ola/network/NetworkUtils.h>
#include <ola/rdm/CommandPrinter.h>
#include <ola/rdm/PidStoreHelper.h>
#include <ola/rdm/RDMCommand.h>
#include <ola/rdm/RDMEnums.h>
#include <ola/rdm/RDMHelper.h>
#include <ola/rdm/RDMPacket.h>
#include <ola/rdm/RDMResponseCodes.h>
#include <ola/rdm/UID.h>
#include <ola/strings/Format.h>

#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "plugins/usbpro/BaseUsbProWidget.h"

using std::auto_ptr;
using std::cerr;
using std::cout;
using std::endl;
using std::string;
using std::vector;
using ola::strings::ToHex;
using ola::io::SelectServerInterface;
using ola::plugin::usbpro::DispatchingUsbProWidget;
using ola::messaging::Descriptor;
using ola::messaging::Message;
using ola::rdm::CommandPrinter;
using ola::rdm::PidStoreHelper;
using ola::rdm::RDMCommand;
using ola::rdm::UID;

DEFINE_s_default_bool(display_dmx, d, false,
                      "Display DMX frames. Defaults to false.");
DEFINE_s_default_bool(timestamp, t, false, "Include timestamps.");
DEFINE_s_default_bool(full_rdm, r, false, "Unpack RDM parameter data.");
DEFINE_s_string(readfile, p, "",
                "Display data from a previously captured file.");
DEFINE_s_string(savefile, w, "",
                "Also write the captured data to a file.");
DEFINE_default_bool(display_asc, false,
                    "Display non-RDM alternate start code frames.");
DEFINE_int16(dmx_slot_limit, ola::DMX_UNIVERSE_SIZE,
             "Only display the first N slots of DMX data.");
DEFINE_string(pid_location, "",
              "The directory containing the PID definitions.");

/**
 * A list of bytes
 */
class ByteStream {
 public:
    ByteStream() {}
    virtual ~ByteStream() {}

    void AddByte(uint8_t byte) { m_bytes.push_back(byte); }
    uint8_t& operator[](unsigned int offset) { return m_bytes[offset]; }
    void Reset() { m_bytes.clear(); }
    unsigned int Size() { return m_bytes.size(); }

 private:
    vector<uint8_t> m_bytes;
};


class RDMSniffer {
 public:
    struct RDMSnifferOptions {
      // DMX Options
      bool display_dmx_frames;  // display DMX frames
      // limit the number of dmx slots displayed
      uint16_t dmx_slot_limit;

      // RDM Options
      bool display_rdm_frames;  // display RDM frames
      bool summarize_rdm_frames;  // only display a one line summary
      bool unpack_param_data;  // display the unpacked RDM param data

      // Non DMX / RDM Frames
      // display non-RDM alternate start code frames
      bool display_non_rdm_asc_frames;

      // the location with the pid data
      string pid_location;

      string write_file;  // write to this file if set

      // print timestamps as well, these aren't saved
      bool timestamp;
    };

    static void InitOptions(RDMSnifferOptions *options) {
      options->display_dmx_frames = false;
      options->dmx_slot_limit = ola::DMX_UNIVERSE_SIZE;
      options->display_rdm_frames = true;
      options->summarize_rdm_frames = true;
      options->unpack_param_data = true;
      options->display_non_rdm_asc_frames = true;
      options->pid_location = "";
      options->write_file = "";
      options->timestamp = false;
    }

    explicit RDMSniffer(const RDMSnifferOptions &options);

    void HandleMessage(uint8_t label,
                       const uint8_t *data,
                       unsigned int length);

 private:
    typedef enum {
      IDLE,
      BREAK,
      MAB,
      DATA,
    } SnifferState;

    SnifferState m_state;
    ByteStream m_frame;
    RDMSnifferOptions m_options;
    PidStoreHelper m_pid_helper;
    CommandPrinter m_command_printer;

    void ProcessTuple(uint8_t control_byte, uint8_t data_byte);
    void ProcessFrame();

    void DisplayDmxFrame();
    void DisplayAlternateFrame();
    void DisplayRDMFrame();
    void DisplayRawData(unsigned int start, unsigned int end);
    void MaybePrintTimestamp();

    static const uint8_t SNIFFER_PACKET = 0x81;
    static const uint8_t SNIFFER_PACKET_SIZE = 200;
    // if the high bit is set, this is a data byte, otherwise it's a control
    // byte
    static const uint8_t DATA_MASK = 0x80;
    static const uint8_t BYTES_PER_LINE = 8;
};


RDMSniffer::RDMSniffer(const RDMSnifferOptions &options)
    : m_state(IDLE),
      m_options(options),
      m_pid_helper(options.pid_location, 4),
      m_command_printer(&cout, &m_pid_helper) {
  if (!m_pid_helper.Init()) {
    OLA_WARN << "Failed to init PidStore";
  }
}


/*
 * Handle the widget replies
 */
void RDMSniffer::HandleMessage(uint8_t label,
                               const uint8_t *data,
                               unsigned int length) {
  if (!m_options.write_file.empty()) {
    uint16_t write_length = ola::network::HostToNetwork(
        static_cast<uint16_t>(length));

    std::ofstream write_file;
    write_file.open(m_options.write_file.c_str(),
                    std::ios::out | std::ios::binary | std::ios::app);
    write_file.write(reinterpret_cast<char*>(&label), sizeof(label));
    write_file.write(reinterpret_cast<char*>(&write_length),
                     sizeof(write_length));
    write_file.write(reinterpret_cast<const char*>(data), length);
    write_file.close();
  }

  if (label != SNIFFER_PACKET) {
    OLA_WARN << "Not a SNIFFER_PACKET, was " << static_cast<int>(label);
    return;
  }

  if (length != SNIFFER_PACKET_SIZE) {
    OLA_WARN << "Strange packet size. Was " <<
      static_cast<unsigned int>(length) << ", expected " <<
      static_cast<unsigned int>(SNIFFER_PACKET_SIZE);
    return;
  }

  for (unsigned int i = 0; i < length; i+= 2) {
    /*
    OLA_INFO << m_state << " " << std::hex << (data[i] & DATA_MASK ? "data  " : "control") << " " << (int)
      (data[i]) << " " << (int) data[i+1];
    */
    ProcessTuple(data[i], data[i + 1]);
  }
}


/**
 * This processes each (control, data) tuple according to the state machine
 */
void RDMSniffer::ProcessTuple(uint8_t control_byte, uint8_t data_byte) {
  if (control_byte & DATA_MASK) {
    // this is an actual byte of data
    switch (m_state) {
      case IDLE:
        // fall through
        OLA_FALLTHROUGH
      case MAB:
        m_state = DATA;
        m_frame.Reset();
        // fall through
        OLA_FALLTHROUGH
      case DATA:
        m_frame.AddByte(data_byte);
        break;
      default:
        OLA_WARN << "Unknown transition from state " << m_state
                 << ", with data " << ToHex(control_byte) << " "
                 << ToHex(data_byte);
    }
  } else {
    // control byte
    if (data_byte == 0) {
      switch (m_state) {
        case BREAK:
          m_state = MAB;
          break;
        default:
          OLA_WARN << "Unknown transition from state " << m_state
                   << ", with data " << ToHex(control_byte) << " "
                   << ToHex(data_byte);
      }
    } else if (data_byte == 1) {
      switch (m_state) {
        case IDLE:
          m_state = BREAK;
          break;
        case DATA:
          ProcessFrame();
          m_state = BREAK;
          break;
        default:
          OLA_WARN << "Unknown transition from state " << m_state
                   << ", with data " << ToHex(control_byte) << " "
                   << ToHex(data_byte);
      }
    } else if (data_byte == 2) {
      switch (m_state) {
        case IDLE:
        case BREAK:
        case MAB:
          break;
        case DATA:
          m_state = IDLE;
          ProcessFrame();
      }
    } else {
      OLA_WARN << "Unknown transition from state " << m_state
               << ", with data " << ToHex(control_byte) << " "
               << ToHex(data_byte);
    }
  }
}


/**
 * Process a frame based on what start code it has.
 */
void RDMSniffer::ProcessFrame() {
  switch (m_frame[0]) {
    case ola::DMX512_START_CODE:
      if (m_options.display_dmx_frames) {
        DisplayDmxFrame();
      }
      break;
    case ola::rdm::START_CODE:
      DisplayRDMFrame();
      break;
    default:
      if (m_options.display_non_rdm_asc_frames) {
        DisplayAlternateFrame();
      }
  }
}


/**
 * Display a DMX Frame
 */
void RDMSniffer::DisplayDmxFrame() {
  unsigned int dmx_slot_count = m_frame.Size() - 1;
  MaybePrintTimestamp();
  cout << "DMX " << std::dec;
  if (m_options.dmx_slot_limit < dmx_slot_count) {
    cout << m_options.dmx_slot_limit << "/";
  }
  cout << dmx_slot_count << ":";
  unsigned int slots_to_display = std::min(
      dmx_slot_count,
      static_cast<unsigned int>(m_options.dmx_slot_limit));

  DisplayRawData(1, slots_to_display);
}


/**
 * Display a non (DMX/RDM) frame
 */
void RDMSniffer::DisplayAlternateFrame() {
  unsigned int slot_count = m_frame.Size() - 1;
  MaybePrintTimestamp();
  cout << "SC " << ToHex(m_frame[0]) << " " << slot_count << ":";
  unsigned int slots_to_display = std::min(
      slot_count,
      static_cast<unsigned int>(m_options.dmx_slot_limit));
  DisplayRawData(1, slots_to_display);
}


/**
 * Display an RDM Frame.
 */
void RDMSniffer::DisplayRDMFrame() {
  unsigned int slot_count = m_frame.Size() - 1;

  auto_ptr<RDMCommand> command(
      RDMCommand::Inflate(reinterpret_cast<const uint8_t*>(&m_frame[1]),
                          slot_count));
  if (command.get()) {
    if (!m_options.summarize_rdm_frames) {
      cout << "---------------------------------------" << endl;
    }

    if (!m_options.summarize_rdm_frames && m_options.timestamp) {
      cout << endl;
    }

    MaybePrintTimestamp();

    command->Print(&m_command_printer, m_options.summarize_rdm_frames,
                   m_options.unpack_param_data);
  } else {
    MaybePrintTimestamp();
    DisplayRawData(1, slot_count);
  }
}


/**
 * Dump out the raw data if we couldn't parse it correctly.
 */
void RDMSniffer::DisplayRawData(unsigned int start, unsigned int end) {
  for (unsigned int i = start; i <= end; i++) {
    cout << ToHex(m_frame[i], false) << " ";
  }
  cout << endl;
}


/**
 * Print the timestamp if timestamps are enabled
 */
void RDMSniffer::MaybePrintTimestamp() {
  if (!m_options.timestamp) {
    return;
  }

  ola::TimeStamp now;
  ola::Clock clock;
  clock.CurrentRealTime(&now);
  time_t seconds_since_epoch = now.Seconds();
  struct tm local_time;
  localtime_r(&seconds_since_epoch, &local_time);

  char output[24];
  strftime(output, sizeof(output), "%d-%m-%Y %H:%M:%S", &local_time);
  cout << output << "." << std::dec << static_cast<int>(now.MicroSeconds()) <<
    " ";
}

void Stop(ola::io::SelectServer *ss) {
  ss->Terminate();
}


/**
 * Interpret data from a save file
 */
void ParseFile(RDMSniffer::RDMSnifferOptions *sniffer_options,
               const string &filename) {
  uint16_t length;
  uint32_t size;
  uint8_t label;
  std::ifstream read_file;

  // turn off timestamps
  sniffer_options->timestamp = false;
  RDMSniffer sniffer(*sniffer_options);

  read_file.open(filename.c_str(), std::ios::in | std::ios::binary);
  if (!read_file.is_open()) {
    OLA_WARN << "Could not open file: " << filename;
    return;
  }

  read_file.seekg(0, std::ios::end);
  size = read_file.tellg();
  read_file.seekg(0, std::ios::beg);

  while (read_file.tellg() < size) {
    read_file.read(reinterpret_cast<char*>(&label), sizeof(label));
    read_file.read(reinterpret_cast<char*>(&length), sizeof(length));
    length = ola::network::NetworkToHost(length);
    uint8_t *buffer = new uint8_t[length];
    read_file.read(reinterpret_cast<char*>(buffer), length);
    sniffer.HandleMessage(label, buffer, length);
    delete[] buffer;
  }
  read_file.close();
}


/*
 * Dump RDM data
 */
int main(int argc, char *argv[]) {
  ola::AppInit(&argc, argv, "[ options ] <usb-device-path>",
               "Sniff traffic from a ENTTEC RDM Pro device.");

  if (!FLAGS_savefile.str().empty() && !FLAGS_readfile.str().empty()) {
    ola::DisplayUsageAndExit();
  }

  RDMSniffer::RDMSnifferOptions sniffer_options;
  RDMSniffer::InitOptions(&sniffer_options);
  sniffer_options.display_non_rdm_asc_frames = FLAGS_display_asc;
  sniffer_options.display_dmx_frames = FLAGS_display_dmx;
  sniffer_options.dmx_slot_limit = FLAGS_dmx_slot_limit;
  sniffer_options.timestamp = FLAGS_timestamp;
  sniffer_options.summarize_rdm_frames = !FLAGS_full_rdm;
  sniffer_options.pid_location = FLAGS_pid_location.str();
  sniffer_options.write_file = FLAGS_savefile.str();

  // if we're writing to a file
  if (!sniffer_options.write_file.empty()) {
    std::ofstream file;
    file.open(sniffer_options.write_file.c_str(),
              std::ios::out | std::ios::binary);
    if (!file.is_open()) {
      cerr << "Could not open file for writing: " << sniffer_options.write_file
           << endl;
      exit(ola::EXIT_UNAVAILABLE);
    }
  }

  if (!FLAGS_readfile.str().empty()) {
    // we're reading from a file
    ParseFile(&sniffer_options, FLAGS_readfile.str());
    return ola::EXIT_OK;
  }

  if (argc != 2) {
    ola::DisplayUsageAndExit();
  }

  const string device = argv[1];

  ola::io::ConnectedDescriptor *descriptor =
      ola::plugin::usbpro::BaseUsbProWidget::OpenDevice(device);
  if (!descriptor) {
    exit(ola::EXIT_UNAVAILABLE);
  }

  ola::io::SelectServer ss;
  descriptor->SetOnClose(ola::NewSingleCallback(&Stop, &ss));
  ss.AddReadDescriptor(descriptor);

  RDMSniffer sniffer(sniffer_options);
  DispatchingUsbProWidget widget(
      descriptor,
      ola::NewCallback(&sniffer, &RDMSniffer::HandleMessage));

  ss.Run();

  return ola::EXIT_OK;
}
