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
 * StageProfiWidget.h
 * This is the base widget class
 * Copyright (C) 2006 Simon Newton
 */

#ifndef PLUGINS_STAGEPROFI_STAGEPROFIWIDGET_H_
#define PLUGINS_STAGEPROFI_STAGEPROFIWIDGET_H_

#include <memory>
#include <string>
#include "ola/DmxBuffer.h"
#include "ola/Callback.h"
#include "ola/io/Descriptor.h"
#include "ola/io/SelectServerInterface.h"

namespace ola {
namespace plugin {
namespace stageprofi {


class StageProfiWidget {
 public:
  typedef ola::SingleUseCallback0<void> DisconnectCallback;

  /**
   * @brief Create a new StageProfiWidget.
   * @param ss The SelectServer.
   * @param descriptor The descriptor to use for the widget. Ownership is
   *   transferred.
   * @param widget_path the path to the widget.
   * @param disconnect_cb Called if the widget is disconnected.
   */
  StageProfiWidget(ola::io::SelectServerInterface *ss,
                   ola::io::ConnectedDescriptor *descriptor,
                   const std::string &widget_path,
                   DisconnectCallback *disconnect_cb);

  /**
   * @brief Destructor.
   */
  ~StageProfiWidget();

  /**
   * @brief Return the path for this widget.
   * @returns Either a filesystem path, or an IP address.
   */
  std::string GetPath() const { return m_widget_path; }

  bool SendDmx(const DmxBuffer &buffer);

 private:
  enum { DMX_MSG_LEN = 255 };
  enum { DMX_HEADER_SIZE = 4};

  ola::io::SelectServerInterface *m_ss;
  std::auto_ptr<ola::io::ConnectedDescriptor> m_descriptor;
  const std::string m_widget_path;
  DisconnectCallback *m_disconnect_cb;
  ola::thread::timeout_id m_timeout_id;
  bool m_got_response;

  void SocketReady();
  void DiscoveryTimeout();
  bool Send255(uint16_t start, const uint8_t *buf, unsigned int len) const;
  void SendQueryPacket();
  void RunDisconnectHandler();
};
}  // namespace stageprofi
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_STAGEPROFI_STAGEPROFIWIDGET_H_
