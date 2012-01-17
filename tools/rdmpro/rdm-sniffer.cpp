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

#include <ola/BaseTypes.h>
#include <ola/Callback.h>
#include <ola/Logging.h>
#include <ola/network/NetworkUtils.h>
#include <ola/network/SelectServer.h>
#include <ola/rdm/PidStoreHelper.h>
#include <ola/rdm/RDMCommand.h>
#include <ola/rdm/RDMEnums.h>
#include <ola/rdm/RDMHelper.h>
#include <ola/rdm/RDMResponseCodes.h>
#include <ola/rdm/UID.h>

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "plugins/usbpro/BaseUsbProWidget.h"

using std::auto_ptr;
using std::cout;
using std::endl;
using std::string;
using std::stringstream;
using std::vector;
using ola::network::SelectServerInterface;
using ola::plugin::usbpro::DispatchingUsbProWidget;
using ola::messaging::Descriptor;
using ola::messaging::Message;
using ola::rdm::PidDescriptor;
using ola::rdm::PidStoreHelper;
using ola::rdm::RDMCommand;
using ola::rdm::RDMDiscoveryCommand;
using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;
using ola::rdm::UID;

typedef struct {
  bool help;
  ola::log_level log_level;
  vector<string> args;  // extra args
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
    };

    static void InitOptions(RDMSnifferOptions *options) {
      options->display_dmx_frames = false;
      options->dmx_slot_limit = DMX_UNIVERSE_SIZE;
      options->display_rdm_frames = true;
      options->summarize_rdm_frames = true;
      options->unpack_param_data = false;
      options->unpack_param_data = true;
      options->display_non_rdm_asc_frames = true;
      options->pid_file = PID_DATA_FILE;
    }

    RDMSniffer(SelectServerInterface *ss, const RDMSnifferOptions &options);

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

    SelectServerInterface *m_ss;
    SnifferState m_state;
    ByteStream m_frame;
    RDMSnifferOptions m_options;
    PidStoreHelper m_pid_helper;

    void ProcessTuple(uint8_t control_byte, uint8_t data_byte);
    void ProcessFrame();

    void DisplayDmxFrame();
    void DisplayAlternateFrame();
    void DisplayRDMFrame();
    void DisplayRDMRequest(unsigned int start, unsigned int end);
    void DisplayRDMResponse(unsigned int start, unsigned int end);
    void DisplayDiscoveryCommand(unsigned int start, unsigned int end);
    void DisplayRawData(unsigned int start, unsigned int end);
    void DisplayParamData(const PidDescriptor *pid_descriptor,
                          bool is_get,
                          const uint8_t *data,
                          unsigned int length);

    static const uint8_t SNIFFER_PACKET = 0x81;
    static const uint8_t SNIFFER_PACKET_SIZE = 200;
    // if the high bit is set, this is a data byte, otherwise it's a control
    // byte
    static const uint8_t DATA_MASK = 0x80;
    static const uint8_t BYTES_PER_LINE = 8;
};


RDMSniffer::RDMSniffer(SelectServerInterface *ss,
                       const RDMSnifferOptions &options)
    : m_ss(ss),
      m_state(IDLE),
      m_options(options),
      m_pid_helper(options.pid_file, 4) {
  if (!m_pid_helper.Init())
    OLA_WARN << "Failed to init PidStore";
}


/*
 * Handle the widget replies
 */
void RDMSniffer::HandleMessage(uint8_t label,
                               const uint8_t *data,
                               unsigned int length) {
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
  cout << "RDM " << std::dec << slot_count << ": " << std::hex;

  if (slot_count < 21) {
    DisplayRawData(1, slot_count);
    return;
  }

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
  if (!request.get()) {
    DisplayRawData(start, end);
    return;
  }

  const PidDescriptor *descriptor = m_pid_helper.GetDescriptor(
       request->ParamId(),
       request->DestinationUID().ManufacturerId());
  bool is_get = request->CommandClass() == RDMCommand::GET_COMMAND_RESPONSE;

  if (m_options.summarize_rdm_frames) {
    cout <<
      request->SourceUID() << " -> " << request->DestinationUID() << " " <<
      (is_get ? "GET" : "SET") <<
      ", sub-device: " << std::dec << request->SubDevice() <<
      ", tn: " << static_cast<int>(request->TransactionNumber()) <<
      ", port: " << std::dec << static_cast<int>(request->PortId()) <<
      ", PID 0x" << std::hex << std::setfill('0') << std::setw(4) <<
        request->ParamId();
      if (descriptor)
        cout << " (" << descriptor->Name() << ")";
      cout << ", pdl: " << std::dec << request->ParamDataSize() << endl;
  } else {
    cout << endl;
    cout << "  Sub start code : 0x" << std::hex <<
      static_cast<unsigned int>(m_frame[start]) << endl;
    cout << "  Message length : " <<
      static_cast<unsigned int>(m_frame[start + 1]) << endl;
    cout << "  Dest UID       : " << request->DestinationUID() << endl;
    cout << "  Source UID     : " << request->SourceUID() << endl;
    cout << "  Transaction #  : " << std::dec <<
      static_cast<unsigned int>(request->TransactionNumber()) << endl;
    cout << "  Port ID        : " << std::dec <<
      static_cast<unsigned int>(request->PortId()) << endl;
    cout << "  Message count  : " << std::dec <<
      static_cast<unsigned int>(request->MessageCount()) << endl;
    cout << "  Sub device     : " << std::dec << request->SubDevice() << endl;
    cout << "  Command class  : " << (is_get ? "GET" : "SET") << endl;
    cout << "  Param ID       : 0x" << std::setfill('0') << std::setw(4) <<
      std::hex << request->ParamId();
    if (descriptor)
      cout << " (" << descriptor->Name() << ")";
    cout << endl;
    cout << "  Param data len : " << std::dec << request->ParamDataSize() <<
      endl;
    DisplayParamData(descriptor,
                     is_get,
                     request->ParamData(),
                     request->ParamDataSize());
  }
}


/**
 * Display an RDM response
 */
void RDMSniffer::DisplayRDMResponse(unsigned int start, unsigned int end) {
  ola::rdm::rdm_response_code code;
  unsigned int length = end - start + 1;
  auto_ptr<RDMResponse> response(
      RDMResponse::InflateFromData(&m_frame[start], length, &code));

  if (!response.get()) {
    DisplayRawData(start, end);
    return;
  }

  const PidDescriptor *descriptor = m_pid_helper.GetDescriptor(
       response->ParamId(),
       response->SourceUID().ManufacturerId());

  bool is_get = response->CommandClass() == RDMCommand::GET_COMMAND_RESPONSE;

  if (m_options.summarize_rdm_frames) {
    cout <<
      response->SourceUID() << " -> " << response->DestinationUID() << " " <<
      (is_get ? "GET_RESPONSE" : "SET_RESPONSE") <<
      ", sub-device: " << std::dec << response->SubDevice() <<
      ", tn: " << static_cast<int>(response->TransactionNumber()) <<
      ", response type: ";

    switch (response->ResponseType()) {
      case ola::rdm::RDM_ACK:
        cout << "ACK";
        break;
      case ola::rdm::RDM_ACK_TIMER:
        cout << "ACK TIMER";
        break;
      case ola::rdm::RDM_NACK_REASON:
        if (length >= 26) {
          uint16_t reason;
          memcpy(reinterpret_cast<uint8_t*>(&reason),
                 reinterpret_cast<const void*>(&m_frame[start + 23]),
                 sizeof(reason));
          reason = ola::network::NetworkToHost(reason);
          cout << "NACK (" << ola::rdm::NackReasonToString(reason) << ")";
        } else {
          cout << "Malformed NACK ";
        }
        break;
      case ola::rdm::ACK_OVERFLOW:
        cout << "ACK OVERFLOW";
        break;
      default:
        cout << "Unknown (" << response->ResponseType() << ")";
    }
    cout << ", PID 0x" << std::hex <<
      std::setfill('0') << std::setw(4) << response->ParamId();
    if (descriptor)
      cout << " (" << descriptor->Name() << ")";
    cout << ", pdl: " << std::dec << response->ParamDataSize() << endl;
  } else {
    cout << endl;
    cout << "  Sub start code : 0x" << std::hex <<
      static_cast<unsigned int>(m_frame[start]) << endl;
    cout << "  Message length : " <<
      static_cast<unsigned int>(m_frame[start + 1]) << endl;
    cout << "  Dest UID       : " << response->DestinationUID() << endl;
    cout << "  Source UID     : " << response->SourceUID() << endl;
    cout << "  Transaction #  : " << std::dec <<
      static_cast<unsigned int>(response->TransactionNumber()) << endl;
    cout << "  Response Type  : ";
    switch (response->ResponseType()) {
      case ola::rdm::RDM_ACK:
        cout << "ACK";
        break;
      case ola::rdm::RDM_ACK_TIMER:
        cout << "ACK TIMER";
        break;
      case ola::rdm::RDM_NACK_REASON:
        if (length >= 26) {
          uint16_t reason;
          memcpy(reinterpret_cast<uint8_t*>(&reason),
                 reinterpret_cast<const void*>(&m_frame[start + 23]),
                 sizeof(reason));
          reason = ola::network::NetworkToHost(reason);
          cout << "NACK (" << ola::rdm::NackReasonToString(reason) << ")";
        } else {
          cout << "Malformed NACK ";
        }
        break;
      case ola::rdm::ACK_OVERFLOW:
        cout << "ACK OVERFLOW";
        break;
      default:
        cout << "Unknown (" << response->ResponseType() << ")";
    }
    cout  << endl;
    cout << "  Message count  : " << std::dec <<
      static_cast<unsigned int>(response->MessageCount()) << endl;
    cout << "  Sub device     : " << std::dec << response->SubDevice() << endl;
    cout << "  Command class  : " << (is_get ? "GET_RESPONSE" : "SET_RESPONSE")
      << endl;

    cout << "  Param ID       : 0x" << std::setfill('0') << std::setw(4) <<
      std::hex << response->ParamId();
    if (descriptor)
      cout << " (" << descriptor->Name() << ")";
    cout << endl;
    cout << "  Param data len : " << std::dec << response->ParamDataSize() <<
      endl;
    DisplayParamData(descriptor,
                     is_get,
                     response->ParamData(),
                     response->ParamDataSize());
  }
}


/**
 * Display the discover unique branch command
 */
void RDMSniffer::DisplayDiscoveryCommand(unsigned int start,
                                         unsigned int end) {
  unsigned int length = end - start + 1;
  auto_ptr<RDMDiscoveryCommand> request(
      RDMDiscoveryCommand::InflateFromData(&m_frame[start], length));

  if (!request.get()) {
    DisplayRawData(start, end);
    return;
  }

  string param_name;
  switch (request->ParamId()) {
    case ola::rdm::PID_DISC_UNIQUE_BRANCH:
      param_name = "DISC_UNIQUE_BRANCH";
      break;
    case ola::rdm::PID_DISC_MUTE:
      param_name = "DISC_MUTE";
      break;
    case ola::rdm::PID_DISC_UN_MUTE:
      param_name = "DISC_UN_MUTE";
      break;
  }

  if (m_options.summarize_rdm_frames) {
    cout <<
      request->SourceUID() << " -> " << request->DestinationUID() <<
      " DISCOVERY_COMMAND" <<
      ", tn: " << static_cast<int>(request->TransactionNumber()) <<
      ", PID 0x" << std::hex << std::setfill('0') << std::setw(4) <<
        request->ParamId();
      if (!param_name.empty())
        cout << " (" << param_name << ")";
      if (request->ParamId() == ola::rdm::PID_DISC_UNIQUE_BRANCH &&
          request->ParamDataSize() == 2 * UID::UID_SIZE) {
        const uint8_t *param_data = request->ParamData();
        UID lower(param_data);
        UID upper(param_data + UID::UID_SIZE);
        cout << ", (" << lower << ", " << upper << ")";
      } else {
        cout << ", pdl: " << std::dec << request->ParamDataSize();
      }
      cout << endl;
  } else {
    cout << endl;
    cout << "  Sub start code : 0x" << std::hex <<
      static_cast<unsigned int>(m_frame[start]) << endl;
    cout << "  Message length : " <<
      static_cast<unsigned int>(m_frame[start + 1]) << endl;
    cout << "  Dest UID       : " << request->DestinationUID() << endl;
    cout << "  Source UID     : " << request->SourceUID() << endl;
    cout << "  Transaction #  : " << std::dec <<
      static_cast<unsigned int>(request->TransactionNumber()) << endl;
    cout << "  Port ID        : " << std::dec <<
      static_cast<unsigned int>(request->PortId()) << endl;
    cout << "  Message count  : " << std::dec <<
      static_cast<unsigned int>(request->MessageCount()) << endl;
    cout << "  Sub device     : " << std::dec << request->SubDevice() << endl;
    cout << "  Command class  : DISCOVERY_COMMAND" << endl;
    cout << "  Param ID       : 0x" << std::setfill('0') << std::setw(4) <<
      std::hex << request->ParamId();
    if (!param_name.empty())
      cout << " (" << param_name << ")";
    cout << endl;
    cout << "  Param data len : " << std::dec << request->ParamDataSize() <<
      endl;
    DisplayParamData(NULL,
                     false,
                     request->ParamData(),
                     request->ParamDataSize());
  }
}


/**
 * Dump out the raw data if we couldn't parse it correctly.
 */
void RDMSniffer::DisplayRawData(unsigned int start, unsigned int end) {
  for (unsigned int i = start; i < end; i++)
    cout << std::hex << std::setw(2) << static_cast<int>(m_frame[i]) << " ";
  cout << endl;
}


/**
 * Display parameter data in hex and ascii
 */
void RDMSniffer::DisplayParamData(const PidDescriptor *pid_descriptor,
                                  bool is_get,
                                  const uint8_t *data,
                                  unsigned int length) {
  if (!length)
    return;

  cout << "  Param data:" << endl;
  if (m_options.unpack_param_data && pid_descriptor) {
    const Descriptor *descriptor =
        is_get ? pid_descriptor->GetResponse() : pid_descriptor->SetResponse();

    if (descriptor) {
      auto_ptr<const Message> message(
          m_pid_helper.DeserializeMessage(descriptor, data, length));

      if (message.get()) {
        cout << m_pid_helper.MessageToString(message.get());
        return;
      }
    }
  }

  // otherwise just display the raw data
  stringstream raw;
  raw << std::setw(2) << std::hex;
  stringstream ascii;
  for (unsigned int i = 0; i != length; i++) {
    raw << std::setw(2) << std::setfill('0') <<
      static_cast<unsigned int>(data[i]) << " ";
    if (data[i] >= ' ' && data[i] <= '~')
      ascii << data[i];
    else
      ascii << ".";

    if (i % BYTES_PER_LINE == BYTES_PER_LINE - 1) {
      cout << "    " << raw.str() << " " << ascii.str() << endl;
      raw.str("");
      ascii.str("");
    }
  }
  if (length % BYTES_PER_LINE != 0) {
    // pad if needed
    raw << string(3 * (BYTES_PER_LINE - (length % BYTES_PER_LINE)), ' ');
    cout << "    " << raw.str() << " " << ascii.str() << endl;
  }
}


/*
void RDMSniffer::DumpDiscover(unsigned int length, const uint8_t *data) {
  DumpRawPacket(length, data);
}
*/

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
      {"unpack-params", no_argument, 0, 'u'},
      {0, 0, 0, 0}
    };

  int option_index = 0;

  while (1) {
    int c = getopt_long(argc, argv, "adhl:s:ru", long_options, &option_index);

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
      case 'r':
        sniffer_options->summarize_rdm_frames = false;
        break;
      case 'u':
        sniffer_options->unpack_param_data = true;
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
  "  -r, --full-rdm      Display the full RDM frame\n"
  "  -u, --unpack-params Unpack parameter data.\n"
  << endl;
  exit(0);
}


void Stop(ola::network::SelectServer *ss) {
  ss->Terminate();
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
  ParseOptions(argc, argv, &opts, &sniffer_options);

  if (opts.help)
    DisplayHelpAndExit(argv);

  if (opts.args.size() != 1)
    DisplayHelpAndExit(argv);
  const string device = opts.args[0];

  ola::InitLogging(opts.log_level, ola::OLA_LOG_STDERR);

  ola::network::ConnectedDescriptor *descriptor =
      ola::plugin::usbpro::BaseUsbProWidget::OpenDevice(device);
  if (!descriptor)
    exit(EX_UNAVAILABLE);

  ola::network::SelectServer ss;
  descriptor->SetOnClose(ola::NewSingleCallback(&Stop, &ss));
  ss.AddReadDescriptor(descriptor);

  RDMSniffer sniffer(&ss, sniffer_options);
  DispatchingUsbProWidget widget(
      descriptor,
      ola::NewCallback(&sniffer, &RDMSniffer::HandleMessage));

  ss.Run();

  return EX_OK;
}
