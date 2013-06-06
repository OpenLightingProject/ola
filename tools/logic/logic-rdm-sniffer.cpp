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
 * logic-rdm-sniffer.cpp
 * RDM Sniffer software for the Saleae Logic Device.
 * Copyright (C) 2013 Simon Newton
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_SALEAEDEVICEAPI_H
#include <SaleaeDeviceApi.h>
#endif

#include <string.h>
#include <time.h>

#include <ola/Logging.h>
#include <ola/base/Flags.h>
#include <ola/io/SelectServer.h>

#include <ola/BaseTypes.h>
#include <ola/Callback.h>
#include <ola/base/SysExits.h>
#include <ola/Clock.h>
#include <ola/network/NetworkUtils.h>
#include <ola/rdm/CommandPrinter.h>
#include <ola/rdm/PidStoreHelper.h>
#include <ola/rdm/RDMCommand.h>
#include <ola/rdm/RDMEnums.h>
#include <ola/rdm/RDMHelper.h>
#include <ola/rdm/RDMResponseCodes.h>
#include <ola/rdm/UID.h>

#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <vector>


using std::auto_ptr;
using std::cerr;
using std::cout;
using std::endl;
using std::string;
using std::stringstream;
using std::vector;
using ola::io::SelectServer;
using ola::messaging::Descriptor;
using ola::messaging::Message;
using ola::rdm::CommandPrinter;
using ola::rdm::PidStoreHelper;
using ola::rdm::RDMCommand;
using ola::rdm::UID;


using ola::thread::Mutex;
using ola::thread::MutexLocker;


DEFINE_bool(display_asc, false,
              "Display non-RDM alternate start code frames");
DEFINE_s_bool(display_dmx, d, false, "Display DMX Frames");
DEFINE_s_bool(full_rdm, r, true, "Display the full RDM frame");
DEFINE_bool(timestamp, false, "Include timestamps");

DEFINE_uint16(dmx_slot_limit, DMX_UNIVERSE_SIZE,
              "Only display the first N DMX slots");


void OnReadData(U64 device_id, uint8_t *data, uint32_t data_length,
                void *user_data) {
  std::cout << "Read " << data_length << " bytes, starting with 0x"
            << std::hex << static_cast<int>(*data) << std::dec << std::endl;
  DevicesManagerInterface::DeleteU8ArrayPtr(data);
  (void) device_id;
  (void) user_data;
}

void OnError(U64 device_id, void *user_data) {
  OLA_INFO << "A device reported an Error.";
  (void) device_id;
  (void) user_data;
}


class LogicReader {
  public:
    explicit LogicReader(SelectServer *ss)
      : m_device_id(0),
        m_logic(NULL),
        m_ss(ss) {
    }

    void DeviceConnected(U64 device, GenericInterface *interface);
    void DeviceDisconnected(U64 device);

    void Stop();

  private:
    U64 m_device_id;  // GUARDED_BY(mu_);
    LogicInterface *m_logic;  // GUARDED_BY(mu_);
    Mutex m_mu;

    SelectServer *m_ss;

    static const uint32_t SAMPLE_HZ = 4000000;
};


void LogicReader::DeviceConnected(U64 device,
                                  GenericInterface *interface) {
  OLA_INFO << "Device " << device << " connected";
  MutexLocker lock(&m_mu);
  if (m_logic != NULL) {
    OLA_WARN << "More than one device is connected";
    return;
  }

  LogicInterface *logic = dynamic_cast<LogicInterface*>(interface);  // NOLINT
  if (logic == NULL) {
    OLA_WARN << "Only the Logic is supported for now";
    return;
  }

  m_device_id = device;
  m_logic = logic;

  // do stuff here
  m_logic->RegisterOnReadData(&OnReadData);
  m_logic->RegisterOnError(&OnError);

  m_logic->SetSampleRateHz(SAMPLE_HZ);
  m_logic->ReadStart();
}

void LogicReader::DeviceDisconnected(U64 device) {
  OLA_INFO << "Device " << device << " disconnected";

  MutexLocker lock(&m_mu);
  if (device != m_device_id) {
    return;
  }
  m_device_id = 0;
  m_logic = NULL;

  m_ss->Terminate();

  //
}


void LogicReader::Stop() {
  MutexLocker lock(&m_mu);
  if (m_logic) {
    m_logic->Stop();
  }
}

void OnConnect(U64 device_id, GenericInterface* device_interface,
               void* user_data) {
  if (!user_data)
    return;

  LogicReader *reader = (LogicReader*) user_data;  // NOLINT
  reader->DeviceConnected(device_id, device_interface);
}

void OnDisconnect(U64 device_id, void *user_data) {
  if (!user_data)
    return;

  LogicReader *reader = (LogicReader*) user_data;  // NOLINT
  reader->DeviceDisconnected(device_id);
}


/*
 * Main.
 */
int main(int argc, char *argv[]) {
  ola::SetHelpString("[options]",
                     "Decode DMX/RDM data from a Saleae Logic device");
  ola::ParseFlags(&argc, argv);
  ola::InitLoggingFromFlags();

  SelectServer ss;
  LogicReader reader(&ss);

  DevicesManagerInterface::RegisterOnConnect(&OnConnect, &reader);
  DevicesManagerInterface::RegisterOnDisconnect(&OnDisconnect, &reader);
  DevicesManagerInterface::BeginConnect();

  OLA_INFO << "Running...";
  ss.Run();
  reader.Stop();
  return ola::EXIT_OK;
}
