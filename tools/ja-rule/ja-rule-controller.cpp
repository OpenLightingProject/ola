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
#include <ola/rdm/RDMCommand.h>
#include <ola/io/SelectServer.h>
#include <ola/io/StdinHandler.h>
#include <ola/rdm/UID.h>
#include <ola/util/Utils.h>

#include <iostream>
#include <memory>
#include <string>

#include "tools/ja-rule/JaRuleEndpoint.h"
#include "tools/ja-rule/JaRuleWidget.h"
#include "tools/ja-rule/USBDeviceManager.h"

using ola::NewCallback;
using ola::NewSingleCallback;
using ola::io::SelectServer;
using ola::io::StdinHandler;
using ola::rdm::RDMSetRequest;
using ola::rdm::UID;
using ola::rdm::UIDSet;
using std::auto_ptr;
using std::cerr;
using std::cout;
using std::endl;
using std::string;

DEFINE_string(controller_uid, "7a70:fffffe00", "The UID of the controller.");

class WidgetManager {
 public:
  explicit WidgetManager(const UID &controller_uid)
      : m_endpoint(NULL),
        m_widget_uid(controller_uid) {
  }

  void WidgetEvent(USBDeviceManager::EventType event,
                   JaRuleEndpoint* device) {
    if (event == USBDeviceManager::DEVICE_ADDED) {
      OLA_INFO << "Open Lighting Device added";
      if (m_endpoint) {
        // We only support a single device for now
        OLA_WARN << "More than one device present";
        return;
      }
      m_endpoint = device;
      m_widget.reset(new JaRuleWidget(device, m_widget_uid));
    } else {
      OLA_INFO << "Open Lighting Device removed";
      if (device == m_endpoint) {
        m_widget.reset();
        m_endpoint = NULL;
      }
    }
  }

  // Only valid for the lifetime of the select server event.
  JaRuleWidget* GetWidget() {
    return m_widget.get();
  }

 private:
  JaRuleEndpoint *m_endpoint;
  const UID m_widget_uid;
  std::auto_ptr<JaRuleWidget> m_widget;

  DISALLOW_COPY_AND_ASSIGN(WidgetManager);
};


/**
 * @brief Wait on input from the keyboard, and based on the input, send
 * messages to the device.
 */
class InputHandler {
 public:
  InputHandler(SelectServer* ss,
               const UID &controller_uid,
               WidgetManager *widget_manager)
      : m_ss(ss),
        m_widget_manager(widget_manager),
        m_widget_uid(controller_uid),
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
  WidgetManager *m_widget_manager;
  const UID m_widget_uid;
  auto_ptr<StdinHandler> m_stdin_handler;
  UIDSet m_uids;
  Mode m_mode;
  UID m_selected_uid;

  void SetIdentify(bool identify_on) {
    JaRuleWidget *widget = m_widget_manager->GetWidget();
    if (!widget) {
      return;
    }

    if (m_uids.Size() == 0) {
      OLA_WARN << "No UIDs";
      return;
    }

    uint8_t param_data = identify_on;
    RDMSetRequest *request = new RDMSetRequest(
        m_widget_uid, m_selected_uid, 0, 0, 0, ola::rdm::PID_IDENTIFY_DEVICE,
        &param_data, sizeof(param_data));
    widget->SendRDMRequest(request, NULL);
  }

  void RunDiscovery(bool incremental) {
    JaRuleWidget *widget = m_widget_manager->GetWidget();
    if (!widget) {
      return;
    }
    if (incremental) {
      widget->RunIncrementalDiscovery(
          NewSingleCallback(this, &InputHandler::DiscoveryComplete));
    } else {
      widget->RunFullDiscovery(
          NewSingleCallback(this, &InputHandler::DiscoveryComplete));
    }
  }

  void ResetDevice() {
    JaRuleWidget *widget = m_widget_manager->GetWidget();
    if (!widget) {
      return;
    }
    OLA_INFO << "Resetting device";
    widget->ResetDevice();
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
  WidgetManager widget_manager(*controller_uid);
  InputHandler input_handler(&ss, *controller_uid, &widget_manager);

  USBDeviceManager manager(
    &ss, NewCallback(&widget_manager, &WidgetManager::WidgetEvent));
  if (!manager.Start()) {
    exit(ola::EXIT_UNAVAILABLE);
  }
  ss.Run();
  return ola::EXIT_OK;
}
