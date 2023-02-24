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
 * basic-device.cpp
 * Copyright (C) 2014 Simon Newton
 *
 * A device which just opens a TCP connection to a controller.
 * I'm using this for scale testing.
 */

#include <errno.h>
#include <ola/Callback.h>
#include <ola/Clock.h>
#include <ola/Constants.h>
#include <ola/Logging.h>
#include <ola/acn/CID.h>
#include <ola/base/Flags.h>
#include <ola/base/Init.h>
#include <ola/base/SysExits.h>
#include <ola/e133/MessageBuilder.h>
#include <ola/io/NonBlockingSender.h>
#include <ola/io/SelectServer.h>
#include <ola/network/AdvancedTCPConnector.h>
#include <ola/network/TCPSocketFactory.h>
#include <signal.h>

#include <memory>
#include <string>

#include "libs/acn/RootInflator.h"
#include "libs/acn/TCPTransport.h"
#include "tools/e133/E133HealthCheckedConnection.h"

DEFINE_string(controller_ip, "", "The IP Address of the Controller");
DEFINE_uint16(controller_port, 5569, "The port on the controller");
DEFINE_uint16(tcp_connect_timeout_ms, 5000,
              "The time in ms for the TCP connect");
DEFINE_uint16(tcp_retry_interval_ms, 5000,
              "The time in ms before retring the TCP connection");

using ola::NewCallback;
using ola::NewSingleCallback;
using ola::TimeInterval;
using ola::io::NonBlockingSender;
using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;
using ola::network::TCPSocket;
using ola::acn::IncomingTCPTransport;
using std::auto_ptr;
using std::string;

/**
 * A very simple E1.33 Device that uses the reverse-connection model.
 */
class SimpleE133Device {
 public:
  struct Options {
    // The controller to connect to.
    IPV4SocketAddress controller;

    explicit Options(const IPV4SocketAddress &controller)
        : controller(controller) {
    }
  };

  explicit SimpleE133Device(const Options &options);
  ~SimpleE133Device();

  void Run();
  void Stop() { m_ss.Terminate(); }

 private:
  const IPV4SocketAddress m_controller;
  ola::io::SelectServer m_ss;

  ola::e133::MessageBuilder m_message_builder;

  ola::network::TCPSocketFactory m_tcp_socket_factory;
  ola::network::AdvancedTCPConnector m_connector;
  ola::ConstantBackoffPolicy m_backoff_policy;
  ola::acn::RootInflator m_root_inflator;

  // Once we have a connection these are filled in.
  auto_ptr<TCPSocket> m_socket;
  auto_ptr<NonBlockingSender> m_message_queue;
  // The Health Checked connection
  auto_ptr<E133HealthCheckedConnection> m_health_checked_connection;
  auto_ptr<IncomingTCPTransport> m_in_transport;

  void OnTCPConnect(TCPSocket *socket);
  void ReceiveTCPData();
  void RLPDataReceived(const ola::acn::TransportHeader &header);

  void SocketUnhealthy(IPV4Address ip_address);
  void SocketClosed();

  DISALLOW_COPY_AND_ASSIGN(SimpleE133Device);
};


SimpleE133Device::SimpleE133Device(const Options &options)
    : m_controller(options.controller),
      m_message_builder(ola::acn::CID::Generate(), "E1.33 Device"),
      m_tcp_socket_factory(NewCallback(this, &SimpleE133Device::OnTCPConnect)),
      m_connector(&m_ss, &m_tcp_socket_factory,
                  TimeInterval(FLAGS_tcp_connect_timeout_ms / 1000,
                               (FLAGS_tcp_connect_timeout_ms % 1000) * 1000)),
      m_backoff_policy(TimeInterval(
            FLAGS_tcp_retry_interval_ms / 1000,
            (FLAGS_tcp_retry_interval_ms % 1000) * 1000)),
      m_root_inflator(NewCallback(this, &SimpleE133Device::RLPDataReceived)) {
  m_connector.AddEndpoint(options.controller, &m_backoff_policy);
}

SimpleE133Device::~SimpleE133Device() {}

void SimpleE133Device::Run() {
  m_ss.Run();
}

void SimpleE133Device::OnTCPConnect(TCPSocket *socket) {
  OLA_INFO << "Opened new TCP connection: " << socket;

  m_socket.reset(socket);
  m_in_transport.reset(new IncomingTCPTransport(&m_root_inflator, socket));

  m_message_queue.reset(
      new NonBlockingSender(m_socket.get(), &m_ss, m_message_builder.pool()));

  m_health_checked_connection.reset(new E133HealthCheckedConnection(
      &m_message_builder,
      m_message_queue.get(),
      NewSingleCallback(this, &SimpleE133Device::SocketClosed),
      &m_ss));

  socket->SetOnData(NewCallback(this, &SimpleE133Device::ReceiveTCPData));
  socket->SetOnClose(NewSingleCallback(this, &SimpleE133Device::SocketClosed));
  m_ss.AddReadDescriptor(socket);


  if (!m_health_checked_connection->Setup()) {
    OLA_WARN << "Failed to setup heartbeat controller for " << m_controller;
    SocketClosed();
    return;
  }
}

void SimpleE133Device::ReceiveTCPData() {
  if (!m_in_transport->Receive()) {
    OLA_WARN << "TCP STREAM IS BAD!!!";
    SocketClosed();
  }
}

void SimpleE133Device::RLPDataReceived(
    const ola::acn::TransportHeader &header) {
  m_health_checked_connection->HeartbeatReceived();
  (void) header;
}

void SimpleE133Device::SocketClosed() {
  OLA_INFO << "controller connection was closed";

  m_health_checked_connection.reset();
  m_message_queue.reset();
  m_in_transport.reset();
  m_ss.RemoveReadDescriptor(m_socket.get());
  m_socket.reset();
  m_connector.Disconnect(m_controller);
}

SimpleE133Device *device = NULL;

/**
 * Interrupt handler
 */
static void InteruptSignal(OLA_UNUSED int signo) {
  int old_errno = errno;
  if (device) {
    device->Stop();
  }
  errno = old_errno;
}

int main(int argc, char *argv[]) {
  ola::SetHelpString("[options]", "Simple E1.33 Device.");
  ola::ParseFlags(&argc, argv);
  ola::InitLoggingFromFlags();

  // Convert the controller's IP address
  IPV4Address controller_ip;
  if (FLAGS_controller_ip.str().empty() ||
      !IPV4Address::FromString(FLAGS_controller_ip, &controller_ip)) {
    ola::DisplayUsage();
    exit(ola::EXIT_USAGE);
  }

  device = new SimpleE133Device(
      SimpleE133Device::Options(
          IPV4SocketAddress(controller_ip, FLAGS_controller_port)));

  ola::InstallSignal(SIGINT, InteruptSignal);
  device->Run();
  delete device;
  device = NULL;
}
