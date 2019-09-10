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
 * e133-monitor.cpp
 * Copyright (C) 2011 Simon Newton
 *
 * This opens a TCP connection to each device in --targets.
 *
 * It then waits to receive E1.33 messages on the TCP connections.
 */

#include <ola/Callback.h>
#include <ola/Constants.h>
#include <ola/Logging.h>
#include <ola/StringUtils.h>
#include <ola/acn/CID.h>
#include <ola/base/Flags.h>
#include <ola/base/Init.h>
#include <ola/base/SysExits.h>
#include <ola/e133/DeviceManager.h>
#include <ola/e133/MessageBuilder.h>
#include <ola/io/SelectServer.h>
#include <ola/io/StdinHandler.h>
#include <ola/network/IPV4Address.h>
#include <ola/rdm/CommandPrinter.h>
#include <ola/rdm/PidStoreHelper.h>
#include <ola/rdm/RDMCommand.h>
#include <ola/rdm/RDMHelper.h>
#include <ola/rdm/UID.h>

#include <iostream>
#include <memory>
#include <string>
#include <vector>

using ola::NewCallback;
using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;
using ola::rdm::PidStoreHelper;
using ola::rdm::RDMCommand;
using ola::rdm::UID;
using std::auto_ptr;
using std::cout;
using std::endl;
using std::string;
using std::vector;

DEFINE_s_string(pid_location, p, "",
                "The directory to read PID definitiions from");
DEFINE_s_string(target_addresses, t, "",
                "List of IPs to connect to");


/**
 * A very simple E1.33 Controller that acts as a passive monitor.
 */
class SimpleE133Monitor {
 public:
    explicit SimpleE133Monitor(PidStoreHelper *pid_helper);
    ~SimpleE133Monitor();

    bool Init();
    void AddIP(const IPV4Address &ip_address);

    void Run() { m_ss.Run(); }

 private:
    ola::rdm::CommandPrinter m_command_printer;
    ola::io::SelectServer m_ss;
    ola::io::StdinHandler m_stdin_handler;

    ola::e133::MessageBuilder m_message_builder;
    ola::e133::DeviceManager m_device_manager;

    void Input(int c);

    bool EndpointRequest(
        const IPV4Address &source,
        uint16_t endpoint,
        const string &raw_request);
};


/**
 * Setup a new Monitor
 */
SimpleE133Monitor::SimpleE133Monitor(PidStoreHelper *pid_helper)
    : m_command_printer(&cout, pid_helper),
      m_stdin_handler(&m_ss,
                      ola::NewCallback(this, &SimpleE133Monitor::Input)),
      m_message_builder(ola::acn::CID::Generate(), "OLA Monitor"),
      m_device_manager(&m_ss, &m_message_builder) {
  m_device_manager.SetRDMMessageCallback(
      NewCallback(this, &SimpleE133Monitor::EndpointRequest));
}


SimpleE133Monitor::~SimpleE133Monitor() {
  // This used to stop the SLP thread.
}


bool SimpleE133Monitor::Init() {
  // Previously this started the SLP thread.
  return true;
}


void SimpleE133Monitor::AddIP(const IPV4Address &ip_address) {
  m_device_manager.AddDevice(ip_address);
}

void SimpleE133Monitor::Input(int c) {
  switch (c) {
    case 'q':
      m_ss.Terminate();
      break;
    default:
      break;
  }
}

/**
 * We received data to endpoint 0
 */
bool SimpleE133Monitor::EndpointRequest(
    const IPV4Address &source,
    uint16_t endpoint,
    const string &raw_request) {
  unsigned int slot_count = raw_request.size();
  const uint8_t *rdm_data = reinterpret_cast<const uint8_t*>(
    raw_request.data());

  cout << "From " << source << ":" << endpoint << endl;
  auto_ptr<RDMCommand> command(
      RDMCommand::Inflate(reinterpret_cast<const uint8_t*>(raw_request.data()),
                          raw_request.size()));
  if (command.get()) {
    command->Print(&m_command_printer, false, true);
  } else {
    ola::FormatData(&cout, rdm_data, slot_count, 2);
  }
  return true;
}


/*
 * Startup a node
 */
int main(int argc, char *argv[]) {
  ola::AppInit(&argc, argv, "[options]",
               "Open a TCP connection to E1.33 Devices and wait for E1.33 "
               "messages.");

  PidStoreHelper pid_helper(FLAGS_pid_location, 4);

  vector<IPV4Address> targets;
  if (!FLAGS_target_addresses.str().empty()) {
    vector<string> tokens;
    ola::StringSplit(FLAGS_target_addresses, &tokens, ",");

    vector<string>::const_iterator iter = tokens.begin();
    for (; iter != tokens.end(); ++iter) {
      IPV4Address ip_address;
      if (!IPV4Address::FromString(*iter, &ip_address)) {
        OLA_WARN << "Invalid address " << *iter;
        ola::DisplayUsage();
      }
      targets.push_back(ip_address);
    }
  }

  if (!pid_helper.Init()) {
    exit(ola::EXIT_OSFILE);
  }

  SimpleE133Monitor monitor(&pid_helper);
  if (!monitor.Init()) {
    exit(ola::EXIT_UNAVAILABLE);
  }

  // manually add the responder IPs
  vector<IPV4Address>::const_iterator iter = targets.begin();
  for (; iter != targets.end(); ++iter) {
    monitor.AddIP(*iter);
  }
  monitor.Run();
}
