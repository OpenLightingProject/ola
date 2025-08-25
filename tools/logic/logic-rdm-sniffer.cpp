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
#include <ola/io/SelectServer.h>
#include <ola/Logging.h>
#include <ola/network/NetworkUtils.h>

#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <vector>
#include <queue>

#include "libs/sniffer/BaseSnifferReader.h"

using std::auto_ptr;
using std::cerr;
using std::cout;
using std::endl;
using std::string;
using std::vector;
using ola::io::SelectServer;


using ola::thread::Mutex;
using ola::thread::MutexLocker;
using ola::NewSingleCallback;


// TODO(Peter): Implement this! Kept for backwards compatibility
DEFINE_s_default_bool(timestamp, t, false, "Include timestamps.");
DEFINE_uint32(sample_rate, 4000000, "Sample rate in HZ.");
// Set the sample rate.  Must be a supported value, i.e.:  24000000, 16000000,
// 12000000, 8000000, 4000000, 2000000, 1000000, 500000, 250000, 200000,
// 100000, 50000, 25000
// TODO(Peter): Trap and report this, don't just crash

void OnReadData(U64 device_id, U8 *data, uint32_t data_length,
                void *user_data);
void OnError(U64 device_id, void *user_data);

class LogicReader: public BaseSnifferReader {
 public:
    explicit LogicReader(SelectServer *ss, unsigned int sample_rate)
      : BaseSnifferReader(ss, sample_rate),
        m_sample_rate(sample_rate),
        m_device_id(0),
        m_logic(NULL),
        m_ss(ss) {
    }

    void DeviceConnected(U64 device, GenericInterface *interface);
    void DeviceDisconnected(U64 device);
    void DataReceived(U64 device, U8 *data, uint32_t data_length);

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
    Mutex m_data_mu;
    std::queue<U8*> m_free_data;

    void ProcessData(U8 *data, uint32_t data_length);
};

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
