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
 * StageProfiDetector.h
 * Detects StageProfi devices as they are attached.
 * Copyright (C) 2014 Simon Newton
 */

#ifndef PLUGINS_STAGEPROFI_STAGEPROFIDETECTOR_H_
#define PLUGINS_STAGEPROFI_STAGEPROFIDETECTOR_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "ola/Callback.h"
#include "ola/io/SelectServerInterface.h"
#include "ola/io/Descriptor.h"
#include "ola/network/AdvancedTCPConnector.h"
#include "ola/util/Backoff.h"

namespace ola {
namespace plugin {
namespace stageprofi {

class StageProfiWidget;

class StageProfiDetector {
 public:
  typedef ola::Callback2<void, const std::string&, io::ConnectedDescriptor*>
      WidgetCallback;

  StageProfiDetector(ola::io::SelectServerInterface *ss,
                     const std::vector<std::string> &widget_paths,
                     WidgetCallback *callback);
  ~StageProfiDetector();

  void Start();
  void Stop();

  void ReleaseWidget(const std::string &widget_path);

 private:
  typedef std::map<std::string, ola::io::ConnectedDescriptor*> DescriptorMap;

  ola::io::SelectServerInterface *m_ss;
  std::unique_ptr<WidgetCallback> m_callback;
  ola::thread::timeout_id m_timeout_id;
  DescriptorMap m_usb_widgets;
  DescriptorMap m_tcp_widgets;
  ola::ExponentialBackoffPolicy m_backoff;

  // TCP members
  ola::network::TCPSocketFactory m_socket_factory;
  ola::network::AdvancedTCPConnector m_tcp_connector;

  bool RunDiscovery();
  ola::io::ConnectedDescriptor* ConnectToUSB(const std::string &widget_path);

  void SocketConnected(ola::network::TCPSocket *socket);
};
}  // namespace stageprofi
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_STAGEPROFI_STAGEPROFIDETECTOR_H_
