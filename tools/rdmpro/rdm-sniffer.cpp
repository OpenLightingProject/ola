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
 * rdm-sniffer.cpp
 * RDM Sniffer software for the Enttec RDM Pro.
 * Copyright (C) 2010 Simon Newton
 */

#include <getopt.h>
#include <string.h>
#include <sysexits.h>
#include <time.h>

#include <ola/BaseTypes.h>
#include <ola/Callback.h>
#include <ola/Clock.h>
#include <ola/Logging.h>
#include <ola/io/SelectServer.h>
#include <ola/network/NetworkUtils.h>
#include <ola/rdm/CommandPrinter.h>
#include <ola/rdm/PidStoreHelper.h>
#include <ola/rdm/RDMCommand.h>
#include <ola/rdm/RDMEnums.h>
#include <ola/rdm/RDMHelper.h>
#include <ola/rdm/RDMResponseCodes.h>
#include <ola/rdm/UID.h>

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
using std::stringstream;
using std::vector;
using ola::io::SelectServerInterface;
using ola::plugin::usbpro::DispatchingUsbProWidget;
using ola::messaging::Descriptor;
using ola::messaging::Message;
using ola::rdm::CommandPrinter;
using ola::rdm::PidDescriptor;
using ola::rdm::PidStoreHelper;
using ola::rdm::RDMCommand;
using ola::rdm::RDMDiscoveryCommand;
using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;
using ola::rdm::UID;

typedef struct {
  bool help;
  bool timestamp;
  ola::log_level log_level;
  vector<string> args;  // extra args
  string read_file;  // file to read data from
} options;


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

      // the file with the pid data
      string pid_file;

      string write_file;  // write to this file if set

      // print timestamps as well, these aren't saved
      bool timestamp;
    };

    static void InitOptions(RDMSnifferOptions *options) {
      options->display_dmx_frames = false;
      options->dmx_slot_limit = DMX_UNIVERSE_SIZE;
      options->display_rdm_frames = true;
      options->summarize_rdm_frames = true;
      options->unpack_param_data = true;
      options->display_non_rdm_asc_frames = true;
      options->pid_file = PID_DATA_FILE;
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
    void DisplayRDMRequest(unsigned int start, unsigned int end);
    void DisplayRDMResponse(unsigned int start, unsigned int end);
    void DisplayDiscoveryCommand(unsigned int start, unsigned int end);
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
      m_pid_helper(options.pid_file, 4),
      m_command_printer(&cout, &m_pid_helper) {
  if (!m_pid_helper.Init())
    OLA_WARN << "Failed to init PidStore";
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
      case MAB:
        m_state = DATA;
        m_frame.Reset();
      case DATA:
        m_frame.AddByte(data_byte);
        break;
      default:
        OLA_WARN << "Unknown transition from state " << m_state <<
          ", with data 0x" << std::hex << static_cast<int>(control_byte) <<
          " 0x" << static_cast<int>(data_byte);
    }
  } else {
    // control byte
    if (data_byte == 0) {
      switch (m_state) {
        case BREAK:
          m_state = MAB;
          break;
        default:
          OLA_WARN << "Unknown transition from state " << m_state <<
            ", with data 0x" << std::hex << static_cast<int>(control_byte) <<
            " 0x" << static_cast<int>(data_byte);
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
          OLA_WARN << "Unknown transition from state " << m_state <<
            ", with data 0x" << std::hex << static_cast<int>(control_byte) <<
            " 0x" << static_cast<int>(data_byte);
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
      OLA_WARN << "Unknown transition from state " << m_state <<
        ", with data 0x" << std::hex << static_cast<int>(control_byte) <<
        " 0x" << static_cast<int>(data_byte);
    }
  }
}


/**
 * Process a frame based on what start code it has.
 */
void RDMSniffer::ProcessFrame() {
  switch (m_frame[0]) {
    case DMX512_START_CODE:
      if (m_options.display_dmx_frames)
        DisplayDmxFrame();
      break;
    case RDMCommand::START_CODE:
      DisplayRDMFrame();
      break;
    default:
      if (m_options.display_non_rdm_asc_frames)
        DisplayAlternateFrame();
  }
}


/**
 * Display a DMX Frame
 */
void RDMSniffer::DisplayDmxFrame() {
  unsigned int dmx_slot_count = m_frame.Size() - 1;
  MaybePrintTimestamp();
  cout << "DMX " << std::dec;
  if (m_options.dmx_slot_limit < dmx_slot_count)
    cout << m_options.dmx_slot_limit << "/";
  cout << dmx_slot_count << ":" << std::hex;
  unsigned int slots_to_display = std::min(
      dmx_slot_count,
      static_cast<unsigned int>(m_options.dmx_slot_limit));

  DisplayRawData(1, 1 + slots_to_display);
}


/**
 * Display a non (DMX/RDM) frame
 */
void RDMSniffer::DisplayAlternateFrame() {
  unsigned int slot_count = m_frame.Size() - 1;
  MaybePrintTimestamp();
  cout << "SC 0x" << std::hex << std::setw(2) << static_cast<int>(m_frame[0])
    << " " << std::dec << slot_count << ":" << std::hex;
  unsigned int slots_to_display = std::min(
      slot_count,
      static_cast<unsigned int>(m_options.dmx_slot_limit));
  DisplayRawData(1, 1 + slots_to_display);
}


/**
 * Display an RDM Frame.
 */
void RDMSniffer::DisplayRDMFrame() {
  unsigned int slot_count = m_frame.Size() - 1;

  if (slot_count < 21) {
    MaybePrintTimestamp();
    DisplayRawData(1, slot_count);
    return;
  }

  if (!m_options.summarize_rdm_frames)
    cout << "---------------------------------------" << endl;

  MaybePrintTimestamp();
  if (!m_options.summarize_rdm_frames && m_options.timestamp)
    cout << endl;

  switch (m_frame[20]) {
    case RDMCommand::GET_COMMAND:
    case RDMCommand::SET_COMMAND:
      DisplayRDMRequest(1, slot_count);
      break;
    case RDMCommand::GET_COMMAND_RESPONSE:
    case RDMCommand::SET_COMMAND_RESPONSE:
      DisplayRDMResponse(1, slot_count);
      return;
    case RDMCommand::DISCOVER_COMMAND:
    case RDMCommand::DISCOVER_COMMAND_RESPONSE:
      DisplayDiscoveryCommand(1, slot_count);
      break;
    /*
    case RDMCommand::DISCOVER_COMMAND_RESPONSE:
      DumpDiscover(length - 1, data + 1);
      return;
    */
    default:
      DisplayRawData(1, slot_count);
  }
}


/**
 * Display an RDM Request.
 */
void RDMSniffer::DisplayRDMRequest(unsigned int start, unsigned int end) {
  auto_ptr<RDMRequest> request(
      RDMRequest::InflateFromData(&m_frame[start], end - start + 1));

  if (request.get()) {
    m_command_printer.DisplayRequest(
        request.get(),
        m_options.summarize_rdm_frames,
        m_options.unpack_param_data);
  } else {
    DisplayRawData(start, end);
  }
}


/**
 * Display an RDM response.
 */
void RDMSniffer::DisplayRDMResponse(unsigned int start, unsigned int end) {
  ola::rdm::rdm_response_code code;
  auto_ptr<RDMResponse> response(
      RDMResponse::InflateFromData(&m_frame[start], end - start + 1, &code));

  if (response.get()) {
    m_command_printer.DisplayResponse(
        response.get(),
        m_options.summarize_rdm_frames,
        m_options.unpack_param_data);
  } else {
    DisplayRawData(start, end);
  }
}


/**
 * Display the discover unique branch command
 */
void RDMSniffer::DisplayDiscoveryCommand(unsigned int start,
                                         unsigned int end) {
  RDMCommand::RDMCommandClass command_class;
  bool ok = ola::rdm::GuessMessageType(NULL, &command_class,
                                       &m_frame[start], end - start + 1);

  auto_ptr<ola::rdm::DiscoveryRequest> request;
  auto_ptr<ola::rdm::DiscoveryResponse> response;
  if (ok && command_class == RDMCommand::DISCOVER_COMMAND) {
    request.reset(
        ola::rdm::DiscoveryRequest::InflateFromData(&m_frame[start],
                                                    end - start + 1));
  } else if (ok && command_class == RDMCommand::DISCOVER_COMMAND_RESPONSE) {
    response.reset(
        ola::rdm::DiscoveryResponse::InflateFromData(&m_frame[start],
                                                     end - start + 1));
  }

  if (request.get()) {
    m_command_printer.DisplayDiscoveryRequest(
        request.get(),
        m_options.summarize_rdm_frames,
        m_options.unpack_param_data);
  } else if (response.get()) {
    m_command_printer.DisplayDiscoveryResponse(
        response.get(),
        m_options.summarize_rdm_frames,
        m_options.unpack_param_data);
  } else {
    DisplayRawData(start, end);
  }
}


/**
 * Dump out the raw data if we couldn't parse it correctly.
 */
void RDMSniffer::DisplayRawData(unsigned int start, unsigned int end) {
  for (unsigned int i = start; i <= end; i++)
    cout << std::hex << std::setw(2) << static_cast<int>(m_frame[i]) << " ";
  cout << endl;
}


/**
 * Print the timestamp if timestamps are enabled
 */
void RDMSniffer::MaybePrintTimestamp() {
  if (!m_options.timestamp)
    return;

  ola::TimeStamp now;
  ola::Clock clock;
  clock.CurrentTime(&now);
  time_t seconds_since_epoch = now.Seconds();
  struct tm local_time;
  localtime_r(&seconds_since_epoch, &local_time);

  char output[24];
  strftime(output, sizeof(output), "%d-%m-%Y %H:%M:%S", &local_time);
  cout << output << "." << std::dec << static_cast<int>(now.MicroSeconds()) <<
    " ";
}

/*
 * Parse our command line options
 */
void ParseOptions(int argc,
                  char *argv[],
                  options *opts,
                  RDMSniffer::RDMSnifferOptions *sniffer_options) {
  static struct option long_options[] = {
      {"display-asc", no_argument, 0, 'a'},
      {"display-dmx", no_argument, 0, 'd'},
      {"dmx-slot-limit", required_argument, 0, 's'},
      {"full-rdm", no_argument, 0, 'r'},
      {"help", no_argument, 0, 'h'},
      {"log-level", required_argument, 0, 'l'},
      {"write-raw-dump", required_argument, 0, 'w'},
      {"parse-raw-dump", required_argument, 0, 'p'},
      {0, 0, 0, 0}
    };

  int option_index = 0;

  while (1) {
    int c = getopt_long(argc, argv, "adhl:s:truw:p:", long_options,
                        &option_index);

    if (c == -1)
      break;

    switch (c) {
      case 0:
        break;
      case 'a':
        sniffer_options->display_non_rdm_asc_frames = true;
        break;
      case 'd':
        sniffer_options->display_dmx_frames = true;
        break;
      case 'h':
        opts->help = true;
        break;
      case 'l':
        switch (atoi(optarg)) {
          case 0:
            // nothing is written at this level
            // so this turns logging off
            opts->log_level = ola::OLA_LOG_NONE;
            break;
          case 1:
            opts->log_level = ola::OLA_LOG_FATAL;
            break;
          case 2:
            opts->log_level = ola::OLA_LOG_WARN;
            break;
          case 3:
            opts->log_level = ola::OLA_LOG_INFO;
            break;
          case 4:
            opts->log_level = ola::OLA_LOG_DEBUG;
            break;
          default :
            break;
        }
        break;
      case 's':
        sniffer_options->dmx_slot_limit = atoi(optarg);
        break;
      case 't':
        sniffer_options->timestamp = true;
        break;
      case 'r':
        sniffer_options->summarize_rdm_frames = false;
        break;
      case 'w':
        if (!opts->read_file.empty())
          return;
        sniffer_options->write_file = optarg;
        break;
      case 'p':
        if (!sniffer_options->write_file.empty())
          return;
        opts->read_file = optarg;
        break;
      case '?':
        break;
      default:
       break;
    }
  }

  int index = optind;
  for (; index < argc; index++)
    opts->args.push_back(argv[index]);
  return;
}


/*
 * Display the help message
 */
void DisplayHelpAndExit(char *argv[]) {
  cout << "Usage: " << argv[0] << " [options] <device>\n"
  "\n"
  "Sniff traffic from a Enttec RDM Pro device.\n"
  "\n"
  "  -a, --display-asc   Display non-RDM alternate start code frames\n"
  "  -d, --display-dmx   Display DMX Frames (default false)\n"
  "  --dmx-slot-limit N  Only display N slots\n"
  "  -h, --help          Display this help message and exit.\n"
  "  -l, --log-level <level>  Set the logging level 0 .. 4.\n"
  "  -p, --parse-raw-dump <file>  Parse data from binary file.\n"
  "  -r, --full-rdm      Display the full RDM frame\n"
  "  -t, --timestamp     Include timestamps.\n"
  "  -w, --write-raw-dump <file>  Write data to binary file.\n"
  << endl;
  exit(0);
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
  RDMSniffer::RDMSnifferOptions sniffer_options;
  RDMSniffer::InitOptions(&sniffer_options);
  options opts;
  opts.log_level = ola::OLA_LOG_INFO;
  opts.help = false;
  opts.timestamp = false;
  ParseOptions(argc, argv, &opts, &sniffer_options);

  if (opts.help)
    DisplayHelpAndExit(argv);

  ola::InitLogging(opts.log_level, ola::OLA_LOG_STDERR);

  // if we're writing to a file
  if (!sniffer_options.write_file.empty()) {
    std::ofstream file;
    file.open(sniffer_options.write_file.c_str(),
              std::ios::out | std::ios::binary);
    if (!file.is_open()) {
      cerr << "Could not open file for writing: " << sniffer_options.write_file
        << endl;
      exit(EX_UNAVAILABLE);
    }
  }

  if (!opts.read_file.empty()) {
    // we're reading from a file
    ParseFile(&sniffer_options, opts.read_file);
    return EX_OK;
  }

  if (opts.args.size() != 1)
    DisplayHelpAndExit(argv);

  const string device = opts.args[0];

  ola::io::ConnectedDescriptor *descriptor =
      ola::plugin::usbpro::BaseUsbProWidget::OpenDevice(device);
  if (!descriptor)
    exit(EX_UNAVAILABLE);

  ola::io::SelectServer ss;
  descriptor->SetOnClose(ola::NewSingleCallback(&Stop, &ss));
  ss.AddReadDescriptor(descriptor);

  RDMSniffer sniffer(sniffer_options);
  DispatchingUsbProWidget widget(
      descriptor,
      ola::NewCallback(&sniffer, &RDMSniffer::HandleMessage));

  ss.Run();

  return EX_OK;
}
