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
 * basic-controller.cpp
 * Copyright (C) 2014 Simon Newton
 *
 * A controller which just listens for new TCP connections from devices.
 * I'm using this for scale testing.
 */

#include <errno.h>
#include <ola/Callback.h>
#include <ola/Clock.h>
#include <ola/Constants.h>
#include <ola/ExportMap.h>
#include <ola/Logging.h>
#include <ola/acn/CID.h>
#include <ola/base/Flags.h>
#include <ola/base/Init.h>
#include <ola/base/SysExits.h>
#include <ola/e133/MessageBuilder.h>
#include <ola/io/NonBlockingSender.h>
#include <ola/io/SelectServer.h>
#include <ola/network/TCPSocketFactory.h>
#include <ola/stl/STLUtils.h>
#include <signal.h>

#include <map>
#include <memory>
#include <string>
#include <utility>

#include "libs/acn/RootInflator.h"
#include "libs/acn/TCPTransport.h"
#include "tools/e133/E133HealthCheckedConnection.h"

DEFINE_string(listen_ip, "", "The IP Address to listen on");
DEFINE_uint16(listen_port, 5569, "The port to listen on");
DEFINE_uint16(listen_backlog, 100,
              "The backlog for the listen() call. Often limited to 128");
DEFINE_uint32(expected_devices, 1,
              "Time how long it takes until this many devices connect.");
DEFINE_default_bool(stop_after_all_devices, false,
            "Exit once all devices connect");

using ola::NewCallback;
using ola::NewSingleCallback;
using ola::STLFindOrNull;
using ola::TimeInterval;
using ola::TimeStamp;
using ola::io::NonBlockingSender;
using ola::network::GenericSocketAddress;
using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;
using ola::network::TCPSocket;
using ola::acn::IncomingTCPTransport;
using std::auto_ptr;
using std::string;

class SimpleE133Controller *controller = NULL;

/**
 * Holds the state for each device
 */
class DeviceState {
 public:
  DeviceState()
    : socket(NULL),
      message_queue(NULL),
      health_checked_connection(NULL),
      in_transport(NULL) {
  }

  // The following may be NULL.
  // The socket connected to the E1.33 device
  auto_ptr<TCPSocket> socket;
  auto_ptr<NonBlockingSender> message_queue;
  // The Health Checked connection
  auto_ptr<E133HealthCheckedConnection> health_checked_connection;
  auto_ptr<IncomingTCPTransport> in_transport;

 private:
  DISALLOW_COPY_AND_ASSIGN(DeviceState);
};

/**
 * A very simple E1.33 Controller that uses the reverse-connection model.
 */
class SimpleE133Controller {
 public:
  struct Options {
    // The controller to connect to.
    IPV4SocketAddress controller;

    explicit Options(const IPV4SocketAddress &controller)
        : controller(controller) {
    }
  };

  explicit SimpleE133Controller(const Options &options);
  ~SimpleE133Controller();

  bool Start();
  void Stop() { m_ss.Terminate(); }

 private:
  typedef std::map<IPV4SocketAddress, DeviceState*> DeviceMap;

  TimeStamp m_start_time;
  DeviceMap m_device_map;

  const IPV4SocketAddress m_listen_address;
  ola::ExportMap m_export_map;
  ola::io::SelectServer m_ss;
  ola::network::TCPSocketFactory m_tcp_socket_factory;
  ola::network::TCPAcceptingSocket m_listen_socket;

  ola::e133::MessageBuilder m_message_builder;

  ola::acn::RootInflator m_root_inflator;

  bool PrintStats();

  void OnTCPConnect(TCPSocket *socket);
  void ReceiveTCPData(IPV4SocketAddress peer,
                      IncomingTCPTransport *transport);
  void RLPDataReceived(const ola::acn::TransportHeader &header);

  void SocketUnhealthy(IPV4SocketAddress peer);

  void SocketClosed(IPV4SocketAddress peer);

  DISALLOW_COPY_AND_ASSIGN(SimpleE133Controller);
};

SimpleE133Controller::SimpleE133Controller(const Options &options)
    : m_listen_address(options.controller),
      m_ss(&m_export_map),
      m_tcp_socket_factory(
          NewCallback(this, &SimpleE133Controller::OnTCPConnect)),
      m_listen_socket(&m_tcp_socket_factory),
      m_message_builder(ola::acn::CID::Generate(), "E1.33 Controller"),
      m_root_inflator(
          NewCallback(this, &SimpleE133Controller::RLPDataReceived)) {
}

SimpleE133Controller::~SimpleE133Controller() {}

bool SimpleE133Controller::Start() {
  ola::Clock clock;
  clock.CurrentMonotonicTime(&m_start_time);

  if (!m_listen_socket.Listen(m_listen_address, FLAGS_listen_backlog)) {
    return false;
  }
  OLA_INFO << "Listening on " << m_listen_address;

  m_ss.AddReadDescriptor(&m_listen_socket);
  m_ss.RegisterRepeatingTimeout(
      TimeInterval(0, 500000),
      NewCallback(this, &SimpleE133Controller::PrintStats));
  m_ss.Run();
  m_ss.RemoveReadDescriptor(&m_listen_socket);
  return true;
}

bool SimpleE133Controller::PrintStats() {
  const TimeStamp *now = m_ss.WakeUpTime();
  const TimeInterval delay = *now - m_start_time;
  ola::CounterVariable *ss_iterations = m_export_map.GetCounterVar(
      "ss-loop-count");
  OLA_INFO << delay << "," << m_device_map.size() << ","
      << ss_iterations->Value();
  return true;
}

void SimpleE133Controller::OnTCPConnect(TCPSocket *socket_ptr) {
  auto_ptr<TCPSocket> socket(socket_ptr);

  GenericSocketAddress generic_peer = socket->GetPeerAddress();
  if (generic_peer.Family() != AF_INET) {
    OLA_WARN << "Unknown family " << generic_peer.Family();
    return;
  }
  IPV4SocketAddress peer = generic_peer.V4Addr();

  // OLA_INFO << "Received new TCP connection from: " << peer;

  auto_ptr<DeviceState> device_state(new DeviceState());
  device_state->in_transport.reset(
      new IncomingTCPTransport(&m_root_inflator, socket.get()));

  socket->SetOnData(
      NewCallback(this, &SimpleE133Controller::ReceiveTCPData, peer,
                  device_state->in_transport.get()));
  socket->SetOnClose(
      NewSingleCallback(this, &SimpleE133Controller::SocketClosed, peer));

  device_state->message_queue.reset(
      new NonBlockingSender(socket.get(), &m_ss, m_message_builder.pool()));

  auto_ptr<E133HealthCheckedConnection> health_checked_connection(
      new E133HealthCheckedConnection(
          &m_message_builder,
          device_state->message_queue.get(),
          NewSingleCallback(this, &SimpleE133Controller::SocketUnhealthy, peer),
          &m_ss));

  if (!health_checked_connection->Setup()) {
    OLA_WARN << "Failed to setup heartbeat controller for " << peer;
    return;
  }

  device_state->health_checked_connection.reset(
    health_checked_connection.release());
  device_state->socket.reset(socket.release());

  m_ss.AddReadDescriptor(socket_ptr);

  std::pair<DeviceMap::iterator, bool> p = m_device_map.insert(
      std::pair<IPV4SocketAddress, DeviceState*>(peer, NULL));
  if (!p.second) {
    OLA_WARN << "Peer " << peer << " is already connected! This is a bug";
    delete p.first->second;
  }
  p.first->second = device_state.release();

  if (m_device_map.size() == FLAGS_expected_devices) {
    ola::Clock clock;
    TimeStamp now;
    clock.CurrentMonotonicTime(&now);
    OLA_INFO << FLAGS_expected_devices << " connected in "
             << (now - m_start_time);
    if (FLAGS_stop_after_all_devices) {
      m_ss.Terminate();
    }
  }
}

void SimpleE133Controller::ReceiveTCPData(IPV4SocketAddress peer,
                                          IncomingTCPTransport *transport) {
  if (!transport->Receive()) {
    OLA_WARN << "TCP STREAM IS BAD!!!";
    SocketClosed(peer);
  }
}

void SimpleE133Controller::RLPDataReceived(
    const ola::acn::TransportHeader &header) {
  if (header.Transport() != ola::acn::TransportHeader::TCP)
    return;

  DeviceState *device_state = STLFindOrNull(m_device_map, header.Source());

  if (!device_state) {
    OLA_FATAL << "Received data but unable to lookup socket for "
        << header.Source();
    return;
  }

  device_state->health_checked_connection->HeartbeatReceived();
}

void SimpleE133Controller::SocketUnhealthy(IPV4SocketAddress peer) {
  OLA_INFO << "connection to " << peer << " went unhealthy";
  SocketClosed(peer);
}

void SimpleE133Controller::SocketClosed(IPV4SocketAddress peer) {
  OLA_INFO << "Connection to " << peer << " was closed";

  auto_ptr<DeviceState> device(ola::STLLookupAndRemovePtr(&m_device_map, peer));

  if (!device.get()) {
    OLA_WARN << "Can't find device entry";
    return;
  }

  m_ss.RemoveReadDescriptor(device->socket.get());
}

/**
 * Interrupt handler
 */
static void InteruptSignal(OLA_UNUSED int signo) {
  int old_errno = errno;
  if (controller) {
    controller->Stop();
  }
  errno = old_errno;
}

int main(int argc, char *argv[]) {
  ola::SetHelpString("[options]", "Simple E1.33 Controller.");
  ola::ParseFlags(&argc, argv);
  ola::InitLoggingFromFlags();

  // Convert the controller's IP address
  IPV4Address controller_ip;
  if (!FLAGS_listen_ip.str().empty() &&
      !IPV4Address::FromString(FLAGS_listen_ip, &controller_ip)) {
    ola::DisplayUsage();
    exit(ola::EXIT_USAGE);
  }

  ola::InstallSignal(SIGINT, InteruptSignal);
  controller = new SimpleE133Controller(
      SimpleE133Controller::Options(
          IPV4SocketAddress(controller_ip, FLAGS_listen_port)));
  controller->Start();
  delete controller;
  controller = NULL;
}
