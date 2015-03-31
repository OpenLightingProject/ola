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
#include <ola/rdm/RDMCommand.h>
#include <ola/rdm/RDMCommandSerializer.h>
#include <ola/rdm/RDMControllerInterface.h>
#include <ola/rdm/UID.h>
#include <ola/util/Utils.h>
#include <ola/util/SequenceNumber.h>

#include <iostream>
#include <memory>
#include <string>

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
using ola::rdm::UID;
using ola::rdm::UIDSet;
using ola::rdm::rdm_response_code;
using ola::strings::ToHex;
using std::auto_ptr;
using std::cout;
using std::endl;
using std::string;

DEFINE_string(controller_uid, "7a70:fffffe00", "The UID of the controller.");

class ControllerImpl : public ola::rdm::DiscoveryTargetInterface,
                       public MessageHandlerInterface {
 public:
  ControllerImpl(OpenLightingDevice *device,
                 const UID &controller_uid)
      : m_device(device),
        m_our_uid(controller_uid),
        m_mute_callback(NULL),
        m_unmute_callback(NULL),
        m_branch_callback(NULL) {
    device->SetHandler(this);
  }

  ~ControllerImpl() {
    m_device->SetHandler(NULL);
  }

  void UpdateDevice(OpenLightingDevice* device) {
    m_device = device;
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

    // TODO(simon): do we need to check there isn't already one in-flight?
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

    // TODO(simon): do we need to check there isn't already one in-flight?
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

    // TODO(simon): do we need to check there isn't already one in-flight?
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
  const UID m_our_uid;
  ola::SequenceNumber<uint8_t> m_transaction_number;
  MuteDeviceCallback *m_mute_callback;
  UnMuteDeviceCallback *m_unmute_callback;
  BranchCallback *m_branch_callback;

  bool CheckForDevice() const {
    if (!m_device) {
      OLA_INFO << "Device not present";
    }
    return m_device != NULL;
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

  DISALLOW_COPY_AND_ASSIGN(ControllerImpl);
};

class Controller : public ola::rdm::DiscoverableRDMControllerInterface {
 public:
  Controller(OpenLightingDevice *device, const UID &controller_uid)
    : m_controller_impl(device, controller_uid),
      m_discovery_agent(&m_controller_impl) {
  }

  ~Controller() {
  }

  void UpdateDevice(OpenLightingDevice* device) {
    m_controller_impl.UpdateDevice(device);
  }

  void SendRDMRequest(const RDMRequest *request,
                      ola::rdm::RDMCallback *on_complete) {
    // TODO(simon): implement me!
    (void) request;
    (void) on_complete;
  }

  void RunFullDiscovery(RDMDiscoveryCallback *callback) {
    OLA_INFO << "Full discovery triggered";
    m_discovery_agent.StartFullDiscovery(
        NewSingleCallback(this, &Controller::DiscoveryComplete,
        callback));
  }

  void RunIncrementalDiscovery(RDMDiscoveryCallback *callback) {
    OLA_INFO << "Incremental discovery triggered";
    m_discovery_agent.StartIncrementalDiscovery(
        NewSingleCallback(this, &Controller::DiscoveryComplete,
        callback));
  }

  void ResetDevice() {
    m_controller_impl.ResetDevice();
  }

 private:
  ControllerImpl m_controller_impl;
  DiscoveryAgent m_discovery_agent;
  UIDSet m_uids;

  void DiscoveryComplete(RDMDiscoveryCallback *callback,
                         bool ok, const UIDSet& uids) {
    OLA_DEBUG << "Discovery complete: " << uids;
    if (ok) {
      m_uids = uids;
    }
    if (callback) {
      callback->Run(m_uids);
    }
  }

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
               DeviceManager *device_manager)
      : m_ss(ss),
        m_device_manager(device_manager),
        m_stdin_handler(
            new StdinHandler(ss,
                             ola::NewCallback(this, &InputHandler::Input))) {
  }

  ~InputHandler() {
    m_stdin_handler.reset();
  }

  void Input(int c) {
    switch (c) {
      case 'd':
        RunDiscovery();
        break;
      case 'h':
        PrintCommands();
        break;
      case 'r':
        ResetDevice();
        break;
      case 'q':
        m_ss->Terminate();
        break;
      default:
        {}
    }
  }

  void PrintCommands() {
    cout << "Commands:" << endl;
    cout << " d - Run Discovery" << endl;
    cout << " h - Print this help message" << endl;
    cout << " q - Quit" << endl;
    cout << " r - Reset" << endl;
  }

 private:
  SelectServer* m_ss;
  DeviceManager *m_device_manager;
  auto_ptr<StdinHandler> m_stdin_handler;

  unsigned int m_log_count;

  void RunDiscovery() {
    Controller *controller = m_device_manager->GetController();
    if (!controller) {
      return;
    }
    controller->RunFullDiscovery(
        NewSingleCallback(this, &InputHandler::DiscoveryComplete));
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
    UIDSet::Iterator iter = uids.Begin();
    cout << "------------" << endl;
    for (; iter != uids.End(); ++iter) {
      cout << *iter << endl;
    }
    cout << "------------" << endl;
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
  InputHandler input_handler(&ss, &device_manager);

  USBDeviceManager manager(
    &ss, NewCallback(&device_manager, &DeviceManager::DeviceEvent));
  if (!manager.Start()) {
    exit(ola::EXIT_UNAVAILABLE);
  }
  ss.Run();
  return ola::EXIT_OK;
}
