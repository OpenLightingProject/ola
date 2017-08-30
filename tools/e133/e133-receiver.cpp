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
 * e133-receiver.cpp
 * Copyright (C) 2011 Simon Newton
 *
 * This creates a E1.33 receiver with one (emulated) RDM responder. The node's
 * RDM responder responds to E1.33 commands.
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H

#include <errno.h>
#include <signal.h>

#include <ola/acn/ACNPort.h>
#include <ola/acn/CID.h>
#include <ola/Constants.h>
#include <ola/DmxBuffer.h>
#include <ola/Logging.h>
#include <ola/base/Flags.h>
#include <ola/base/Init.h>
#include <ola/base/SysExits.h>
#include <ola/io/Descriptor.h>
#include <ola/network/InterfacePicker.h>
#include <ola/rdm/DummyResponder.h>
#include <ola/rdm/RDMControllerAdaptor.h>
#include <ola/rdm/UID.h>
#include <ola/rdm/UIDAllocator.h>
#include <ola/stl/STLUtils.h>

#include <memory>
#include <string>
#include <vector>

#ifdef USE_SPI
#include "plugins/spi/SpiBackend.h"
#include "plugins/spi/SpiOutput.h"
#include "plugins/spi/SpiWriter.h"
using ola::plugin::spi::SoftwareBackend;
using ola::plugin::spi::SpiOutput;
using ola::plugin::spi::SpiWriter;
DEFINE_string(spi_device, "", "Path to the SPI device to use.");
#endif  // USE_SPI

#include "libs/acn/E131Node.h"
#include "plugins/usbpro/BaseUsbProWidget.h"
#include "plugins/usbpro/DmxTriWidget.h"
#include "tools/e133/SimpleE133Node.h"

using ola::DmxBuffer;
using ola::acn::CID;
using ola::network::IPV4Address;
using ola::rdm::UID;
using std::auto_ptr;
using std::string;
using std::vector;
using ola::plugin::usbpro::DmxTriWidget;

DEFINE_default_bool(dummy, true, "Include a dummy responder endpoint");
DEFINE_default_bool(e131, true, "Include E1.31 support");
DEFINE_string(listen_ip, "", "The IP address to listen on.");
DEFINE_string(uid, "7a70:00000001", "The UID of the responder.");
DEFINE_s_uint16(lifetime, t, 300, "The value to use for the service lifetime");
DEFINE_s_uint32(universe, u, 1, "The E1.31 universe to listen on.");
DEFINE_string(tri_device, "", "Path to the RDM-TRI device to use.");

SimpleE133Node *simple_node;

/*
 * Terminate cleanly on interrupt.
 */
static void InteruptSignal(OLA_UNUSED int signo) {
  int old_errno = errno;
  if (simple_node) {
    simple_node->Stop();
  }
  errno = old_errno;
}

void HandleTriDMX(DmxBuffer *buffer, DmxTriWidget *widget) {
  widget->SendDMX(*buffer);
}


#ifdef USE_SPI
void HandleSpiDMX(DmxBuffer *buffer, SpiOutput *output) {
  output->WriteDMX(*buffer);
}
#endif  // USE_SPI


/*
 * Startup a node
 */
int main(int argc, char *argv[]) {
  ola::AppInit(&argc, argv, "[options]", "Run a very simple E1.33 Responder.");

  auto_ptr<UID> uid(UID::FromString(FLAGS_uid));
  if (!uid.get()) {
    OLA_WARN << "Invalid UID: " << FLAGS_uid;
    ola::DisplayUsage();
    exit(ola::EXIT_USAGE);
  }

  CID cid = CID::Generate();

  // Find a network interface to use
  ola::network::Interface interface;

  {
    auto_ptr<const ola::network::InterfacePicker> picker(
      ola::network::InterfacePicker::NewPicker());
    if (!picker->ChooseInterface(&interface, FLAGS_listen_ip)) {
      OLA_INFO << "Failed to find an interface";
      exit(ola::EXIT_UNAVAILABLE);
    }
  }

  // Setup the Node.
  SimpleE133Node::Options opts(cid, interface.ip_address, *uid, FLAGS_lifetime);
  SimpleE133Node node(opts);

  // Optionally attach some other endpoints.
  vector<E133Endpoint*> endpoints;
  auto_ptr<ola::rdm::DummyResponder> dummy_responder;
  auto_ptr<ola::rdm::DiscoverableRDMControllerAdaptor>
      discoverable_dummy_responder;
  auto_ptr<DmxTriWidget> tri_widget;

  ola::rdm::UIDAllocator uid_allocator(*uid);
  // The first uid is used for the management endpoint so we burn a UID here.
  {
    auto_ptr<UID> dummy_uid(uid_allocator.AllocateNext());
  }

  // Setup E1.31 if required.
  auto_ptr<ola::acn::E131Node> e131_node;
  if (FLAGS_e131) {
    e131_node.reset(new ola::acn::E131Node(
                    node.SelectServer(), FLAGS_listen_ip,
                    ola::acn::E131Node::Options(), cid));
    if (!e131_node->Start()) {
      OLA_WARN << "Failed to start E1.31 node";
      exit(ola::EXIT_UNAVAILABLE);
    }
    OLA_INFO << "Started E1.31 node!";
    node.SelectServer()->AddReadDescriptor(e131_node->GetSocket());
  }

  if (FLAGS_dummy) {
    auto_ptr<UID> dummy_uid(uid_allocator.AllocateNext());
    OLA_INFO << "Dummy UID is " << *dummy_uid;
    if (!dummy_uid.get()) {
      OLA_WARN << "Failed to allocate a UID for the DummyResponder.";
      exit(ola::EXIT_USAGE);
    }

    dummy_responder.reset(new ola::rdm::DummyResponder(*dummy_uid));
    discoverable_dummy_responder.reset(
        new ola::rdm::DiscoverableRDMControllerAdaptor(
        *dummy_uid, dummy_responder.get()));
    endpoints.push_back(new E133Endpoint(discoverable_dummy_responder.get(),
                                         E133Endpoint::EndpointProperties()));
  }

  // uber hack for now.
  // TODO(simon): fix this
  DmxBuffer tri_buffer;
  uint8_t unused_priority;
  if (!FLAGS_tri_device.str().empty()) {
    ola::io::ConnectedDescriptor *descriptor =
        ola::plugin::usbpro::BaseUsbProWidget::OpenDevice(FLAGS_tri_device);
    if (!descriptor) {
      OLA_WARN << "Failed to open " << FLAGS_tri_device;
      exit(ola::EXIT_USAGE);
    }
    tri_widget.reset(new DmxTriWidget(node.SelectServer(), descriptor));
    node.SelectServer()->AddReadDescriptor(descriptor);
    E133Endpoint::EndpointProperties properties;
    properties.is_physical = true;
    endpoints.push_back(
        new E133Endpoint(tri_widget.get(), properties));

    if (e131_node.get()) {
      // Danger!
      e131_node->SetHandler(
          1, &tri_buffer, &unused_priority,
          NewCallback(&HandleTriDMX, &tri_buffer, tri_widget.get()));
    }
  }

  // uber hack for now.
  // TODO(simon): fix this
#ifdef USE_SPI
  auto_ptr<SpiWriter> spi_writer;
  auto_ptr<SoftwareBackend> spi_backend;
  auto_ptr<SpiOutput> spi_output;
  DmxBuffer spi_buffer;

  if (FLAGS_spi_device.present() && !FLAGS_spi_device.str().empty()) {
    auto_ptr<UID> spi_uid(uid_allocator.AllocateNext());
    if (!spi_uid.get()) {
      OLA_WARN << "Failed to allocate a UID for the SPI device.";
      exit(ola::EXIT_USAGE);
    }

    spi_writer.reset(
        new SpiWriter(FLAGS_spi_device, SpiWriter::Options(), NULL));

    SoftwareBackend::Options options;
    spi_backend.reset(new SoftwareBackend(options, spi_writer.get(), NULL));
    if (!spi_backend->Init()) {
      OLA_WARN << "Failed to init SPI backend";
      exit(ola::EXIT_USAGE);
    }

    spi_output.reset(new SpiOutput(
        *spi_uid,
        spi_backend.get(),
        SpiOutput::Options(0, FLAGS_spi_device.str())));
    E133Endpoint::EndpointProperties properties;
    properties.is_physical = true;
    endpoints.push_back(new E133Endpoint(spi_output.get(), properties));

    if (e131_node.get()) {
      // Danger!
      e131_node->SetHandler(
          1, &spi_buffer, &unused_priority,
          NewCallback(&HandleSpiDMX, &spi_buffer, spi_output.get()));
    }
  }
#endif  // USE_SPI

  for (unsigned int i = 0; i < endpoints.size(); i++) {
    node.AddEndpoint(i + 1, endpoints[i]);
  }
  simple_node = &node;

  if (!node.Init()) {
    exit(ola::EXIT_UNAVAILABLE);
  }

  // signal handler
  if (!ola::InstallSignal(SIGINT, &InteruptSignal)) {
    exit(ola::EXIT_OSERR);
  }

  node.Run();
  if (e131_node.get()) {
    node.SelectServer()->RemoveReadDescriptor(e131_node->GetSocket());
  }
  for (unsigned int i = 0; i < endpoints.size(); i++) {
    node.RemoveEndpoint(i + 1);
  }
  ola::STLDeleteElements(&endpoints);
}
