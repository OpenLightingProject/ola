/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * usbpro_test.cpp
 * Allows testing of a usb pro
 * Copyright (C) 2010 Simon Newton
 */

#include <assert.h>
#include <getopt.h>
#include <string>
#include "ola/Logging.h"
#include "ola/DmxBuffer.h"
#include "ola/network/SelectServer.h"
#include "ola/network/Socket.h"
#include "plugins/usbpro/UsbProWidget.h"
#include "plugins/usbpro/UsbProWidgetListener.h"

using ola::DmxBuffer;
using ola::network::SelectServer;
using ola::plugin::usbpro::UsbProWidget;
using std::string;


class Listener: public ola::plugin::usbpro::UsbProWidgetListener {
  public:
    Listener(UsbProWidget *widget): m_widget(widget) {}
    void HandleWidgetDmx() {
      const DmxBuffer &buffer = m_widget->FetchDMX();
      OLA_INFO << buffer.ToString();
    }

  private:
    UsbProWidget *m_widget;
};

int main(int argc, char *argv[]) {
  ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
  string usb_path;

  static struct option long_options[] = {
    {"usb", required_argument, 0, 'u'},
    {0, 0, 0, 0}
  };

  int option_index = 0;

  while (1) {
    int c = getopt_long(argc, argv, "u:", long_options, &option_index);

    if (c == -1)
      break;

    switch (c) {
      case 0:
        break;
      case 'u':
        usb_path = optarg;
        break;
      case '?':
        break;
      default:
        break;
    }
  }

  SelectServer ss;
  UsbProWidget widget;
  assert(widget.Connect(usb_path));
  assert(ss.AddSocket(widget.GetSocket()));
  assert(widget.ChangeToReceiveMode());

  Listener listener(&widget);
  widget.SetListener(&listener);
  ss.Run();
}
