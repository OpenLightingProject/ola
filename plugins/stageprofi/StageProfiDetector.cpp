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
 * StageProfiDetector.cpp
 * Detects StageProfi devices as they are attached.
 * Copyright (C) 2014 Simon Newton
 */


#include "plugins/stageprofi/StageProfiDetector.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "ola/Callback.h"
#include "ola/Clock.h"
#include "ola/Logging.h"
#include "ola/file/Util.h"
#include "ola/io/Descriptor.h"
#include "ola/io/IOUtils.h"
#include "ola/io/SelectServerInterface.h"
#include "ola/io/Serial.h"
#include "ola/network/IPV4Address.h"
#include "ola/network/SocketAddress.h"
#include "ola/stl/STLUtils.h"

namespace ola {
namespace plugin {
namespace stageprofi {

using ola::TimeInterval;
using ola::io::ConnectedDescriptor;
using ola::io::SelectServerInterface;
using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;
using ola::thread::INVALID_TIMEOUT;
using std::set;
using std::string;
using std::vector;

bool EndpointFromString(const string &widget_path,
                        IPV4SocketAddress *socket_addr) {
  static const uint16_t STAGEPROFI_PORT = 10001;
  IPV4Address ip_address;
  if (!IPV4Address::FromString(widget_path, &ip_address)) {
    OLA_WARN << "Invalid StageProfi device: " << widget_path;
    return false;
  }

  *socket_addr = ola::network::IPV4SocketAddress(ip_address, STAGEPROFI_PORT);
  return true;
}

StageProfiDetector::StageProfiDetector(SelectServerInterface *ss,
                                       const vector<string> &widget_paths,
                                       WidgetCallback *callback)
    : m_ss(ss),
      m_callback(callback),
      m_timeout_id(INVALID_TIMEOUT),
      m_backoff(TimeInterval(1, 0), TimeInterval(300, 0)),
      m_socket_factory(NewCallback(this, &StageProfiDetector::SocketConnected)),
      m_tcp_connector(ss, &m_socket_factory, TimeInterval(3, 0)) {
  if (!callback) {
    OLA_FATAL << "No WidgetCallback provided";
    return;
  }

  set<string> paths(widget_paths.begin(), widget_paths.end());
  set<string>::const_iterator iter = paths.begin();
  for (; iter != paths.end(); ++iter) {
    if (iter->empty()) {
      continue;
    }

    if (iter->at(0) == ola::file::PATH_SEPARATOR) {
      STLReplace(&m_usb_widgets, *iter, NULL);
    } else {
      IPV4SocketAddress socket_addr;
      if (EndpointFromString(*iter, &socket_addr)) {
        m_tcp_connector.AddEndpoint(socket_addr, &m_backoff);
      }
    }
  }
}

StageProfiDetector::~StageProfiDetector() {
  Stop();
}

void StageProfiDetector::Start() {
  if (m_timeout_id == INVALID_TIMEOUT) {
    m_timeout_id = m_ss->RegisterRepeatingTimeout(
        ola::TimeInterval(5, 0),
        NewCallback(this, &StageProfiDetector::RunDiscovery));
  }
}

void StageProfiDetector::Stop() {
  if (m_timeout_id != INVALID_TIMEOUT) {
    m_ss->RemoveTimeout(m_timeout_id);
    m_timeout_id = INVALID_TIMEOUT;
  }
}

void StageProfiDetector::ReleaseWidget(const std::string &widget_path) {
  // It's important not to add the widget_path if it doesn't already exist in
  // the map.
  DescriptorMap::iterator iter = m_usb_widgets.find(widget_path);
  if (iter != m_usb_widgets.end()) {
    ola::io::ReleaseSerialPortLock(widget_path);
    iter->second = NULL;
    return;
  }

  iter = m_tcp_widgets.find(widget_path);
  if (iter != m_tcp_widgets.end()) {
    iter->second = NULL;
    IPV4SocketAddress socket_addr;
    if (EndpointFromString(widget_path, &socket_addr)) {
      m_tcp_connector.Disconnect(socket_addr);
    }
  }
}

bool StageProfiDetector::RunDiscovery() {
  DescriptorMap::iterator iter = m_usb_widgets.begin();
  for (; iter != m_usb_widgets.end(); ++iter) {
    if (iter->second) {
      continue;
    }

    ConnectedDescriptor *descriptor = ConnectToUSB(iter->first);
    if (descriptor) {
      iter->second = descriptor;
      if (m_callback.get()) {
        m_callback->Run(iter->first, descriptor);
      }
    }
  }
  return true;
}

ConnectedDescriptor* StageProfiDetector::ConnectToUSB(
    const string &widget_path) {
  struct termios newtio;

  int fd;
  if (!ola::io::AcquireLockAndOpenSerialPort(widget_path,
                                             O_RDWR | O_NONBLOCK | O_NOCTTY,
                                             &fd)) {
    return NULL;
  }

  memset(&newtio, 0, sizeof(newtio));  // clear struct for new port settings
  tcgetattr(fd, &newtio);
  cfsetospeed(&newtio, B38400);
  tcsetattr(fd, TCSANOW, &newtio);

  return new ola::io::DeviceDescriptor(fd);
}

void StageProfiDetector::SocketConnected(ola::network::TCPSocket *socket) {
  ola::network::GenericSocketAddress socket_addr = socket->GetPeerAddress();
  if (socket_addr.Family() != AF_INET) {
    delete socket;
    return;
  }
  IPV4SocketAddress v4_socket_addr = socket_addr.V4Addr();
  string key = v4_socket_addr.Host().ToString();

  OLA_INFO << "Connected to " << v4_socket_addr;
  DescriptorMap::iterator iter = STLLookupOrInsertNull(&m_tcp_widgets, key);
  if (iter->second) {
    OLA_WARN << "Duplicate socket for " << key;
    delete socket;
    return;
  }

  if (m_callback.get()) {
    m_callback->Run(key, socket);
  }
}
}  // namespace stageprofi
}  // namespace plugin
}  // namespace ola
