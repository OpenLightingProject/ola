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
#include <ola/rdm/UIDSet.h>
#include <ola/util/Utils.h>

#include <iostream>
#include <memory>
#include <string>

#include "libs/usb/JaRuleWidget.h"
#include "libs/usb/JaRulePortHandle.h"
#include "tools/ja-rule/USBDeviceManager.h"

using ola::NewCallback;
using ola::NewSingleCallback;
using ola::io::SelectServer;
using ola::io::StdinHandler;
using ola::rdm::RDMSetRequest;
using ola::rdm::UID;
using ola::rdm::UIDSet;
using ola::usb::JaRuleWidget;
using ola::usb::JaRulePortHandle;
using std::auto_ptr;
using std::cerr;
using std::cout;
using std::endl;
using std::string;

/**
 * @brief Wait on input from the keyboard, and based on the input, send
 * messages to the device.
 */
class Controller {
 public:
  explicit Controller(SelectServer* ss)
      : m_ss(ss),
        m_widget(NULL),
        m_port(NULL),
        m_stdin_handler(
            new StdinHandler(ss, ola::NewCallback(this, &Controller::Input))),
        m_mode(NORMAL),
        m_selected_uid(0, 0) {
  }

  ~Controller() {
    m_stdin_handler.reset();
  }

  void WidgetEvent(USBDeviceManager::EventType event,
                   JaRuleWidget *widget) {
    if (event == USBDeviceManager::WIDGET_ADDED) {
      OLA_INFO << "Open Lighting Device added";
      if (m_widget) {
        // We only support a single device for now
        OLA_WARN << "More than one device present";
        return;
      }
      m_widget = widget;
      m_port = widget->ClaimPort(0);
    } else {
      OLA_INFO << "Open Lighting Device removed";
      if (widget == m_widget) {
        m_widget->ReleasePort(0);
        m_port = NULL;
        m_widget = NULL;
      }
    }
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
      case 's':
        if (m_uids.Empty()) {
          cout << "No UIDs found to select from" << endl;
        } else {
          cout << "Enter a letter for the UID" << endl;
          m_mode = SELECT_UID;
        }
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
    cout << " s - Select UID" << endl;
    cout << " u - Show UIDs" << endl;
  }

 private:
  typedef enum {
    NORMAL,
    SELECT_UID,
  } Mode;

  SelectServer* m_ss;
  JaRuleWidget *m_widget;
  JaRulePortHandle *m_port;
  auto_ptr<StdinHandler> m_stdin_handler;
  UIDSet m_uids;
  Mode m_mode;
  UID m_selected_uid;

  void SetIdentify(bool identify_on) {
    if (!m_widget) {
      return;
    }

    if (m_uids.Empty()) {
      OLA_WARN << "No UIDs";
      return;
    }

    uint8_t param_data = identify_on;
    RDMSetRequest *request = new RDMSetRequest(
        m_widget->GetUID(), m_selected_uid, 0, 0, 0,
        ola::rdm::PID_IDENTIFY_DEVICE,
        &param_data, sizeof(param_data));
    m_port->SendRDMRequest(request, NULL);
  }

  void RunDiscovery(bool incremental) {
    if (!m_widget) {
      return;
    }
    if (incremental) {
      m_port->RunIncrementalDiscovery(
          NewSingleCallback(this, &Controller::DiscoveryComplete));
    } else {
      m_port->RunFullDiscovery(
          NewSingleCallback(this, &Controller::DiscoveryComplete));
    }
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

  DISALLOW_COPY_AND_ASSIGN(Controller);
};

/*
 * Main.
 */
int main(int argc, char **argv) {
  ola::AppInit(&argc, argv, "[ options ]", "Ja Rule Admin Tool");

  SelectServer ss;
  Controller controller(&ss);

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
