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
 * ja-rule.cpp
 * Host side code for ja-rule
 * Copyright (C) 2015 Simon Newton
 */

#include <ola/Callback.h>
#include <ola/Constants.h>
#include <ola/Logging.h>
#include <ola/base/Array.h>
#include <ola/base/Init.h>
#include <ola/base/Flags.h>
#include <ola/base/SysExits.h>
#include <ola/io/SelectServer.h>
#include <ola/io/StdinHandler.h>
#include <ola/rdm/RDMCommand.h>
#include <ola/rdm/RDMCommandSerializer.h>
#include <ola/rdm/UID.h>
#include <ola/strings/Format.h>
#include <ola/thread/Thread.h>
#include <ola/util/Utils.h>

#include <iostream>
#include <memory>
#include <string>

#include "tools/ja-rule/JaRuleEndpoint.h"
#include "tools/ja-rule/USBDeviceManager.h"

using ola::NewCallback;
using ola::io::SelectServer;
using ola::io::StdinHandler;
using ola::rdm::RDMCommand;
using ola::rdm::RDMCommandSerializer;
using ola::rdm::RDMDiscoveryResponse;
using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;
using ola::rdm::RDMSetRequest;
using ola::rdm::UID;
using ola::rdm::RDMStatusCode;
using ola::strings::ToHex;
using ola::thread::Thread;
using ola::utils::JoinUInt8;
using ola::utils::SplitUInt16;
using std::auto_ptr;
using std::cout;
using std::endl;
using std::string;

DEFINE_string(target_uid, "7a70:00000001",
              "The UID of the responder to control.");
DEFINE_string(controller_uid, "7a70:fffffe00", "The UID of the controller.");
DEFINE_string(lower_uid, "0000:00000000", "The lower UID for the DUB.");
DEFINE_string(upper_uid, "ffff:ffffffff", "The upper UID for the DUB.");

/**
 * @brief Print messages received from the device.
 */
class MessageHandler : public JaRuleEndpoint::MessageHandlerInterface {
 public:
  MessageHandler() {}

  void NewMessage(const Message& message) {
    switch (message.command) {
      case JaRuleEndpoint::ECHO_COMMAND:
        PrintEcho(message);
        break;
      case JaRuleEndpoint::TX_DMX:
        PrintResponse(message);
        break;
      case JaRuleEndpoint::GET_LOG:
        PrintLog(message.payload, message.payload_size);
        break;
      case JaRuleEndpoint::GET_FLAGS:
        PrintFlags(message);
        break;
      case JaRuleEndpoint::WRITE_LOG:
        PrintAck(message);
        break;
      case JaRuleEndpoint::RESET_DEVICE:
        PrintAck(message);
        break;
      case JaRuleEndpoint::RDM_DUB:
        PrintDUBResponse(message);
        break;
      case JaRuleEndpoint::RDM_REQUEST:
        PrintResponse(message);
        break;
      case JaRuleEndpoint::GET_BREAK_TIME:
        PrintTime(message, MICRO_SECONDS);
        break;
      case JaRuleEndpoint::SET_BREAK_TIME:
        PrintAck(message);
        break;
      case JaRuleEndpoint::GET_MAB_TIME:
        PrintTime(message, MICRO_SECONDS);
        break;
      case JaRuleEndpoint::SET_MAB_TIME:
        PrintAck(message);
        break;
      case JaRuleEndpoint::GET_RDM_WAIT_TIME:
        PrintTime(message, TENTHS_OF_MILLI_SECONDS);
        break;
      case JaRuleEndpoint::SET_RDM_WAIT_TIME:
        PrintAck(message);
        break;
      case JaRuleEndpoint::GET_RDM_BROADCAST_LISTEN:
        PrintTime(message, TENTHS_OF_MILLI_SECONDS);
        break;
      case JaRuleEndpoint::SET_RDM_BROADCAST_LISTEN:
        PrintAck(message);
        break;
      case JaRuleEndpoint::RDM_BROADCAST_REQUEST:
        PrintAck(message);
        break;
      default:
        OLA_WARN << "Unknown command: " << ToHex(message.command);
    }

    if (message.flags & LOGS_PENDING_FLAG) {
      OLA_INFO << "Logs pending!";
    }
    if (message.flags & FLAGS_CHANGED_FLAG) {
      OLA_INFO << "Flags changed!";
    }
    if (message.flags & MSG_TRUNCATED_FLAG) {
      OLA_INFO << "Message truncated";
    }
  }

 private:
  enum TimeFormat {
    MICRO_SECONDS,
    TENTHS_OF_MILLI_SECONDS
  };

  string m_log_buffer;

  void PrintEcho(const Message& message) {
    string response;
    if (message.payload && message.payload_size > 0) {
       response.append(reinterpret_cast<const char*>(message.payload),
                       message.payload_size);
    }
    OLA_INFO << "Echo Reply (" << static_cast<int>(message.return_code) << "): "
                               << response;
  }

  void PrintLog(const uint8_t* data, unsigned int size) {
    if (!data || size == 0) {
      OLA_WARN << "Malformed logs response";
      return;
    }

    bool overflow = data[0];
    data++;
    size--;

    m_log_buffer.append(reinterpret_cast<const char*>(data), size);

    size_t start = 0;
    // Each log line is null terminated. Log lines may span messages.
    while (true) {
      size_t pos = m_log_buffer.find_first_of(static_cast<char>(0), start);
      if (pos == string::npos) {
        m_log_buffer.erase(0, start);
        break;
      }
      OLA_INFO << "[" << start << ", " << pos << "]";
      OLA_INFO << "LOG: " << m_log_buffer.substr(start, pos - start);
      if (pos + 1 == m_log_buffer.size()) {
        m_log_buffer.clear();
        break;
      } else {
        start = pos + 1;
      }
    }

    if (overflow) {
      OLA_INFO << "Log overflow occured, some messages have been lost";
    }
  }

  void PrintFlags(const Message& message) {
    OLA_INFO << "Flags (" << static_cast<int>(message.return_code) << "):";
    if (message.payload && message.payload_size) {
      ola::strings::FormatData(&std::cout, message.payload,
                               message.payload_size);
    }
  }

  void PrintAck(const Message& message) {
    OLA_INFO << "ACK (" << static_cast<int>(message.return_code)
             << "): payload_size: " << message.payload_size;
  }


  void PrintDUBResponse(const Message& message) {
    OLA_INFO << "DUB Response: RC: " << static_cast<int>(message.return_code)
             << ", size: " << message.payload_size;
  }

  void PrintResponse(const Message& message) {
    OLA_INFO << "RC (" << static_cast<int>(message.return_code)
             << "): payload_size: " << message.payload_size;

    if (message.payload_size == 0 || message.payload == NULL) {
      return;
    }

    if (message.payload[0] == RDMCommand::START_CODE) {
      RDMStatusCode status_code;
      // SKip over the start code.
      auto_ptr<RDMResponse> response(RDMResponse::InflateFromData(
          message.payload + 1, message.payload_size - 1, &status_code));

      if (!response.get()) {
        OLA_WARN << "Failed to inflate RDM response";
        if (message.payload == NULL || message.payload_size == 0) {
          ola::strings::FormatData(&std::cout, message.payload,
                                   message.payload_size);
        }
      }
      OLA_INFO << *response;
    }
  }

  void PrintTime(const Message& message, TimeFormat format) {
    uint16_t value = 0;
    if (message.return_code != 0) {
      OLA_INFO << "Failed (" << static_cast<int>(message.return_code)
               << "): payload_size: " << message.payload_size;
      return;
    }

    if (message.payload_size == sizeof(value)) {
      value = JoinUInt8(message.payload[1], message.payload[0]);
      if (format == MICRO_SECONDS) {
        OLA_INFO << "Time: " << value << " us";
      } else if (format == TENTHS_OF_MILLI_SECONDS) {
        float adjusted_time = value / 10.0;
        OLA_INFO << "Time: " << adjusted_time << " ms";
      }
    } else {
      OLA_WARN << "Payload size mismatch";
    }
  }

  DISALLOW_COPY_AND_ASSIGN(MessageHandler);
};

/**
 * @brief Wait on input from the keyboard, and based on the input, send
 * messages to the device.
 */
class InputHandler {
 public:
  explicit InputHandler(SelectServer* ss,
                        const UID &controller_uid,
                        const UID &target_uid,
                        const UID &lower_uid,
                        const UID &upper_uid)
      : m_ss(ss),
        m_our_uid(controller_uid),
        m_target_uid(target_uid),
        m_lower_uid(lower_uid),
        m_upper_uid(upper_uid),
        m_stdin_handler(
            new StdinHandler(ss, ola::NewCallback(this, &InputHandler::Input))),
        m_device(NULL),
        m_log_count(0),
        m_dmx_slot_data(0),
        m_mode(DEFAULT),
        m_break(176),
        m_mab(12),
        m_rdm_wait_time(28),
        m_rdm_broadcast_listen(28) {
  }

  ~InputHandler() {
    m_stdin_handler.reset();
  }

  void DeviceEvent(USBDeviceManager::EventType event,
                   JaRuleEndpoint* device) {
    if (event == USBDeviceManager::DEVICE_ADDED) {
      OLA_INFO << "Open Lighting Device added";
      m_device = device;
      m_device->SetHandler(&m_handler);
    } else {
      OLA_INFO << "Open Lighting Device removed";
      m_device = NULL;
    }
  }

  void Input(int c) {
    switch (c) {
      case '\n':
        Commit();
        break;
      case 27:  // Escape
        if (m_mode != DEFAULT) {
          cout << "Edit Aborted" << endl;
          m_mode = DEFAULT;
        }
        break;
      case '+':
        Adjust(true);
        break;
      case '-':
        Adjust(false);
        break;
      case '0':
        SendZeroDMX();
        break;
      case '2':
        SendDoubleDMX();
        break;
      case 'b':
        GetBreakTime();
        break;
      case 'B':
        cout << "Editing Break, use +/- to adjust, Enter commits, Esc to abort"
             << endl;
        m_mode = EDIT_BREAK;
        break;
      case 'd':
        SendDUB(UID(0, 0), UID::AllDevices());
        break;
      case 'D':
        SendDUB(m_lower_uid, m_upper_uid);
        break;
      case 'e':
        SendEcho();
        break;
      case 'f':
        GetFlags();
        break;
      case 'h':
        PrintCommands();
        break;
      case 'i':
        SendIdentify(true);
        break;
      case 'I':
        SendIdentify(false);
        break;
      case 'l':
        GetLogs();
        break;
      case 'm':
        SendMute(UID::AllDevices());
        break;
      case 'M':
        SendMute(m_target_uid);
        break;
      case 'r':
        ResetDevice();
        break;
      case 't':
        SendDMX();
        break;
      case 'u':
        SendUnMute(UID::AllDevices());
        break;
      case 'U':
        SendUnMute(m_target_uid);
        break;
      case 'q':
        m_ss->Terminate();
        break;
      case 'w':
        WriteLog();
        break;
      case 'x':
        GetRDMWaitTime();
        break;
      case 'X':
        cout << "Editing RDM Wait time, use +/- to adjust, Enter commits, "
             << "Esc to abort" << endl;
        m_mode = EDIT_RDM_WAIT_TIME;
        break;
      case 'y':
        GetMABTime();
        break;
      case 'Y':
        cout << "Editing MAB, use +/- to adjust, Enter commits, Esc to abort"
             << endl;
        m_mode = EDIT_MAB;
        break;
      case 'z':
        GetRDMBroadcastListen();
        break;
      case 'Z':
        cout << "Editing RDM Broadcast Listen Time, use +/- to adjust, Enter "
             << "commits, Esc to abort" << endl;
        m_mode = EDIT_RDM_BROADCAST_LISTEN;
        break;
      default:
        {}
    }
  }

  void PrintCommands() {
    cout << "Commands:" << endl;
    cout << " 0 - Send a 0 length DMX frame" << endl;
    cout << " 2 - Send 2 DMX frames back to back" << endl;
    cout << " b - Get Break time" << endl;
    cout << " B - Set Break time" << endl;
    cout << " d - Send a DUB frame" << endl;
    cout << " e - Send Echo command" << endl;
    cout << " f - Fetch Flags State" << endl;
    cout << " h - Print this help message" << endl;
    cout << " i - Identify On to " << m_target_uid << endl;
    cout << " I - Identify Off to " << m_target_uid << endl;
    cout << " l - Fetch Logs" << endl;
    cout << " m - Send a broadcast mute" << endl;
    cout << " M - Send a mute to " << m_target_uid << endl;
    cout << " q - Quit" << endl;
    cout << " r - Reset" << endl;
    cout << " t - Send DMX frame" << endl;
    cout << " u - Send a broadcast unmute" << endl;
    cout << " U - Send an unmute to " << m_target_uid << endl;
    cout << " w - Write Log" << endl;
    cout << " x - Get RDM Wait time" << endl;
    cout << " X - Set RDM Wait time" << endl;
    cout << " y - Get MAB time" << endl;
    cout << " Y - Set MAB time" << endl;
    cout << " z - Get RDM Broadcast Listen time" << endl;
    cout << " Z - Set RDM Broadcast Listen time" << endl;
  }

 private:
  enum Mode {
    DEFAULT,
    EDIT_BREAK,
    EDIT_MAB,
    EDIT_RDM_BROADCAST_LISTEN,
    EDIT_RDM_WAIT_TIME
  };

  SelectServer* m_ss;
  UID m_our_uid, m_target_uid, m_lower_uid, m_upper_uid;
  auto_ptr<StdinHandler> m_stdin_handler;
  MessageHandler m_handler;
  JaRuleEndpoint* m_device;
  unsigned int m_log_count;
  uint8_t m_dmx_slot_data;
  Mode m_mode;
  uint16_t m_break;
  uint16_t m_mab;
  uint16_t m_rdm_wait_time;  // in 0.1ms increments.
  uint16_t m_rdm_broadcast_listen;  // in 0.1ms increments.

  bool CheckForDevice() const {
    if (!m_device) {
      OLA_INFO << "Device not present";
      return false;
    }
    return true;
  }

  void GetLogs() {
    if (!CheckForDevice()) {
      return;
    }

    m_device->SendMessage(JaRuleEndpoint::GET_LOG, NULL, 0);
  }

  void ResetDevice() {
    if (!CheckForDevice()) {
      return;
    }

    m_device->SendMessage(JaRuleEndpoint::RESET_DEVICE, NULL, 0);
  }

  void GetFlags() {
    if (!CheckForDevice()) {
      return;
    }

    m_device->SendMessage(JaRuleEndpoint::GET_FLAGS, NULL, 0);
  }

  void _SendDMX(const uint8_t *data, unsigned int size) {
    if (!CheckForDevice()) {
      return;
    }
    m_device->SendMessage(JaRuleEndpoint::TX_DMX, data, size);
  }

  void Adjust(bool increase) {
    if (m_mode == EDIT_BREAK) {
      if (increase) {
        m_break++;
      } else {
        m_break--;
      }
      cout << "Break is now " << m_break << " us" << endl;
    } else if (m_mode == EDIT_MAB) {
      if (increase) {
        m_mab++;
      } else {
        m_mab--;
      }
      cout << "MAB is now " << m_mab << " us" << endl;
    } else if (m_mode == EDIT_RDM_WAIT_TIME) {
      if (increase) {
        m_rdm_wait_time++;
      } else {
        m_rdm_wait_time--;
      }
      float wait_time = m_rdm_wait_time / 10.0;
      cout << "RDM Wait time is now " << wait_time << " ms" << endl;
    } else if (m_mode == EDIT_RDM_BROADCAST_LISTEN) {
      if (increase) {
        m_rdm_broadcast_listen++;
      } else {
        m_rdm_broadcast_listen--;
      }
      float listen_time = m_rdm_broadcast_listen / 10.0;
      cout << "RDM Broadcast listen is now " << listen_time << " ms"
           << endl;
    }
  }

  void Commit() {
    if (!CheckForDevice()) {
      return;
    }

    uint8_t payload[2];
    switch (m_mode) {
      case EDIT_BREAK:
        SplitUInt16(m_break, &payload[1], &payload[0]);
        m_device->SendMessage(JaRuleEndpoint::SET_BREAK_TIME, payload,
                              arraysize(payload));
        break;
      case EDIT_MAB:
        SplitUInt16(m_mab, &payload[1], &payload[0]);
        m_device->SendMessage(JaRuleEndpoint::SET_MAB_TIME, payload,
                              arraysize(payload));
        break;
      case EDIT_RDM_WAIT_TIME:
        SplitUInt16(m_rdm_wait_time, &payload[1], &payload[0]);
        m_device->SendMessage(JaRuleEndpoint::SET_RDM_WAIT_TIME,
                              payload, arraysize(payload));
        break;
      case EDIT_RDM_BROADCAST_LISTEN:
        SplitUInt16(m_rdm_broadcast_listen, &payload[1], &payload[0]);
        m_device->SendMessage(JaRuleEndpoint::SET_RDM_BROADCAST_LISTEN,
                              payload, arraysize(payload));
        break;
      default:
        OLA_WARN << "Unknown mode " << m_mode;
    }
    m_mode = DEFAULT;
  }

  void SendZeroDMX() {
    _SendDMX(NULL, 0);
  }

  void SendDoubleDMX() {
    uint8_t payload[512];
    memset(payload, 0, 512);
    payload[0] = m_dmx_slot_data;
    _SendDMX(payload, arraysize(payload));
    m_dmx_slot_data += 16;
    payload[0] = m_dmx_slot_data;
    _SendDMX(payload, arraysize(payload));
    m_dmx_slot_data += 16;
  }

  void SendDMX() {
    if (!CheckForDevice()) {
      return;
    }

    uint8_t payload[512];
    memset(payload, 0, 512);
    payload[0] = m_dmx_slot_data;
    _SendDMX(payload, arraysize(payload));
    m_dmx_slot_data += 16;
  }

  void SendEcho() {
    if (!CheckForDevice()) {
      return;
    }

    const uint8_t payload[] = {'e', 'c', 'h', 'o', ' ', 't', 'e', 's', 't'};
    m_device->SendMessage(JaRuleEndpoint::ECHO_COMMAND, payload,
                          arraysize(payload));
  }

  void GetBreakTime() {
    if (!CheckForDevice()) {
      return;
    }

    m_device->SendMessage(JaRuleEndpoint::GET_BREAK_TIME, NULL, 0);
  }

  void GetMABTime() {
    if (!CheckForDevice()) {
      return;
    }

    m_device->SendMessage(JaRuleEndpoint::GET_MAB_TIME, NULL, 0);
  }

  void GetRDMWaitTime() {
    if (!CheckForDevice()) {
      return;
    }
    m_device->SendMessage(JaRuleEndpoint::GET_RDM_WAIT_TIME, NULL, 0);
  }

  void GetRDMBroadcastListen() {
    if (!CheckForDevice()) {
      return;
    }
    m_device->SendMessage(JaRuleEndpoint::GET_RDM_BROADCAST_LISTEN, NULL,
                          0);
  }

  void SendDUB(const UID &lower, const UID &upper) {
    if (!CheckForDevice()) {
      return;
    }

    auto_ptr<RDMRequest> request(
        ola::rdm::NewDiscoveryUniqueBranchRequest(m_our_uid, lower, upper, 0));
    unsigned int rdm_length = RDMCommandSerializer::RequiredSize(*request);
    uint8_t data[rdm_length];
    RDMCommandSerializer::Pack(*request, data, &rdm_length);
    OLA_INFO << "Sending " << rdm_length << " RDM command.";
    m_device->SendMessage(JaRuleEndpoint::RDM_DUB, data, rdm_length);
  }

  void SendIdentify(bool identify_on) {
    if (!CheckForDevice()) {
      return;
    }

    uint8_t param_data = identify_on;
    RDMSetRequest request(m_our_uid, m_target_uid, 0, 0, 0,
                          ola::rdm::PID_IDENTIFY_DEVICE, &param_data,
                          sizeof(param_data));

    unsigned int rdm_length = RDMCommandSerializer::RequiredSize(request);
    uint8_t data[rdm_length];
    RDMCommandSerializer::Pack(request, data, &rdm_length);
    m_device->SendMessage(JaRuleEndpoint::RDM_REQUEST, data, rdm_length);
  }

  void SendMute(const UID &target) {
    if (!CheckForDevice()) {
      return;
    }

    auto_ptr<RDMRequest> request(
        ola::rdm::NewMuteRequest(m_our_uid, target, 0));

    unsigned int rdm_length = RDMCommandSerializer::RequiredSize(*request);
    uint8_t data[rdm_length];
    RDMCommandSerializer::Pack(*request, data, &rdm_length);
    m_device->SendMessage(JaRuleEndpoint::RDM_REQUEST, data, rdm_length);
  }

  void SendUnMute(const UID &target) {
    if (!CheckForDevice()) {
      return;
    }

    auto_ptr<RDMRequest> request(
        ola::rdm::NewUnMuteRequest(m_our_uid, target, 0));

    unsigned int rdm_length = RDMCommandSerializer::RequiredSize(*request);
    uint8_t data[rdm_length];
    RDMCommandSerializer::Pack(*request, data, &rdm_length);
    m_device->SendMessage(
        target.IsBroadcast() ? JaRuleEndpoint::RDM_BROADCAST_REQUEST :
          JaRuleEndpoint::RDM_REQUEST,
        data, rdm_length);
  }

  void WriteLog() {
    if (!CheckForDevice()) {
      return;
    }

    std::ostringstream str;
    str << "Log Test " << m_log_count++ << ", this is quite long";
    const string payload = str.str();
    m_device->SendMessage(JaRuleEndpoint::WRITE_LOG,
                          reinterpret_cast<const uint8_t*>(payload.c_str()),
                          payload.size());
  }

  DISALLOW_COPY_AND_ASSIGN(InputHandler);
};

void ParseUID(const string &uid_str, UID *uid) {
  auto_ptr<UID> target_uid(UID::FromString(uid_str));
  if (!target_uid.get()) {
    OLA_WARN << "Invalid UID: '" << uid_str << "'";
    exit(ola::EXIT_USAGE);
  }
  *uid = *target_uid;
}

/*
 * Main.
 */
int main(int argc, char **argv) {
  ola::AppInit(&argc, argv, "[ options ]", "Ja Rule Admin Tool");

  UID target_uid(0, 0), controller_uid(0, 0), lower_uid(0, 0), upper_uid(0, 0);
  ParseUID(FLAGS_target_uid.str(), &target_uid);
  ParseUID(FLAGS_controller_uid.str(), &controller_uid);
  ParseUID(FLAGS_lower_uid.str(), &lower_uid);
  ParseUID(FLAGS_upper_uid.str(), &upper_uid);

  if (controller_uid.IsBroadcast()) {
    OLA_WARN << "The controller UID should not be a broadcast UID";
    exit(ola::EXIT_USAGE);
  }

  SelectServer ss;
  InputHandler input_handler(&ss, controller_uid, target_uid, lower_uid,
                             upper_uid);

  USBDeviceManager manager(
    &ss, NewCallback(&input_handler, &InputHandler::DeviceEvent));

  if (!manager.Start()) {
    exit(ola::EXIT_UNAVAILABLE);
  }

  input_handler.PrintCommands();

  ss.Run();
  return ola::EXIT_OK;
}
