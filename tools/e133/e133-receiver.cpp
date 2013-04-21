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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * e133-receiver.cpp
 * Copyright (C) 2011 Simon Newton
 *
 * This creates a E1.33 receiver with one (emulated) RDM responder. The node is
 * registered in slp and the RDM responder responds to E1.33 commands.
 */

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sysexits.h>

#include <ola/BaseTypes.h>
#include <ola/Logging.h>
#include <ola/acn/ACNPort.h>
#include <ola/base/Flags.h>
#include <ola/base/Init.h>
#include <ola/network/InterfacePicker.h>
#include <ola/rdm/UID.h>

#include <memory>
#include <string>

#include "tools/e133/SimpleE133Node.h"

using ola::network::IPV4Address;
using ola::rdm::UID;
using std::auto_ptr;
using std::string;

DEFINE_string(listen_ip, "", "The IP address to listen on.");
DEFINE_s_uint16(lifetime, t, 300, "The value to use for the service lifetime");
DEFINE_string(uid, "7a70:00000001", "The UID of the responder.");
DEFINE_s_uint32(universe, u, 1, "The E1.31 universe to listen on.");

SimpleE133Node *simple_node;

/*
 * Terminate cleanly on interrupt.
 */
static void InteruptSignal(int signo) {
  if (simple_node)
    simple_node->Stop();
  (void) signo;
}


/*
 * Startup a node
 */
int main(int argc, char *argv[]) {
  ola::SetHelpString(
      "[options]",
      "Run a very simple E1.33 Responder.");
  ola::ParseFlags(&argc, argv);
  ola::InitLoggingFromFlags();

  auto_ptr<UID> uid(UID::FromString(FLAGS_uid));
  if (!uid.get()) {
    OLA_WARN << "Invalid UID: " << FLAGS_uid;
    ola::DisplayUsage();
    exit(EX_USAGE);
  }

  ola::network::Interface interface;

  {
    auto_ptr<const ola::network::InterfacePicker> picker(
      ola::network::InterfacePicker::NewPicker());
    if (!picker->ChooseInterface(&interface, FLAGS_listen_ip)) {
      OLA_INFO << "Failed to find an interface";
      exit(EX_UNAVAILABLE);
    }
  }

  SimpleE133Node::Options opts(interface.ip_address, *uid, FLAGS_lifetime);
  SimpleE133Node node(opts);
  simple_node = &node;

  if (!node.Init())
    exit(EX_UNAVAILABLE);

  // signal handler
  if (!ola::InstallSignal(SIGINT, &InteruptSignal))
    return false;

  node.Run();
}
