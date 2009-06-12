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
 * usbpro-firmware.cpp
 * Copyright (C) 2005-2009 Simon Newton
 */

#include <getopt.h>
#include <errno.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <string>

#include <lla/Logging.h>
#include <lla/network/SelectServer.h>
#include "usbpro/UsbProWidget.h"
#include "usbpro/UsbProWidgetListener.h"

using namespace std;
using lla::usbpro::UsbProWidget;

static const string DEFAULT_DEVICE = "/dev/ttyUSB0";
static const string DEFAULT_FIRMWARE = "main.bin";

typedef struct {
  bool help;
  lla::log_level log_level;
  string firmware;
  string device;
} options;

// number of seconds to wait for a firmware response before aborting
const int TIMEOUT = 2;

class FirmwareTransferer: public lla::usbpro::UsbProWidgetListener {
  public:
    FirmwareTransferer(ifstream &file,
                       UsbProWidget *widget,
                       lla::network::SelectServer *ss):
      m_sent_last_page(false),
      m_firmware(file),
      m_widget(widget),
      m_ss(ss) {}

    void HandleFirmwareReply(bool success);
    bool SendNextChunk();
  private:
    bool m_sent_last_page;
    ifstream &m_firmware;
    UsbProWidget *m_widget;
    lla::network::SelectServer *m_ss;
};


/*
 * Called when we receive a firmware reply
 */
void FirmwareTransferer::HandleFirmwareReply(bool success) {
  LLA_INFO << "got reply " << success;
  if (success) {
    if (!SendNextChunk()) {
      LLA_FATAL << "Sending failed";
      m_ss->Terminate();
    }
  } else {
    LLA_FATAL << "Bad response from widget";
    m_ss->Terminate();
  }
}


/*
 * Send the next chunk of the firmware file
 */
bool FirmwareTransferer::SendNextChunk() {
  uint8_t page[lla::usbpro::FLASH_PAGE_LENGTH];
  m_firmware.read((char*) page, lla::usbpro::FLASH_PAGE_LENGTH);
  streamsize size = m_firmware.gcount();
  bool ret = m_widget->SendFirmwarePage(page, size);
  if (m_firmware.eof())
    m_sent_last_page = true;
  return ret;
}


/*
 * Parse our command line options
 */
void ParseOptions(int argc, char *argv[], options *opts) {
  static struct option long_options[] = {
      {"device", required_argument, 0, 'd'},
      {"firmware", required_argument, 0, 'f'},
      {"help", no_argument, 0, 'h'},
      {"log-level", required_argument, 0, 'l'},
      {0, 0, 0, 0}
    };

  int option_index = 0;

  while (1) {
    int c = getopt_long(argc, argv, "d:f:hl:", long_options, &option_index);

    if (c == -1)
      break;

    switch (c) {
      case 0:
        break;

      case 'd':
        opts->device = optarg;
        break;

      case 'f':
        opts->firmware = optarg;
        break;

      case 'h':
        opts->help = true;
        break;

      case 'l':
        int ll = atoi(optarg);
        switch(ll) {
          case 0:
            // nothing is written at this level
            // so this turns logging off
            opts->log_level = lla::LLA_LOG_NONE;
            break;
          case 1:
            opts->log_level = lla::LLA_LOG_FATAL;
            break;
          case 2:
            opts->log_level = lla::LLA_LOG_WARN;
            break;
          case 3:
            opts->log_level = lla::LLA_LOG_INFO;
            break;
          case 4:
            opts->log_level = lla::LLA_LOG_DEBUG;
            break;
          default :
            break;
        }
        break;
      case '?':
        break;

      default:
       ;
    }
  }
  return;
}


/*
 * Display the help message
 */
void DisplayHelpAndExit(char *argv[]) {
   cout << "Usage: " << argv[0] <<
  " -d <device> -f <firmware_file>\n"
  "\n"
  "Flash the firmware on an Enttec USB Pro device.\n"
  "\n"
  "  -d <device_path>   The path to the device.\n"
  "  -f <firmware_file> The path to the firmware to use.\n"
  "  -h, --help         Display this help message and exit.\n"
  "  -l, --log-level <level>  Set the loggging level 0 .. 4.\n"
  << endl;
  exit(0);
}



/*
 * Flashes the device
 */
int main(int argc, char *argv[]) {
  options opts;
  opts.log_level = lla::LLA_LOG_WARN;
  opts.help = false;
  opts.firmware = DEFAULT_FIRMWARE;
  opts.device = DEFAULT_DEVICE;
  ParseOptions(argc, argv, &opts);

  if (opts.help)
    DisplayHelpAndExit(argv);
  lla::InitLogging(opts.log_level, lla::LLA_LOG_STDERR);

  ifstream firmware_file(opts.firmware.data());

  if (!firmware_file.is_open()) {
    LLA_FATAL << "Can't open the firmware file " << opts.firmware << ": " <<
      strerror(errno);
    exit(1);
  }

  UsbProWidget widget;
  if (!widget.Connect(opts.device))
    exit(1);

  lla::network::SelectServer ss;
  FirmwareTransferer transferer(firmware_file, &widget, &ss);
  widget.SetListener(&transferer);

  if (!widget.SendReprogram()) {
    LLA_FATAL << "Send message failed";
    exit(1);
  }

  if (!transferer.SendNextChunk()) {
    LLA_FATAL << "Send firmware failed";
    exit(1);
  }

  ss.AddSocket(widget.GetSocket());
  ss.Run();

  widget.Disconnect();
  firmware_file.close();
  return 0;
}
