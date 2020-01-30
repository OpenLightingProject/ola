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
 * logic-rdm-sniffer.cpp
 * RDM Sniffer software for the Saleae Logic Device.
 * Copyright (C) 2013 Simon Newton
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H

#ifdef HAVE_SALEAEDEVICEAPI_H
#include <SaleaeDeviceApi.h>
#endif  // HAVE_SALEAEDEVICEAPI_H

#include <string.h>
#include <time.h>

#include <ola/base/Flags.h>
#include <ola/base/Init.h>
#include <ola/base/Macro.h>
#include <ola/base/SysExits.h>
#include <ola/Callback.h>
#include <ola/Constants.h>
#include <ola/Clock.h>
#include <ola/DmxBuffer.h>
#include <ola/io/SelectServer.h>
#include <ola/Logging.h>
#include <ola/network/NetworkUtils.h>
#include <ola/rdm/CommandPrinter.h>
#include <ola/rdm/PidStoreHelper.h>
#include <ola/rdm/RDMCommand.h>
#include <ola/rdm/RDMEnums.h>
#include <ola/rdm/RDMHelper.h>
#include <ola/rdm/RDMPacket.h>
#include <ola/rdm/RDMResponseCodes.h>
#include <ola/rdm/UID.h>
#include <ola/StringUtils.h>

#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <vector>
#include <queue>

#include "tools/logic/DMXSignalProcessor.h"

using std::auto_ptr;
using std::cerr;
using std::cout;
using std::endl;
using std::string;
using std::vector;
using ola::io::SelectServer;
using ola::messaging::Descriptor;
using ola::messaging::Message;
using ola::rdm::CommandPrinter;
using ola::rdm::PidStoreHelper;
using ola::rdm::RDMCommand;
using ola::rdm::UID;
using ola::strings::ToHex;


using ola::thread::Mutex;
using ola::thread::MutexLocker;
using ola::NewSingleCallback;


DEFINE_default_bool(display_asc, false,
                    "Display non-RDM alternate start code frames.");
DEFINE_s_default_bool(full_rdm, r, false, "Unpack RDM parameter data.");
// TODO(Peter): Implement this!
DEFINE_s_default_bool(timestamp, t, false, "Include timestamps.");
DEFINE_s_default_bool(display_dmx, d, false,
                      "Display DMX Frames. Defaults to false.");
DEFINE_uint16(dmx_slot_limit, ola::DMX_UNIVERSE_SIZE,
              "Only display the first N slots of DMX data.");
DEFINE_uint32(sample_rate, 4000000, "Sample rate in HZ.");
DEFINE_string(pid_location, "",
              "The directory containing the PID definitions.");

void OnReadData(U64 device_id, U8 *data, uint32_t data_length,
                void *user_data);
void OnError(U64 device_id, void *user_data);
void ProcessData(U8 *data, uint32_t data_length);

class LogicReader {
 public:
    explicit LogicReader(SelectServer *ss, unsigned int sample_rate)
      : m_sample_rate(sample_rate),
        m_device_id(0),
        m_logic(NULL),
        m_ss(ss),
        m_signal_processor(ola::NewCallback(this, &LogicReader::FrameReceived),
                           sample_rate),
        m_pid_helper(FLAGS_pid_location.str(), 4),
        m_command_printer(&cout, &m_pid_helper) {
      if (!m_pid_helper.Init()) {
        OLA_WARN << "Failed to init PidStore";
      }
    }
    ~LogicReader();

    void DeviceConnected(U64 device, GenericInterface *interface);
    void DeviceDisconnected(U64 device);
    void DataReceived(U64 device, U8 *data, uint32_t data_length);
    void FrameReceived(const uint8_t *data, unsigned int length);

    void Stop();

    bool IsConnected() const {
      MutexLocker lock(&m_mu);
      return m_logic != NULL;
    }

 private:
    const unsigned int m_sample_rate;
    U64 m_device_id;  // GUARDED_BY(m_mu);
    LogicInterface *m_logic;  // GUARDED_BY(m_mu);
    mutable Mutex m_mu;
    SelectServer *m_ss;
    DMXSignalProcessor m_signal_processor;
    PidStoreHelper m_pid_helper;
    CommandPrinter m_command_printer;
    Mutex m_data_mu;
    std::queue<U8*> m_free_data;

    void ProcessData(U8 *data, uint32_t data_length);
    void DisplayDMXFrame(const uint8_t *data, unsigned int length);
    void DisplayRDMFrame(const uint8_t *data, unsigned int length);
    void DisplayAlternateFrame(const uint8_t *data, unsigned int length);
    void DisplayRawData(const uint8_t *data, unsigned int length);
};

LogicReader::~LogicReader() {
  m_ss->DrainCallbacks();
}

void LogicReader::DeviceConnected(U64 device, GenericInterface *interface) {
  OLA_INFO << "Device " << device << " connected, setting sample rate to "
           << m_sample_rate << "Hz";
  MutexLocker lock(&m_mu);
  if (m_logic != NULL) {
    OLA_WARN << "More than one device is connected";
    return;
  }

  LogicInterface *logic = dynamic_cast<LogicInterface*>(interface);
  if (logic == NULL) {
    OLA_WARN << "Only the Logic is supported for now";
    return;
  }

  m_device_id = device;
  m_logic = logic;

  // do stuff here
  m_logic->RegisterOnReadData(&OnReadData, this);
  m_logic->RegisterOnError(&OnError);

  m_logic->SetSampleRateHz(m_sample_rate);
  m_logic->ReadStart();
}

void LogicReader::DeviceDisconnected(U64 device) {
  OLA_FATAL << "Device " << device << " disconnected";

  MutexLocker lock(&m_mu);
  if (device != m_device_id) {
    return;
  }
  m_device_id = 0;
  m_logic = NULL;

  m_ss->Terminate();
}

/**
 * Called by the receive thread when new data arrives
 * @param device the device id which produced the data
 * @param data pointer to the data, ownership is transferred, use
 *   DeleteU8ArrayPtr to free.
 * @param data_length the size of the data
 */
void LogicReader::DataReceived(U64 device, U8 *data, uint32_t data_length) {
  {
    MutexLocker lock(&m_mu);
    if (device != m_device_id) {
      OLA_WARN << "Received data from another device, expecting "
               << m_device_id << " got " << device;
      DevicesManagerInterface::DeleteU8ArrayPtr(data);
      return;
    }
  }
  m_ss->Execute(
      NewSingleCallback(this, &LogicReader::ProcessData, data, data_length));

  {
    MutexLocker lock(&m_data_mu);
    while (!m_free_data.empty()) {
      U8 *data = m_free_data.front();
      DevicesManagerInterface::DeleteU8ArrayPtr(data);
      m_free_data.pop();
    }
  }
}


void LogicReader::FrameReceived(const uint8_t *data, unsigned int length) {
  if (!length) {
    return;
  }

  switch (data[0]) {
    case ola::DMX512_START_CODE:
      DisplayDMXFrame(data + 1, length - 1);
      break;
    case ola::rdm::START_CODE:
      DisplayRDMFrame(data + 1, length - 1);
      break;
    default:
      DisplayAlternateFrame(data, length);
  }
}


void LogicReader::Stop() {
  MutexLocker lock(&m_mu);
  if (m_logic) {
    m_logic->Stop();
  }
}


/**
 * Called in the main thread.
 * @param data pointer to the data, ownership is transferred, use
 *   DeleteU8ArrayPtr to free.
 * @param data_length the size of the data
 */
void LogicReader::ProcessData(U8 *data, uint32_t data_length) {
  m_signal_processor.Process(data, data_length, 0x01);
  DevicesManagerInterface::DeleteU8ArrayPtr(data);

  /*
   * This is commented out until we get clarification on if DeleteU8ArrayPtr is
   * thread safe. http://community.saleae.com/node/403
   */
  /*
  {
    MutexLocker lock(&m_data_mu);
    m_free_data.push(data);
  }
  */
}


void LogicReader::DisplayDMXFrame(const uint8_t *data, unsigned int length) {
  if (!FLAGS_display_dmx) {
    return;
  }

  cout << "DMX " << std::dec;
  cout << length << ":" << std::hex;
  DisplayRawData(data, length);
}

void LogicReader::DisplayRDMFrame(const uint8_t *data, unsigned int length) {
  auto_ptr<RDMCommand> command(RDMCommand::Inflate(data, length));
  if (command.get()) {
    if (FLAGS_full_rdm) {
      cout << "---------------------------------------" << endl;
    }
    command->Print(&m_command_printer, !FLAGS_full_rdm, true);
  } else {
    cout << "RDM " << std::dec;
    cout << length << ":" << std::hex;
    DisplayRawData(data, length);
  }
}


void LogicReader::DisplayAlternateFrame(const uint8_t *data,
                                        unsigned int length) {
  if (!FLAGS_display_asc || length == 0) {
    return;
  }

  unsigned int slot_count = length - 1;
  cout << "SC " << ToHex(static_cast<int>(data[0]))
       << " " << slot_count << ":";
  DisplayRawData(data + 1, slot_count);
}


/**
 * Dump out the raw data if we couldn't parse it correctly.
 */
void LogicReader::DisplayRawData(const uint8_t *data, unsigned int length) {
  for (unsigned int i = 0; i < length; i++) {
    cout << std::hex << std::setw(2) << std::setfill('0')
         << static_cast<int>(data[i]) << " ";
  }
  cout << endl;
}

// SaleaeDeviceApi callbacks
void OnConnect(U64 device_id, GenericInterface* device_interface,
               void* user_data) {
  if (!user_data) {
    return;
  }

  LogicReader *reader =
      (LogicReader*) user_data;  // NOLINT(readability/casting)
  reader->DeviceConnected(device_id, device_interface);
}

void OnDisconnect(U64 device_id, void *user_data) {
  if (!user_data) {
    return;
  }

  LogicReader *reader =
      (LogicReader*) user_data;  // NOLINT(readability/casting)
  reader->DeviceDisconnected(device_id);
}

void OnReadData(U64 device_id, U8 *data, uint32_t data_length,
                void *user_data) {
  if (!user_data) {
    DevicesManagerInterface::DeleteU8ArrayPtr(data);
    return;
  }
  LogicReader *reader =
      (LogicReader*) user_data;  // NOLINT(readability/casting)
  reader->DataReceived(device_id, data, data_length);
}

void OnError(U64 device_id, OLA_UNUSED void *user_data) {
  OLA_WARN << "Device " << device_id << " reported an error.";
}

void DisplayReminder(LogicReader *reader) {
  if (!reader->IsConnected()) {
    cout << "No devices found, maybe you should check the permissions "
         << "and/or the cable?" << endl;
  }
}

/*
 * Main.
 */
int main(int argc, char *argv[]) {
  ola::AppInit(&argc, argv, "[ options ]",
               "Decode DMX/RDM data from a Saleae Logic device");

  SelectServer ss;
  LogicReader reader(&ss, FLAGS_sample_rate);

  DevicesManagerInterface::RegisterOnConnect(&OnConnect, &reader);
  DevicesManagerInterface::RegisterOnDisconnect(&OnDisconnect, &reader);
  DevicesManagerInterface::BeginConnect();

  OLA_INFO << "Running...";
  ss.RegisterSingleTimeout(3000, NewSingleCallback(DisplayReminder, &reader));
  ss.Run();
  reader.Stop();
  return ola::EXIT_OK;
}
