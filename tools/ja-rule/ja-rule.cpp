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
#include <ola/base/SysExits.h>
#include <ola/io/SelectServer.h>
#include <ola/io/StdinHandler.h>
#include <ola/rdm/RDMCommand.h>
#include <ola/rdm/RDMCommandSerializer.h>
#include <ola/rdm/UID.h>
#include <ola/strings/Format.h>
#include <ola/thread/Thread.h>

#include <iostream>
#include <memory>
#include <string>

#include "tools/ja-rule/OpenLightingDevice.h"
#include "tools/ja-rule/USBDeviceManager.h"

using ola::NewCallback;
using ola::io::SelectServer;
using ola::io::StdinHandler;
using ola::rdm::RDMCommandSerializer;
using ola::rdm::RDMDiscoveryResponse;
using ola::rdm::RDMRequest;
using ola::rdm::UID;
using ola::strings::ToHex;
using ola::thread::Thread;
using std::auto_ptr;
using std::cout;
using std::endl;
using std::string;

/**
 * @brief Print messages received from the device.
 */
class MessageHandler : public MessageHandlerInterface {
 public:
  MessageHandler() {}

  void NewMessage(const Message& message) {
    switch (message.command) {
      case OpenLightingDevice::ECHO_COMMAND:
        PrintEcho(message);
        break;
      case OpenLightingDevice::TX_DMX:
        PrintResponse(message);
        break;
      case OpenLightingDevice::GET_LOG:
        PrintLog(message.payload, message.payload_size);
        break;
      case OpenLightingDevice::GET_FLAGS:
        PrintFlags(message);
        break;
      case OpenLightingDevice::WRITE_LOG:
        PrintAck(message);
        break;
      case OpenLightingDevice::RESET_DEVICE:
        PrintAck(message);
        break;
      case OpenLightingDevice::RDM_DUB:
        PrintDUBResponse(message);
        break;
      case OpenLightingDevice::RDM_REQUEST:
        PrintResponse(message);
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
    OLA_INFO << "Got response of size " << message.payload_size;
  }

  void PrintResponse(const Message& message) {
    OLA_INFO << "RC (" << static_cast<int>(message.return_code)
             << "): payload_size: " << message.payload_size;
    if (message.payload && message.payload_size) {
      ola::strings::FormatData(&std::cout, message.payload,
                               message.payload_size);
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
  explicit InputHandler(SelectServer* ss)
      : m_ss(ss),
        m_stdin_handler(
            new StdinHandler(ss, ola::NewCallback(this, &InputHandler::Input))),
        m_device(NULL),
        m_log_count(0),
        m_dmx_slot_data(0),
        m_our_uid(ola::OPEN_LIGHTING_ESTA_CODE, 10) {
  }

  ~InputHandler() {
    m_stdin_handler.reset();
  }

  void DeviceEvent(USBDeviceManager::EventType event,
                   OpenLightingDevice* device) {
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
      case '0':
        SendZeroDMX();
        break;
      case '2':
        SendDoubleDMX();
        break;
      case 'd':
        SendDUB();
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
      case 'l':
        GetLogs();
        break;
      case 'r':
        ResetDevice();
        break;
      case 't':
        SendDMX();
        break;
      case 'u':
        SendUnMute();
        break;
      case 'q':
        m_ss->Terminate();
        break;
      case 'w':
        WriteLog();
        break;
      default:
        {}
    }
  }

  void PrintCommands() {
    cout << "Commands:" << endl;
    cout << " 0 - Send a 0 length DMX frame" << endl;
    cout << " 2 - Send 2 DMX frames back to back" << endl;
    cout << " d - Send a DUB frame" << endl;
    cout << " e - Send Echo command" << endl;
    cout << " f - Fetch Flags State" << endl;
    cout << " h - Print this help message" << endl;
    cout << " l - Fetch Logs" << endl;
    cout << " q - Quit" << endl;
    cout << " r - Reset" << endl;
    cout << " t - Send DMX frame" << endl;
    cout << " u - Send an UnMute-all frame" << endl;
    cout << " w - Write Log" << endl;
  }

 private:
  SelectServer* m_ss;
  auto_ptr<StdinHandler> m_stdin_handler;
  MessageHandler m_handler;
  OpenLightingDevice* m_device;
  unsigned int m_log_count;
  uint8_t m_dmx_slot_data;
  UID m_our_uid;

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

    m_device->SendMessage(OpenLightingDevice::GET_LOG, NULL, 0);
  }

  void ResetDevice() {
    if (!CheckForDevice()) {
      return;
    }

    m_device->SendMessage(OpenLightingDevice::RESET_DEVICE, NULL, 0);
  }

  void GetFlags() {
    if (!CheckForDevice()) {
      return;
    }

    m_device->SendMessage(OpenLightingDevice::GET_FLAGS, NULL, 0);
  }

  void _SendDMX(const uint8_t *data, unsigned int size) {
    if (!CheckForDevice()) {
      return;
    }
    m_device->SendMessage(OpenLightingDevice::TX_DMX, data, size);
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
    m_device->SendMessage(OpenLightingDevice::ECHO_COMMAND, payload,
                          arraysize(payload));
  }

  void SendDUB() {
    if (!CheckForDevice()) {
      return;
    }

    auto_ptr<RDMRequest> request(
        ola::rdm::NewDiscoveryUniqueBranchRequest(m_our_uid, UID(0, 0),
                                                  UID::AllDevices(), 0));
    unsigned int rdm_length = RDMCommandSerializer::RequiredSize(*request);
    uint8_t data[rdm_length];
    RDMCommandSerializer::Pack(*request, data, &rdm_length);
    OLA_INFO << "Sending " << rdm_length << " RDM command.";
    m_device->SendMessage(OpenLightingDevice::RDM_DUB, data, rdm_length);
  }

  void SendUnMute() {
    if (!CheckForDevice()) {
      return;
    }

    auto_ptr<RDMRequest> request(
        ola::rdm::NewUnMuteRequest(m_our_uid, UID::AllDevices(), 0));

    unsigned int rdm_length = RDMCommandSerializer::RequiredSize(*request);
    uint8_t data[rdm_length];
    RDMCommandSerializer::Pack(*request, data, &rdm_length);
    m_device->SendMessage(OpenLightingDevice::RDM_REQUEST, data, rdm_length);
  }

  void WriteLog() {
    if (!CheckForDevice()) {
      return;
    }

    std::ostringstream str;
    str << "Log Test " << m_log_count++ << ", this is quite long";
    const string payload = str.str();
    m_device->SendMessage(OpenLightingDevice::WRITE_LOG,
                          reinterpret_cast<const uint8_t*>(payload.c_str()),
                          payload.size());
  }

  DISALLOW_COPY_AND_ASSIGN(InputHandler);
};

/*
 * Main.
 */
int main(int argc, char **argv) {
  ola::AppInit(&argc, argv, "[ options ]", "Ja Rule Admin Tool");

  SelectServer ss;
  InputHandler input_handler(&ss);

  USBDeviceManager manager(
    &ss, NewCallback(&input_handler, &InputHandler::DeviceEvent));

  if (!manager.Start()) {
    exit(ola::EXIT_UNAVAILABLE);
  }

  input_handler.PrintCommands();

  ss.Run();
  return ola::EXIT_OK;
}
