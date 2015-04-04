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
 * ja-rule-controller.cpp
 * A basic RDM controller that uses the Ja Rule interface.
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
#include <ola/rdm/DiscoveryAgent.h>
#include <ola/rdm/QueueingRDMController.h>
#include <ola/rdm/RDMCommand.h>
#include <ola/rdm/RDMCommandSerializer.h>
#include <ola/rdm/RDMControllerInterface.h>
#include <ola/rdm/UID.h>
#include <ola/util/Utils.h>
#include <ola/util/SequenceNumber.h>

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "tools/ja-rule/OpenLightingDevice.h"
#include "tools/ja-rule/USBDeviceManager.h"

using ola::NewCallback;
using ola::NewSingleCallback;
using ola::io::SelectServer;
using ola::io::StdinHandler;
using ola::rdm::DiscoveryAgent;
using ola::rdm::RDMCommand;
using ola::rdm::RDMCommandSerializer;
using ola::rdm::RDMDiscoveryCallback;
using ola::rdm::RDMDiscoveryResponse;
using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;
using ola::rdm::RDMSetRequest;
using ola::rdm::DiscoverableQueueingRDMController;
using ola::rdm::UID;
using ola::rdm::UIDSet;
using ola::rdm::rdm_response_code;
using ola::strings::ToHex;
using std::auto_ptr;
using std::cerr;
using std::cout;
using std::endl;
using std::string;

DEFINE_string(controller_uid, "7a70:fffffe00", "The UID of the controller.");

class ControllerImpl : public ola::rdm::DiscoveryTargetInterface,
                       public ola::rdm::DiscoverableRDMControllerInterface,
                       public MessageHandlerInterface {
 public:
  ControllerImpl(OpenLightingDevice *device,
                 const UID &controller_uid)
      : m_device(device),
        m_discovery_agent(this),
        m_our_uid(controller_uid),
        m_rdm_callback(NULL),
        m_mute_callback(NULL),
        m_unmute_callback(NULL),
        m_branch_callback(NULL) {
    device->SetHandler(this);
  }

  ~ControllerImpl() {
    m_discovery_agent.Abort();
    m_device->SetHandler(NULL);
  }

  void UpdateDevice(OpenLightingDevice* device) {
    m_device = device;
  }

  void RunFullDiscovery(RDMDiscoveryCallback *callback) {
    OLA_INFO << "Full discovery triggered";
    m_discovery_agent.StartFullDiscovery(
        NewSingleCallback(this, &ControllerImpl::DiscoveryComplete,
        callback));
  }

  void RunIncrementalDiscovery(RDMDiscoveryCallback *callback) {
    OLA_INFO << "Incremental discovery triggered";
    m_discovery_agent.StartIncrementalDiscovery(
        NewSingleCallback(this, &ControllerImpl::DiscoveryComplete,
        callback));
  }


  void SendRDMRequest(const RDMRequest *request,
                      ola::rdm::RDMCallback *on_complete) {
    if (!CheckForDevice()) {
      const std::vector<std::string> packets;
      on_complete->Run(ola::rdm::RDM_FAILED_TO_SEND, NULL, packets);
      return;
    }

    m_rdm_callback = on_complete;

    unsigned int rdm_length = RDMCommandSerializer::RequiredSize(*request);
    uint8_t data[rdm_length];
    RDMCommandSerializer::Pack(*request, data, &rdm_length);
    m_device->SendMessage(OpenLightingDevice::RDM_REQUEST, data, rdm_length);
  }

  void MuteDevice(const UID &target,
                  MuteDeviceCallback *mute_complete) {
    if (!CheckForDevice()) {
      mute_complete->Run(false);
      return;
    }

    auto_ptr<RDMRequest> request(
        ola::rdm::NewMuteRequest(m_our_uid, target,
                                 m_transaction_number.Next()));

    unsigned int rdm_length = RDMCommandSerializer::RequiredSize(*request);
    uint8_t data[rdm_length];
    RDMCommandSerializer::Pack(*request, data, &rdm_length);
    m_device->SendMessage(OpenLightingDevice::RDM_REQUEST, data, rdm_length);

    m_mute_callback = mute_complete;
  }

  void UnMuteAll(UnMuteDeviceCallback *unmute_complete) {
    if (!CheckForDevice()) {
      unmute_complete->Run();
      return;
    }

    auto_ptr<RDMRequest> request(
        ola::rdm::NewUnMuteRequest(m_our_uid, UID::AllDevices(),
                                   m_transaction_number.Next()));

    unsigned int rdm_length = RDMCommandSerializer::RequiredSize(*request);
    uint8_t data[rdm_length];
    RDMCommandSerializer::Pack(*request, data, &rdm_length);
    m_device->SendMessage(OpenLightingDevice::RDM_REQUEST, data, rdm_length);

    m_unmute_callback = unmute_complete;
  }

  void Branch(const UID &lower,
              const UID &upper,
              BranchCallback *branch_complete) {
    if (!CheckForDevice()) {
      branch_complete->Run(NULL, 0);
      return;
    }

    auto_ptr<RDMRequest> request(
        ola::rdm::NewDiscoveryUniqueBranchRequest(m_our_uid, lower, upper,
                                                  m_transaction_number.Next()));
    unsigned int rdm_length = RDMCommandSerializer::RequiredSize(*request);
    uint8_t data[rdm_length];
    RDMCommandSerializer::Pack(*request, data, &rdm_length);
    OLA_INFO << "Sending " << rdm_length << " RDM command: " << lower << " - "
             << upper;
    m_device->SendMessage(OpenLightingDevice::RDM_DUB, data, rdm_length);

    m_branch_callback = branch_complete;
  }

  void ResetDevice() {
    m_device->SendMessage(OpenLightingDevice::RESET_DEVICE, NULL, 0);
  }

  void NewMessage(const Message& message) {
    OLA_INFO << "Got message with command "
             << static_cast<int>(message.command);

    switch (message.command) {
      case OpenLightingDevice::RDM_DUB:
        HandleDUBResponse(message);
        break;
      case OpenLightingDevice::RDM_REQUEST:
        HandleRDM(message);
        break;
      case OpenLightingDevice::RESET_DEVICE:
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
  OpenLightingDevice *m_device;
  DiscoveryAgent m_discovery_agent;
  const UID m_our_uid;
  ola::SequenceNumber<uint8_t> m_transaction_number;
  ola::rdm::RDMCallback *m_rdm_callback;
  MuteDeviceCallback *m_mute_callback;
  UnMuteDeviceCallback *m_unmute_callback;
  BranchCallback *m_branch_callback;
  UIDSet m_uids;

  bool CheckForDevice() const {
    if (!m_device) {
      OLA_INFO << "Device not present";
    }
    return m_device != NULL;
  }

  void PrintAck(const Message& message) {
    OLA_INFO << "ACK (" << static_cast<int>(message.return_code)
             << "): payload_size: " << message.payload_size;
  }

  void HandleDUBResponse(const Message& message) {
    if (m_branch_callback) {
      const uint8_t *data = NULL;
      unsigned int size = 0;
      if (message.payload && message.payload_size > 1) {
        data = message.payload + 1;
        size = message.payload_size - 1;
      }
      BranchCallback *callback = m_branch_callback;
      m_branch_callback = NULL;
      callback->Run(data, size);
    }
  }

  void HandleRDM(const Message& message) {
    if (m_unmute_callback) {
      UnMuteDeviceCallback *callback = m_unmute_callback;
      m_unmute_callback = NULL;
      callback->Run();
      return;
    }

    if (m_mute_callback) {
      // TODO(simon): inflate the actual RDM response here. Right now we treat
      // any response as good.
      bool ok = message.payload_size > 1;
      MuteDeviceCallback *callback = m_mute_callback;
      m_mute_callback = NULL;
      callback->Run(ok);
    }
  }

  void DiscoveryComplete(RDMDiscoveryCallback *callback,
                         OLA_UNUSED bool ok,
                         const UIDSet& uids) {
    OLA_DEBUG << "Discovery complete: " << uids;
    m_uids = uids;
    if (callback) {
      callback->Run(m_uids);
    }
  }

  DISALLOW_COPY_AND_ASSIGN(ControllerImpl);
};

class Controller : public ola::rdm::DiscoverableRDMControllerInterface {
 public:
  Controller(OpenLightingDevice *device, const UID &controller_uid)
    : m_controller_impl(device, controller_uid),
      m_queueing_controller(&m_controller_impl, 50) {
  }

  ~Controller() {}

  void UpdateDevice(OpenLightingDevice* device) {
    m_controller_impl.UpdateDevice(device);
  }

  void SendRDMRequest(const RDMRequest *request,
                      ola::rdm::RDMCallback *on_complete) {
    m_queueing_controller.SendRDMRequest(request, on_complete);
  }

  void RunFullDiscovery(RDMDiscoveryCallback *callback) {
    m_queueing_controller.RunFullDiscovery(callback);
  }

  void RunIncrementalDiscovery(RDMDiscoveryCallback *callback) {
    m_queueing_controller.RunIncrementalDiscovery(callback);
  }

  void ResetDevice() {
    m_controller_impl.ResetDevice();
  }

 private:
  ControllerImpl m_controller_impl;
  DiscoverableQueueingRDMController m_queueing_controller;

  DISALLOW_COPY_AND_ASSIGN(Controller);
};

class DeviceManager {
 public:
  explicit DeviceManager(const UID &controller_uid)
      : m_device(NULL),
        m_controller_uid(controller_uid) {
  }

  void DeviceEvent(USBDeviceManager::EventType event,
                   OpenLightingDevice* device) {
    if (event == USBDeviceManager::DEVICE_ADDED) {
      OLA_INFO << "Open Lighting Device added";
      if (m_device) {
        // We only support a single device for now
        OLA_WARN << "More than one device present";
        return;
      }
      m_device = device;
      m_controller.reset(new Controller(device, m_controller_uid));
    } else {
      OLA_INFO << "Open Lighting Device removed";
      if (device == m_device) {
        m_controller.reset();
        m_device = NULL;
      }
    }
  }

  // Only valid for the lifetime of the select server event.
  Controller* GetController() {
    return m_controller.get();
  }

 private:
  OpenLightingDevice *m_device;
  const UID m_controller_uid;
  std::auto_ptr<Controller> m_controller;

  DISALLOW_COPY_AND_ASSIGN(DeviceManager);
};


/**
 * @brief Wait on input from the keyboard, and based on the input, send
 * messages to the device.
 */
class InputHandler {
 public:
  InputHandler(SelectServer* ss,
               const UID &controller_uid,
               DeviceManager *device_manager)
      : m_ss(ss),
        m_device_manager(device_manager),
        m_controller_uid(controller_uid),
        m_stdin_handler(
            new StdinHandler(ss,
                             ola::NewCallback(this, &InputHandler::Input))),
        m_mode(NORMAL),
        m_selected_uid(0, 0) {
  }

  ~InputHandler() {
    m_stdin_handler.reset();
  }

  void Input(int c) {
    if (m_mode == SELECT_UID) {
      UIDSet::Iterator iter = m_uids.Begin();
      char index = 'A';
      for (; iter != m_uids.End() && index <= 'Z'; ++iter, index++) {
        if (c == index) {
          m_selected_uid = *iter;
          cout << "Selected " << *iter << endl;
          m_mode = NORMAL;
          return;
        }
      }
      cerr << "Unknown selection, try again" << endl;
      return;
    }

    switch (c) {
      case 'i':
        SetIdentify(true);
        break;
      case 'I':
        SetIdentify(false);
        break;
      case 'd':
        RunDiscovery(false);
        break;
      case 'h':
        PrintCommands();
        break;
      case 'p':
        RunDiscovery(true);
        break;
      case 'q':
        m_ss->Terminate();
        break;
      case 'r':
        ResetDevice();
        break;
      case 's':
        cout << "Enter a letter for the UID" << endl;
        m_mode = SELECT_UID;
        break;
      case 'u':
        ShowUIDs();
        break;
      default:
        {}
    }
  }

  void PrintCommands() {
    cout << "Commands:" << endl;
    cout << " i - Identify On" << endl;
    cout << " I - Identify Off" << endl;
    cout << " d - Run Full Discovery" << endl;
    cout << " h - Print this help message" << endl;
    cout << " p - Run Incremental Discovery" << endl;
    cout << " q - Quit" << endl;
    cout << " r - Reset" << endl;
    cout << " s - Select UID" << endl;
    cout << " u - Show UIDs" << endl;
  }

 private:
  typedef enum {
    NORMAL,
    SELECT_UID,
  } Mode;

  SelectServer* m_ss;
  DeviceManager *m_device_manager;
  const UID m_controller_uid;
  auto_ptr<StdinHandler> m_stdin_handler;
  UIDSet m_uids;
  Mode m_mode;
  UID m_selected_uid;

  unsigned int m_log_count;

  void SetIdentify(bool identify_on) {
    Controller *controller = m_device_manager->GetController();
    if (!controller) {
      return;
    }

    if (m_uids.Size() == 0) {
      OLA_WARN << "No UIDs";
      return;
    }

    uint8_t param_data = identify_on;
    RDMSetRequest *request = new RDMSetRequest(
        m_controller_uid, m_selected_uid, 0, 0, 0, 0,
        ola::rdm::PID_IDENTIFY_DEVICE, &param_data,
        sizeof(param_data));
    controller->SendRDMRequest(request, NULL);
  }

  void RunDiscovery(bool incremental) {
    Controller *controller = m_device_manager->GetController();
    if (!controller) {
      return;
    }
    if (incremental) {
      controller->RunIncrementalDiscovery(
          NewSingleCallback(this, &InputHandler::DiscoveryComplete));
    } else {
      controller->RunFullDiscovery(
          NewSingleCallback(this, &InputHandler::DiscoveryComplete));
    }
  }

  void ResetDevice() {
    Controller *controller = m_device_manager->GetController();
    if (!controller) {
      return;
    }
    OLA_INFO << "Resetting device";
    controller->ResetDevice();
  }

  void DiscoveryComplete(const UIDSet& uids) {
    m_uids = uids;
    ShowUIDs();
  }

  void ShowUIDs() {
    UIDSet::Iterator iter = m_uids.Begin();
    cout << "---------- " << m_uids.Size() << " UIDs -------" << endl;
    char c = 'A';
    for (; iter != m_uids.End(); ++iter) {
      if (c <= 'Z') {
        cout << *iter << " (" << c++ << ")" << endl;
      } else {
        cout << *iter << endl;
      }
    }
    cout << "-------------------------" << endl;
  }

  DISALLOW_COPY_AND_ASSIGN(InputHandler);
};

/*
 * Main.
 */
int main(int argc, char **argv) {
  ola::AppInit(&argc, argv, "[ options ]", "Ja Rule Admin Tool");

  auto_ptr<UID> controller_uid(UID::FromString(FLAGS_controller_uid));
  if (!controller_uid.get()) {
    OLA_WARN << "Invalid Controller UID: '" << FLAGS_controller_uid << "'";
    exit(ola::EXIT_USAGE);
  }

  if (controller_uid->IsBroadcast()) {
    OLA_WARN << "The controller UID should not be a broadcast UID";
    exit(ola::EXIT_USAGE);
  }

  SelectServer ss;
  DeviceManager device_manager(*controller_uid);
  InputHandler input_handler(&ss, *controller_uid, &device_manager);

  USBDeviceManager manager(
    &ss, NewCallback(&device_manager, &DeviceManager::DeviceEvent));
  if (!manager.Start()) {
    exit(ola::EXIT_UNAVAILABLE);
  }
  ss.Run();
  return ola::EXIT_OK;
}
