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
#include <ola/base/Flags.h>
#include <ola/base/Init.h>
#include <ola/base/SysExits.h>
#include <ola/io/SelectServer.h>
#include <ola/io/StdinHandler.h>
#include <ola/network/MACAddress.h>
#include <ola/rdm/RDMCommand.h>
#include <ola/rdm/RDMCommandSerializer.h>
#include <ola/rdm/RDMPacket.h>
#include <ola/rdm/UID.h>
#include <ola/strings/Format.h>
#include <ola/util/Utils.h>
#include <ola/stl/STLUtils.h>

#include <algorithm>
#include <cctype>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "libs/usb/JaRuleWidget.h"
#include "tools/ja-rule/USBDeviceManager.h"

using ola::NewCallback;
using ola::NewSingleCallback;
using ola::STLFind;
using ola::STLReplace;
using ola::io::ByteString;
using ola::io::SelectServer;
using ola::io::StdinHandler;
using ola::rdm::RDMCommand;
using ola::rdm::RDMCommandSerializer;
using ola::rdm::RDMDiscoveryResponse;
using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;
using ola::rdm::RDMSetRequest;
using ola::rdm::RDMStatusCode;
using ola::rdm::UID;
using ola::strings::ToHex;
using ola::usb::CommandClass;
using ola::usb::JaRuleReturnCode;
using ola::usb::JaRuleWidget;
using ola::utils::JoinUInt8;
using ola::utils::SplitUInt16;
using std::auto_ptr;
using std::cout;
using std::endl;
using std::string;
using std::vector;

DEFINE_string(target_uid, "7a70:00000001",
              "The UID of the responder to control.");
DEFINE_string(lower_uid, "0000:00000000", "The lower UID for the DUB.");
DEFINE_string(upper_uid, "ffff:ffffffff", "The upper UID for the DUB.");
DEFINE_uint8(port, 0, "The port to control");

/**
 * @brief Wait on input from the keyboard, and based on the input, send
 * messages to the device.
 */
class Controller {
 public:
  enum TimingOption {
    TIMING_BREAK,
    TIMING_MARK,
    TIMING_RDM_RESPONSE_TIMEOUT,
    TIMING_RDM_BROADCAST_TIMEOUT,
    TIMING_DUB_RESPONSE_LIMIT,
    TIMING_RESPONDER_DELAY,
    TIMING_RESPONDER_JITTER,
  };

  Controller(SelectServer* ss, const UID &target_uid,
               const UID &lower_uid, const UID &upper_uid)
      : m_ss(ss),
        m_our_uid(0, 0),
        m_target_uid(target_uid),
        m_lower_uid(lower_uid),
        m_upper_uid(upper_uid),
        m_stdin_handler(
            new StdinHandler(ss, ola::NewCallback(this, &Controller::Input))),
        m_widget(NULL),
        m_log_count(0),
        m_dmx_slot_data(0),
        m_mode(MODE_DEFAULT),
        m_current_timing_option(TIMING_BREAK) {
    m_actions['\n'] = Action(NewCallback(this, &Controller::Commit));
    m_actions[27] = Action(NewCallback(this, &Controller::ExitEditMode));
    m_actions['+'] = Action(NewCallback(this, &Controller::Adjust, true));
    m_actions['-'] = Action(NewCallback(this, &Controller::Adjust, false));
    m_actions['0'] = Action("Send a 0 length DMX frame",
                            NewCallback(this, &Controller::SendZeroDMX));
    m_actions['2'] = Action("Send 2 DMX frames back to back",
                            NewCallback(this, &Controller::SendDoubleDMX));
    m_actions['d'] = Action(
        "Send a DUB frame from 0000:00000000 to ffff:ffffffff",
        NewCallback(this, &Controller::SendDUB, UID(0, 0),
                    UID::AllDevices()));
    m_actions['D'] = Action(
        "Send a DUB frame from --lower-uid to --upper-uid",
        NewCallback(this, &Controller::SendDUB, m_lower_uid, m_upper_uid));
    m_actions['e'] = Action("Send an echo command",
                            NewCallback(this, &Controller::SendEcho));
    m_actions['f'] = Action("Fetch the flags state",
                            NewCallback(this, &Controller::GetFlags));
    m_actions['h'] = Action("Display help",
                            NewCallback(this, &Controller::PrintCommands));
    m_actions['i'] = Action(
        "Identify on",
        NewCallback(this, &Controller::SendIdentify, true));
    m_actions['I'] = Action(
        "Identify off",
        NewCallback(this, &Controller::SendIdentify, false));
    m_actions['m'] = Action(
        "Send a broadcast mute",
        NewCallback(this, &Controller::SendMute, UID::AllDevices()));
    m_actions['M'] = Action(
        "Send a mute to the target UID",
        NewCallback(this, &Controller::SendMute, m_target_uid));
    m_actions['q'] = Action(
        "Quit",
        NewCallback(m_ss, &SelectServer::Terminate));
    m_actions['r'] = Action(
        "Reset the device",
        NewCallback(this, &Controller::ResetDevice));
    m_actions['t'] = Action(
        "Send a DMX frame",
        NewCallback(this, &Controller::SendDMX));
    m_actions['u'] = Action(
        "Send a broadcast unmute",
        NewCallback(this, &Controller::SendUnMute, UID::AllDevices()));
    m_actions['U'] = Action(
        "Send an unmute to the target UID",
        NewCallback(this, &Controller::SendUnMute, m_target_uid));
    m_actions['.'] = Action(
        "Get the hardware info",
        NewCallback(this, &Controller::GetHardwareInfo));
    m_actions[','] = Action(
        "Run the self test",
        NewCallback(this, &Controller::RunSelfTest));

    // Timing Options
    // For each of the options below, we allow a bigger range than the device
    // itself so we can test out-of-range errors.
    STLReplace(&m_timing_settings, TIMING_BREAK,
               TimingSetting('b', "break time", 176,
                             40,  // actual min is 44
                             1000,  // actual max is 800
                             TIMING_UNITS_MICROSECONDS,
                             ola::usb::JARULE_CMD_GET_BREAK_TIME,
                             ola::usb::JARULE_CMD_SET_BREAK_TIME));

    STLReplace(&m_timing_settings, TIMING_MARK,
               TimingSetting('x', "mark time", 12,
                             2,  // actual min is 4
                             850,  // actual max is 800
                             TIMING_UNITS_MICROSECONDS,
                             ola::usb::JARULE_CMD_GET_MARK_TIME,
                             ola::usb::JARULE_CMD_SET_MARK_TIME));

    STLReplace(&m_timing_settings, TIMING_RDM_RESPONSE_TIMEOUT,
               TimingSetting('y', "RDM response timeout", 28,
                             5,  // actual min 10
                             55,  // action max is 50
                             TIMING_UNITS_TENTHS_OF_MILLI_SECONDS,
                             ola::usb::JARULE_CMD_GET_RDM_RESPONSE_TIMEOUT,
                             ola::usb::JARULE_CMD_GET_RDM_RESPONSE_TIMEOUT));

    STLReplace(&m_timing_settings, TIMING_RDM_BROADCAST_TIMEOUT,
               TimingSetting('z', "RDM broadcast response timeout", 28,
                             5,  // actual min 10
                             55,  // action max is 50
                             TIMING_UNITS_TENTHS_OF_MILLI_SECONDS,
                             ola::usb::JARULE_CMD_GET_RDM_BROADCAST_TIMEOUT,
                             ola::usb::JARULE_CMD_SET_RDM_BROADCAST_TIMEOUT));

    TimingSettingMap::const_iterator iter = m_timing_settings.begin();
    for (; iter != m_timing_settings.end(); ++iter) {
      const TimingSetting &setting = iter->second;
      m_actions[setting.character_code] = Action(
          "Get " + setting.description,
          NewCallback(this, &Controller::GetTime, iter->first));
      m_actions[toupper(setting.character_code)] = Action(
          "Set " + setting.description,
          NewCallback(this, &Controller::EditTiming, iter->first));
    }
  }

  ~Controller() {
    m_stdin_handler.reset();

    ActionMap::iterator iter = m_actions.begin();
    for (; iter != m_actions.end(); ++iter) {
      delete iter->second.action;
    }
  }

  void WidgetEvent(USBDeviceManager::EventType event,
                   JaRuleWidget* widget) {
    if (event == USBDeviceManager::WIDGET_ADDED) {
      OLA_INFO << "Open Lighting Device added";
      if (!m_widget) {
        m_widget = widget;
        m_our_uid = widget->GetUID();

        // Switch to controller mode.
        uint8_t mode = ola::usb::CONTROLLER_MODE;
        m_widget->SendCommand(
            FLAGS_port,
            ola::usb::JARULE_CMD_SET_MODE,
            &mode, sizeof(mode), NULL);
      } else {
        OLA_WARN << "Only a single device is supported";
      }
    } else {
      OLA_INFO << "Open Lighting Device removed";
      if (widget == m_widget) {
        m_widget = NULL;
      }
    }
  }

  void Input(int c) {
    Action *action = STLFind(&m_actions, c);
    if (action) {
      action->action->Run();
    } else {
      switch (c) {
        case 27:  // Escape
          if (m_mode != MODE_DEFAULT) {
            cout << "Edit Aborted" << endl;
            m_mode = MODE_DEFAULT;
          }
          break;
        default:
          {}
      }
    }
  }

  void PrintCommands() {
    vector<string> lines;
    for (ActionMap::iterator iter = m_actions.begin();
         iter != m_actions.end(); ++iter) {
      if (!iter->second.description.empty() &&
          std::isprint(iter->first)) {
        std::ostringstream str;
        str << " " << iter->first << " - " << iter->second.description << endl;
        lines.push_back(str.str());
      }
    }

    std::sort(lines.begin(), lines.end());

    cout << "Commands:" << endl;
    for (vector<string>::iterator iter = lines.begin();
         iter != lines.end(); ++iter) {
      cout << *iter;
    }
  }

  void EditTiming(TimingOption option) {
    m_mode = MODE_EDIT_TIMING;
    m_current_timing_option = option;

    const TimingSetting* setting = STLFind(&m_timing_settings, option);
    if (!setting) {
      OLA_WARN << "Missing timing setting " << option;
      return;
    }
    cout << "Editing " << setting->description << ", currently "
         << FormatTime(setting->units, setting->current_value) << "." << endl
         << "Use +/- to adjust, Enter commits, Esc to abort" << endl;
  }

  void EchoCommandComplete(ola::usb::USBCommandResult result,
                           JaRuleReturnCode return_code, uint8_t status_flags,
                           const ByteString &payload) {
    if (!CheckResult(result, status_flags)) {
      return;
    }

    string response;
    if (!payload.empty()) {
       response.append(reinterpret_cast<const char*>(payload.data()),
                       payload.size());
    }
    cout << "Echo Reply: RC " << return_code << ": " << response << endl;
  }

  void AckCommandComplete(
      ola::usb::USBCommandResult result,
      JaRuleReturnCode return_code, uint8_t status_flags,
      const ByteString &payload) {
    if (!CheckResult(result, status_flags)) {
      return;
    }

    OLA_INFO << "RC: " << return_code << ", payload_size: " << payload.size();
  }

  void GetFlagsCommandComplete(
      ola::usb::USBCommandResult result,
      JaRuleReturnCode return_code, uint8_t status_flags,
      const ByteString &payload) {
    if (!CheckResult(result, status_flags)) {
      return;
    }

    OLA_INFO << "RC: " << return_code << ", payload_size: " << payload.size();
    if (!payload.empty()) {
      ola::strings::FormatData(&std::cout, payload.data(),
                               payload.size());
    }
  }

  void DUBCommandComplete(ola::usb::USBCommandResult result,
                          JaRuleReturnCode return_code, uint8_t status_flags,
                          const ByteString &payload) {
    if (!CheckResult(result, status_flags)) {
      return;
    }

    cout << "DUB Response: RC: " << return_code << ", size: "
         << payload.size() << endl;
  }

  void DisplayTime(TimingOption option,
                   ola::usb::USBCommandResult result,
                   JaRuleReturnCode return_code, uint8_t status_flags,
                   const ByteString &payload) {
    if (!CheckResult(result, status_flags)) {
      return;
    }

    if (return_code != ola::usb::RC_OK) {
      OLA_INFO << "RC: " << return_code << ", payload_size: "
               << payload.size();
      return;
    }

    const TimingSetting* setting = STLFind(&m_timing_settings, option);
    if (!setting) {
      OLA_WARN << "Missing timing setting " << option;
      return;
    }

    uint16_t value = 0;
    if (payload.size() != sizeof(value)) {
      OLA_WARN << "Payload size mismatch";
      return;
    }

    value = JoinUInt8(payload[1], payload[0]);

    string description = setting->description;
    ola::CapitalizeFirst(&description);
    cout << description << ": " << FormatTime(setting->units, value) << endl;
  }

  void CommandComplete(ola::usb::USBCommandResult result,
                       JaRuleReturnCode return_code, uint8_t status_flags,
                       const ByteString &payload) {
    if (!CheckResult(result, status_flags)) {
      return;
    }

    OLA_INFO << "RC: " << return_code << ", payload_size: " << payload.size();

    if (!payload.empty()) {
      return;
    }

    if (payload[0] == ola::rdm::START_CODE) {
      RDMStatusCode status_code;
      // Skip over the start code.
      auto_ptr<RDMResponse> response(RDMResponse::InflateFromData(
          payload.data() + 1, payload.size() - 1, &status_code));

      if (!response.get()) {
        OLA_WARN << "Failed to inflate RDM response";
        if (!payload.empty()) {
          ola::strings::FormatData(&std::cout, payload.data(), payload.size());
        }
      }
      OLA_INFO << *response;
    }
  }

  void HardwareInfoComplete(
      ola::usb::USBCommandResult result,
      JaRuleReturnCode return_code, uint8_t status_flags,
      const ByteString &payload) {
    if (!CheckResult(result, status_flags)) {
      return;
    }

    OLA_INFO << "RC: " << return_code << ", payload_size: " << payload.size();
    if (payload.size() >= 14) {
      uint16_t model_id = JoinUInt8(payload[1], payload[0]);
      UID uid(payload.data() + sizeof(uint16_t));
      ola::network::MACAddress mac_address(
          payload.data() + sizeof(uint16_t) + UID::LENGTH);
      cout << "Model: " << model_id << ", UID: " << uid << ", MAC: "
           << mac_address << endl;
    } else {
      OLA_WARN << "Received " << payload.size() << " bytes, expecting 14";
    }
  }

  void SelfTestPart2Complete(ola::usb::USBCommandResult result,
                             JaRuleReturnCode return_code, uint8_t status_flags,
                             OLA_UNUSED const ByteString &payload) {
    if (!CheckResult(result, status_flags)) {
      return;
    }

    cout << "Test result: " << return_code << endl;
  }

  void SelfTestPart1Complete(ola::usb::USBCommandResult result,
                             JaRuleReturnCode return_code, uint8_t status_flags,
                             OLA_UNUSED const ByteString &payload) {
    if (!CheckResult(result, status_flags)) {
      return;
    }
    if (return_code == ola::usb::RC_OK) {
      m_widget->SendCommand(
          FLAGS_port, ola::usb::JARULE_CMD_RUN_SELF_TEST,
          NULL, 0,
          NewSingleCallback(this, &Controller::SelfTestPart2Complete));
    } else {
      OLA_WARN << "Unable to change to self test mode";
    }
  }

 private:
  enum Mode {
    MODE_DEFAULT,
    MODE_EDIT_TIMING,
  };

  typedef enum {
    TIMING_UNITS_MICROSECONDS,
    TIMING_UNITS_TENTHS_OF_MILLI_SECONDS,
  } TimingUnit;

  struct Action {
   public:
    typedef ola::Callback0<void> ActionCallback;

    Action() : action(NULL) {}
    explicit Action(ActionCallback *cb) : action(cb) {}
    Action(const string &description, ActionCallback *cb)
       : description(description), action(cb) {}

    string description;
    ActionCallback *action;
  };

  typedef std::map<char, Action> ActionMap;

  struct TimingSetting {
   public:
     TimingSetting(char character_code,
                   string description,
                   uint16_t initial_value,
                   uint16_t min_value,
                   uint16_t max_value,
                   TimingUnit units,
                   CommandClass get_command,
                   CommandClass set_command)
       : character_code(character_code),
         description(description),
         current_value(initial_value),
         min_value(min_value),
         max_value(max_value),
         units(units),
         get_command(get_command),
         set_command(set_command) {
    }

    char character_code;
    string description;
    uint16_t current_value;
    uint16_t min_value;
    uint16_t max_value;
    TimingUnit units;
    CommandClass get_command;
    CommandClass set_command;
  };

  typedef std::map<TimingOption, TimingSetting> TimingSettingMap;

  ActionMap m_actions;
  TimingSettingMap m_timing_settings;

  SelectServer* m_ss;
  UID m_our_uid, m_target_uid, m_lower_uid, m_upper_uid;
  auto_ptr<StdinHandler> m_stdin_handler;
  JaRuleWidget* m_widget;
  unsigned int m_log_count;
  uint8_t m_dmx_slot_data;
  Mode m_mode;
  TimingOption m_current_timing_option;

  bool CheckForWidget() const {
    if (!m_widget) {
      cout << "Device not present or device unavailable" << endl;
      return false;
    }
    return true;
  }

  void ResetDevice() {
    if (!CheckForWidget()) {
      return;
    }

    m_widget->SendCommand(
        FLAGS_port,
        ola::usb::JARULE_CMD_RESET_DEVICE,
        NULL, 0,
        NewSingleCallback(this, &Controller::AckCommandComplete));
  }

  void GetFlags() {
    if (!CheckForWidget()) {
      return;
    }

    m_widget->SendCommand(
        FLAGS_port,
        ola::usb::JARULE_CMD_GET_FLAGS,
        NULL, 0,
        NewSingleCallback(this, &Controller::GetFlagsCommandComplete));
  }

  void _SendDMX(const uint8_t *data, unsigned int size) {
    if (!CheckForWidget()) {
      return;
    }
    m_widget->SendCommand(
        FLAGS_port,
        ola::usb::JARULE_CMD_TX_DMX,
        data, size,
        NewSingleCallback(this, &Controller::CommandComplete));
  }

  void Adjust(bool increase) {
    TimingSetting* setting = STLFind(&m_timing_settings,
                                     m_current_timing_option);
    if (!setting) {
      OLA_WARN << "Missing timing setting " << m_current_timing_option;
      return;
    }

    if (increase) {
      setting->current_value++;
      if (setting->current_value > setting->max_value) {
        setting->current_value = setting->max_value;
      }
    } else {
      setting->current_value--;
      if (setting->current_value < setting->min_value) {
        setting->current_value = setting->min_value;
      }
    }

    string description = setting->description;
    ola::CapitalizeFirst(&description);
    cout << description << " is now "
         << FormatTime(setting->units, setting->current_value) << endl;
  }

  void Commit() {
    if (!CheckForWidget()) {
      return;
    }

    if (m_mode == MODE_DEFAULT) {
      return;
    }

    uint8_t payload[2];
    const TimingSetting* setting = STLFind(
        &m_timing_settings, m_current_timing_option);
    if (!setting) {
      OLA_WARN << "Missing timing setting " << m_current_timing_option;
      return;
    }
    SplitUInt16(setting->current_value, &payload[1], &payload[0]);

    m_widget->SendCommand(
        FLAGS_port, setting->set_command,
        payload, arraysize(payload),
        NewSingleCallback(this, &Controller::AckCommandComplete));
    m_mode = MODE_DEFAULT;
  }

  void ExitEditMode() {
    if (m_mode != MODE_DEFAULT) {
      cout << "Edit aborted" << endl;
      m_mode = MODE_DEFAULT;
    }
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
    if (!CheckForWidget()) {
      return;
    }

    uint8_t payload[512];
    memset(payload, 0, 512);
    payload[0] = m_dmx_slot_data;
    _SendDMX(payload, arraysize(payload));
    m_dmx_slot_data += 16;
  }

  void SendEcho() {
    if (!CheckForWidget()) {
      return;
    }

    const char payload[] = "echo test";
    m_widget->SendCommand(
        FLAGS_port,
        ola::usb::JARULE_CMD_ECHO,
        reinterpret_cast<const uint8_t*>(payload),
        arraysize(payload),
        NewSingleCallback(this, &Controller::EchoCommandComplete));
  }

  void GetTime(TimingOption option) {
    if (!CheckForWidget()) {
      return;
    }

    const TimingSetting* setting = STLFind(&m_timing_settings, option);
    if (!setting) {
      OLA_WARN << "Missing timing setting " << option;
      return;
    }
    m_widget->SendCommand(
        FLAGS_port,
        setting->get_command, NULL, 0,
        NewSingleCallback(this, &Controller::DisplayTime, option));
  }

  void SendDUB(UID lower, UID upper) {
    if (!CheckForWidget()) {
      return;
    }

    auto_ptr<RDMRequest> request(
        ola::rdm::NewDiscoveryUniqueBranchRequest(m_our_uid, lower, upper, 0));
    unsigned int rdm_length = RDMCommandSerializer::RequiredSize(*request);
    uint8_t data[rdm_length];
    RDMCommandSerializer::Pack(*request, data, &rdm_length);
    OLA_INFO << "Sending " << rdm_length << " byte RDM command.";
    m_widget->SendCommand(
        FLAGS_port,
        ola::usb::JARULE_CMD_RDM_DUB_REQUEST,
        data, rdm_length,
        NewSingleCallback(this, &Controller::DUBCommandComplete));
  }

  void SendIdentify(bool identify_on) {
    if (!CheckForWidget()) {
      return;
    }

    uint8_t param_data = identify_on;
    RDMSetRequest request(m_our_uid, m_target_uid, 0, 0, 0,
                          ola::rdm::PID_IDENTIFY_DEVICE, &param_data,
                          sizeof(param_data));

    unsigned int rdm_length = RDMCommandSerializer::RequiredSize(request);
    uint8_t data[rdm_length];
    RDMCommandSerializer::Pack(request, data, &rdm_length);
    m_widget->SendCommand(
        FLAGS_port,
        ola::usb::JARULE_CMD_RDM_REQUEST,
        data, rdm_length,
        NewSingleCallback(this, &Controller::CommandComplete));
  }

  void SendMute(UID target) {
    if (!CheckForWidget()) {
      return;
    }

    auto_ptr<RDMRequest> request(
        ola::rdm::NewMuteRequest(m_our_uid, target, 0));

    CommandClass command_class = target.IsBroadcast() ?
        ola::usb::JARULE_CMD_RDM_BROADCAST_REQUEST :
        ola::usb::JARULE_CMD_RDM_REQUEST;
    unsigned int rdm_length = RDMCommandSerializer::RequiredSize(*request);
    uint8_t data[rdm_length];
    RDMCommandSerializer::Pack(*request, data, &rdm_length);
    m_widget->SendCommand(
        FLAGS_port, command_class,
        data, rdm_length,
        NewSingleCallback(this, &Controller::CommandComplete));
  }

  void SendUnMute(UID target) {
    if (!CheckForWidget()) {
      return;
    }

    auto_ptr<RDMRequest> request(
        ola::rdm::NewUnMuteRequest(m_our_uid, target, 0));

    CommandClass command_class = target.IsBroadcast() ?
        ola::usb::JARULE_CMD_RDM_BROADCAST_REQUEST :
        ola::usb::JARULE_CMD_RDM_REQUEST;

    unsigned int rdm_length = RDMCommandSerializer::RequiredSize(*request);
    uint8_t data[rdm_length];
    RDMCommandSerializer::Pack(*request, data, &rdm_length);
    m_widget->SendCommand(
        FLAGS_port, command_class,
        data, rdm_length,
        NewSingleCallback(this, &Controller::CommandComplete));
  }

  void GetHardwareInfo() {
    if (!CheckForWidget()) {
      return;
    }
    m_widget->SendCommand(
        FLAGS_port, ola::usb::JARULE_CMD_GET_HARDWARE_INFO,
        NULL, 0,
        NewSingleCallback(this, &Controller::HardwareInfoComplete));
  }

  void RunSelfTest() {
    if (!CheckForWidget()) {
      return;
    }
    uint8_t mode = ola::usb::SELF_TEST_MODE;
    m_widget->SendCommand(
        FLAGS_port,
        ola::usb::JARULE_CMD_SET_MODE,
        &mode, sizeof(mode),
        NewSingleCallback(this, &Controller::SelfTestPart1Complete));
  }

  bool CheckResult(ola::usb::USBCommandResult result,
                   uint8_t status_flags) {
    if (result != ola::usb::COMMAND_RESULT_OK) {
      OLA_WARN << "Error: " << result;
      return false;
    }
    if (status_flags & ola::usb::FLAGS_CHANGED_FLAG) {
      OLA_INFO << "Flags changed!";
    }
    if (status_flags & ola::usb::MSG_TRUNCATED_FLAG) {
      OLA_INFO << "Message truncated";
    }
    return true;
  }

  string FormatTime(TimingUnit units, uint16_t value) {
    std::stringstream str;
    if (units == TIMING_UNITS_MICROSECONDS) {
      str << value << " us";
    } else if (units == TIMING_UNITS_TENTHS_OF_MILLI_SECONDS) {
      float adjusted_time = value / 10.0;
      str << adjusted_time << " ms";
    }
    return str.str();
  }

  DISALLOW_COPY_AND_ASSIGN(Controller);
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

  UID target_uid(0, 0), lower_uid(0, 0), upper_uid(0, 0);
  ParseUID(FLAGS_target_uid.str(), &target_uid);
  ParseUID(FLAGS_lower_uid.str(), &lower_uid);
  ParseUID(FLAGS_upper_uid.str(), &upper_uid);

  SelectServer ss;
  Controller controller(&ss, target_uid, lower_uid, upper_uid);

  USBDeviceManager manager(
    &ss, NewCallback(&controller, &Controller::WidgetEvent));

  if (!manager.Start()) {
    exit(ola::EXIT_UNAVAILABLE);
  }

  // Print this via cout to ensure we actually get some output by default
  cout << "Press h to print a help message" << endl;

  ss.Run();
  return ola::EXIT_OK;
}
