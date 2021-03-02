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
 * sigrok-rdm-sniffer.cpp
 * RDM Sniffer software for the any devices supported by Sigrok.
 * Copyright (C) 2021 Peter Newman
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H

// Perhaps add an ifdef
#include <stdio.h>
#include <libsigrok/libsigrok.h>

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
//DEFINE_uint32(sigrok_log_level, SR_LOG_NONE, "Sigrok log level, from "
//                + SR_LOG_NONE + " to " + SR_LOG_SPEW + ".");
DEFINE_uint32(sigrok_log_level, SR_LOG_NONE, "Set the Sigrok logging level from 0 .. 5.");
DEFINE_uint32(sigrok_samples, 200, "Set the Sigrok sample count.");
DEFINE_uint32(sigrok_time, 2000, "Set the Sigrok sample time in ms.");
DEFINE_string(sigrok_device, "demo", "Set the Sigrok device to use.");

//void OnReadData(U64 device_id, U8 *data, uint32_t data_length,
//                void *user_data);
//void OnError(U64 device_id, void *user_data);
//void ProcessData(uint8_t *data, uint64_t data_length);
void ProcessData(const sr_datafeed_logic *logic);

class LogicReader {
 public:
    explicit LogicReader(SelectServer *ss, unsigned int sample_rate)
      : m_sample_rate(sample_rate),
//        m_device_id(0),
//        m_logic(NULL),
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

//    void DeviceConnected(U64 device, GenericInterface *interface);
//    void DeviceDisconnected(U64 device);
    //void DataReceived(U64 device, U8 *data, uint32_t data_length);
    void DataReceived(uint8_t *data, uint64_t data_length);
    //void DataReceived(const sr_datafeed_logic *logic);
    void FrameReceived(const uint8_t *data, unsigned int length);

    void Stop();

    bool IsConnected() const {
      MutexLocker lock(&m_mu);
//      return m_logic != NULL;
      return false;
    }

 private:
    const unsigned int m_sample_rate;
//    U64 m_device_id;  // GUARDED_BY(m_mu);
//    LogicInterface *m_logic;  // GUARDED_BY(m_mu);
    mutable Mutex m_mu;
    SelectServer *m_ss;
    DMXSignalProcessor m_signal_processor;
    PidStoreHelper m_pid_helper;
    CommandPrinter m_command_printer;
    Mutex m_data_mu;
//    std::queue<U8*> m_free_data;
    //uint8_t* copy_data;
    uint8_t copy_data[5000000];


    void ProcessData(uint8_t *data, uint64_t data_length);
    //void ProcessData(uint64_t data_length);
    //void ProcessData(const sr_datafeed_logic *logic);
    void DisplayDMXFrame(const uint8_t *data, unsigned int length);
    void DisplayRDMFrame(const uint8_t *data, unsigned int length);
    void DisplayAlternateFrame(const uint8_t *data, unsigned int length);
    void DisplayRawData(const uint8_t *data, unsigned int length);
};

static void sigrok_feed_callback(const struct sr_dev_inst *sdi,
                                 const struct sr_datafeed_packet *packet,
                                 void *cb_data) {
  OLA_DEBUG << "Got feed callback for " << sdi->driver->name;

  if (packet->type == SR_DF_HEADER) {
    OLA_INFO << "Sigrok header for " << sdi->driver->name << " got header";
    return;
  }

  if (packet->type == SR_DF_END) {
    // TODO(Peter): try to restart acquisition after a delay?
    OLA_WARN << "Sigrok acquisition for " << sdi->driver->name << " ended";
    return;
  }

  if (packet->type != SR_DF_LOGIC) {
    OLA_DEBUG << "Got a non-logic packet, type " << packet->type;
    return;
  }

  const struct sr_datafeed_logic *logic;
  logic = (const sr_datafeed_logic*)packet->payload;
  OLA_DEBUG << "Got " << logic->length << " bytes of unitsize "
            << logic->unitsize;
  //ola::strings::FormatData(&std::cout, ((uint8_t*)logic->data), logic->length);
  LogicReader *reader =
      (LogicReader*) cb_data;  // NOLINT(readability/casting)
  reader->DataReceived(((uint8_t*)logic->data), logic->length);
  //reader->DataReceived(data, logic->length);
  //reader->DataReceived(logic);
  //reader->DataReceived((logic->data), logic->length);
}


class SigrokThread : public ola::thread::Thread {
 public:
    SigrokThread(LogicReader *reader)
     : m_reader(reader) {
    }
    ~SigrokThread();

    bool Stop();
    void *Run();

 private:
    LogicReader *m_reader;
    bool m_term;
    ola::thread::Mutex m_term_mutex;
};

SigrokThread::~SigrokThread() {
  Stop();
}


/**
 * @brief Stop this thread
 */
bool SigrokThread::Stop() {
  {
    ola::thread::MutexLocker locker(&m_term_mutex);

    m_term = true;
  }
  return Join();
}

void *SigrokThread::Run() {
  int ret;
  struct sr_context *sr_ctx;

  sr_log_loglevel_set(FLAGS_sigrok_log_level);

  if ((ret = sr_init(&sr_ctx)) != SR_OK) {
    OLA_FATAL << "Error initializing libsigrok (" << sr_strerror_name(ret)
              << "): " << sr_strerror(ret);
    return NULL;
  }

  struct sr_session *sr_sess;
  if ((sr_sess = sr_session_new()) == NULL) {
    OLA_FATAL << "Error initializing libsigrok session";
    return NULL;
  }

//  if ((ret = sr_session_new(sr_ctx, &sr_sess)) != SR_OK) {
//    OLA_FATAL << "Error initializing libsigrok session (" << sr_strerror_name(ret) << "): " << sr_strerror(ret);
//    return NULL;
//  }

  struct sr_dev_driver **drivers;
  struct sr_dev_driver *driver = {};
  int i;

  drivers = sr_driver_list();
  if (drivers == NULL) {
    OLA_FATAL << "No drivers found";
    return NULL;
  }

  for (i = 0; drivers[i]; i++) {
    //OLA_DEBUG << "Found driver: " << drivers[i]->name;
    if (string(drivers[i]->name) == FLAGS_sigrok_device.str()) {
      OLA_INFO << "Got driver: " << drivers[i]->name << " - " << drivers[i]->longname;
      driver = drivers[i];
    }
  }

  if (driver == NULL) {
    OLA_FATAL << "Failed to find driver " << FLAGS_sigrok_device.str();
    return NULL;
  }

  if ((ret = sr_driver_init(sr_ctx, driver)) != SR_OK) {
    OLA_FATAL << "Error initializing libsigrok driver " << driver->name << " ("
              << sr_strerror_name(ret) << "): " << sr_strerror(ret);
    return NULL;
  }

  GSList *devlist;
  devlist = sr_driver_scan(driver, NULL);
  if ((devlist == NULL) || (g_slist_length(devlist) <= 0)) {
    OLA_FATAL << "Scanning with libsigrok driver " << driver->name << " didn't find any devices";
    return NULL;
  } else {
    OLA_INFO << "Found " << g_slist_length(devlist) << " devices";
  }

  struct sr_dev_inst *sdi;
  sdi = (sr_dev_inst*)g_slist_nth_data(devlist, 0);
  g_slist_free(devlist);

  OLA_INFO << "Found device (using first):";
  OLA_INFO << "\tVendor: " << (sdi->vendor ? sdi->vendor : "");
  OLA_INFO << "\tModel: " << (sdi->model ? sdi->model : "");
  OLA_INFO << "\tVersion: " << (sdi->version ? sdi->version : "");

  if ((ret = sr_dev_open(sdi)) != SR_OK) {
    OLA_FATAL << "Error opening device via libsigrok driver " << driver->name
              << " (" << sr_strerror_name(ret) << "): " << sr_strerror(ret);
    return NULL;
  }

  if ((ret = sr_session_dev_add(sdi)) != SR_OK) {
    OLA_FATAL << "Error adding device to session via libsigrok driver "
              << driver->name << " (" << sr_strerror_name(ret) << "): "
              << sr_strerror(ret);
    return NULL;
  }

  GVariant *gvar;
  sr_config_get(sdi->driver, sdi, NULL, SR_CONF_SAMPLERATE, &gvar);
  OLA_INFO << "Initial sample rate is " << g_variant_get_uint64(gvar) << "Hz";
  g_variant_unref(gvar);

  if ((ret = sr_config_set(
           sdi, NULL, SR_CONF_SAMPLERATE,
           g_variant_new_uint64(FLAGS_sample_rate))) != SR_OK) {
    OLA_FATAL << "Error setting config sample rate via libsigrok driver "
              << driver->name << " (" << sr_strerror_name(ret) << "): "
              << sr_strerror(ret);
    return NULL;
  }

  sr_config_get(sdi->driver, sdi, NULL, SR_CONF_SAMPLERATE, &gvar);
  OLA_INFO << "New sample rate is " << g_variant_get_uint64(gvar) << "Hz";
  g_variant_unref(gvar);

//  if ((ret = sr_config_set(sdi, NULL, SR_CONF_LIMIT_SAMPLES, g_variant_new_uint64(FLAGS_sigrok_samples))) != SR_OK) {
//    OLA_FATAL << "Error setting config limit samples via libsigrok driver "
//              << driver->name << " (" << sr_strerror_name(ret) << "): "
//              << sr_strerror(ret);
//    return NULL;
//  }

/*  if (sr_dev_has_option(sdi, SR_CONF_LIMIT_MSEC)) {
    gvar = g_variant_new_uint64(FLAGS_sigrok_time);
    if (sr_config_set(sdi, NULL, SR_CONF_LIMIT_MSEC, gvar) != SR_OK) {
      OLA_FATAL << "Failed to configure time limit.";
      return NULL;
    }
  } else if (sr_dev_has_option(sdi, SR_CONF_SAMPLERATE)) {
    // Convert to samples based on the samplerate.
    sr_config_get(sdi->driver, sdi, NULL, SR_CONF_SAMPLERATE, &gvar);
    uint64_t limit_samples = (g_variant_get_uint64(gvar) *
        (FLAGS_sigrok_time / (uint64_t)1000));
    g_variant_unref(gvar);
    if (limit_samples == 0) {
      OLA_FATAL << "Not enough time at this samplerate.";
      return NULL;
    }
    gvar = g_variant_new_uint64(limit_samples);
    if (sr_config_set(sdi, NULL, SR_CONF_LIMIT_SAMPLES, gvar) != SR_OK) {
      OLA_FATAL << "Failed to configure time-based sample limit.";
      return NULL;
    }
  }*/

    // Just samples based
    gvar = g_variant_new_uint64(FLAGS_sigrok_samples);
    if (sr_config_set(sdi, NULL, SR_CONF_LIMIT_SAMPLES, gvar) != SR_OK) {
      OLA_FATAL << "Failed to configure time-based sample limit.";
      return NULL;
    }

  if ((ret = sr_session_datafeed_callback_add(sigrok_feed_callback, &m_reader)) != SR_OK) {
    OLA_FATAL << "Error adding session datafeed callback via libsigrok ("
              << sr_strerror_name(ret) << "): " << sr_strerror(ret);
    return NULL;
  }

  // Start acquisition on device.
  if ((ret = sr_session_start()) != SR_OK) {
    OLA_FATAL << "Error starting session (" << sr_strerror_name(ret) << "): "
              << sr_strerror(ret);
    return NULL;
  }

  // Main loop, runs forever.
  sr_session_run();

  sr_session_stop();
  sr_session_dev_remove_all();

  // if ((ret = sr_session_destroy(sr_sess)) != SR_OK) {
  if ((ret = sr_session_destroy()) != SR_OK) {
    OLA_FATAL << "Error destroying libsigrok session ("
              << sr_strerror_name(ret) << "): " << sr_strerror(ret);
    return NULL;
  }

  if ((ret = sr_exit(sr_ctx)) != SR_OK) {
    OLA_FATAL << "Error shutting down libsigrok (" << sr_strerror_name(ret)
              << "): " << sr_strerror(ret);
    return NULL;
  }
  return NULL;
}

LogicReader::~LogicReader() {
  m_ss->DrainCallbacks();
}

/*void LogicReader::DeviceConnected(U64 device, GenericInterface *interface) {
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
}*/

/**
 * Called by the receive thread when new data arrives
 * @param device the device id which produced the data
 * @param data pointer to the data, ownership is transferred, use
 *   DeleteU8ArrayPtr to free.
 * @param data_length the size of the data
 */
void LogicReader::DataReceived(uint8_t *data, uint64_t data_length) {
//void LogicReader::DataReceived(const sr_datafeed_logic *logic) {
/*  {
    MutexLocker lock(&m_mu);
    if (device != m_device_id) {
      OLA_WARN << "Received data from another device, expecting "
               << m_device_id << " got " << device;
      DevicesManagerInterface::DeleteU8ArrayPtr(data);
      return;
    }
  }*/
//  for (unsigned int i = 0 ; i < data_length; i++) {
//    OLA_DEBUG << "Got sample (before) " << ToHex(data[i]);
//  }
  OLA_DEBUG << "Copied...";
  //uint8_t* copy_data;
  //copy_data = new uint8_t[data_length];
  //copy_data = (uint8_t*) malloc(data_length);
  memcpy(copy_data, data, data_length);
  //ola::strings::FormatData(&std::cout, copy_data, data_length);
//  OLA_DEBUG << "Got " << logic->length << " samples";
  OLA_DEBUG << "Got " << data_length << " samples";
  // TODO(Peter): The callback segfaults, but this works...
  LogicReader::ProcessData(copy_data, data_length);
//  m_ss->Execute(
//      NewSingleCallback(this, &LogicReader::ProcessData, data, data_length));
//      NewSingleCallback(this, &LogicReader::ProcessData, copy_data, data_length));
//      NewSingleCallback(this, &LogicReader::ProcessData, data_length));
//      NewSingleCallback(this, &LogicReader::ProcessData, logic));
//  for (unsigned int i = 0 ; i < data_length; i++) {
//    OLA_DEBUG << "Got sample (after) " << ToHex(data[i]);
//  }

/*  {
    MutexLocker lock(&m_data_mu);
    while (!m_free_data.empty()) {
      U8 *data = m_free_data.front();
      DevicesManagerInterface::DeleteU8ArrayPtr(data);
      m_free_data.pop();
    }
  }*/
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
  // TODO(Peter): Stop the Sigrok thread
//  if (m_logic) {
//    m_logic->Stop();
//  }
}


/**
 * Called in the main thread.
 * @param data pointer to the data, ownership is transferred, use
 *   DeleteU8ArrayPtr to free.
 * @param data_length the size of the data
 */
void LogicReader::ProcessData(uint8_t *data, uint64_t data_length) {
//void LogicReader::ProcessData(uint64_t data_length) {
//void LogicReader::ProcessData(const sr_datafeed_logic *logic) {
  //OLA_DEBUG << "Got " << logic->length << " samples";
  OLA_DEBUG << "Got " << data_length << " samples";
//  for (unsigned int i = 0 ; i < data_length; i++) {
//    OLA_DEBUG << "Got sample " << ToHex(data[i]);
//  }
  m_signal_processor.Process(data, data_length, 0x01);
  //m_signal_processor.Process(copy_data, data_length, 0x01);
//  DevicesManagerInterface::DeleteU8ArrayPtr(data);

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
/*void OnConnect(U64 device_id, GenericInterface* device_interface,
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
}*/

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
               "Decode DMX/RDM data from devices supported by Sigrok");

  if (
    // (FLAGS_sigrok_log_level < SR_LOG_NONE) ||
      (FLAGS_sigrok_log_level > SR_LOG_SPEW)) {
    OLA_WARN << "Invalid sigrok log level, should be between " << SR_LOG_NONE
             << " and " << SR_LOG_NONE;
  }

  SelectServer ss;
  LogicReader reader(&ss, FLAGS_sample_rate);
  SigrokThread thread(&reader);

  //DevicesManagerInterface::RegisterOnConnect(&OnConnect, &reader);
  //DevicesManagerInterface::RegisterOnDisconnect(&OnDisconnect, &reader);
  //DevicesManagerInterface::BeginConnect();

/*  int ret;
  struct sr_context *sr_ctx;

  sr_log_loglevel_set(FLAGS_sigrok_log_level);

  if ((ret = sr_init(&sr_ctx)) != SR_OK) {
    OLA_FATAL << "Error initializing libsigrok (" << sr_strerror_name(ret) << "): " << sr_strerror(ret);
    return 1;
  }

  struct sr_session *sr_sess;
  if ((sr_sess = sr_session_new()) == NULL) {
    OLA_FATAL << "Error initializing libsigrok session";
    return 1;
  }
//  if ((ret = sr_session_new(sr_ctx, &sr_sess)) != SR_OK) {
//    OLA_FATAL << "Error initializing libsigrok session (" << sr_strerror_name(ret) << "): " << sr_strerror(ret);
//    return 1;
//  }

  struct sr_dev_driver **drivers;
  struct sr_dev_driver *driver = {};
  int i;

  drivers = sr_driver_list();
//  fail_unless(drivers != NULL, "No drivers found.");

  for (i = 0; drivers[i]; i++) {
    //OLA_DEBUG << "Found driver: " << drivers[i]->name;
    if (string(drivers[i]->name) == FLAGS_sigrok_device.str()) {
      OLA_INFO << "Got demo: " << drivers[i]->name << " - " << drivers[i]->longname;
      driver = drivers[i];
    }
  }

  if (driver == NULL) {
    OLA_FATAL << "Failed to find driver";
    return 1;
  }

  if ((ret = sr_driver_init(sr_ctx, driver)) != SR_OK) {
    OLA_FATAL << "Error initializing libsigrok driver " << driver->name << " (" << sr_strerror_name(ret) << "): " << sr_strerror(ret);
    return 1;
  }

  GSList *devlist;
  devlist = sr_driver_scan(driver, NULL);
  if (devlist == NULL) {
    OLA_FATAL << "Scanning with libsigrok driver " << driver->name << " didn't find any devices";
    return 1;
  } else {
    OLA_INFO << "Found " << g_slist_length(devlist) << " devices";
  }

  struct sr_dev_inst *sdi;
  //sdi = (sr_dev_inst*)devlist->data;
  sdi = (sr_dev_inst*)g_slist_nth_data(devlist, 0);
  g_slist_free(devlist);

  OLA_INFO << "Found device:";
  OLA_INFO << "\tVendor: " << (sdi->vendor ? sdi->vendor : "");
  OLA_INFO << "\tModel: " << (sdi->model ? sdi->model : "");
  OLA_INFO << "\tVersion: " << (sdi->version ? sdi->version : "");

  if ((ret = sr_dev_open(sdi)) != SR_OK) {
    OLA_FATAL << "Error opening device via libsigrok driver " << driver->name << " (" << sr_strerror_name(ret) << "): " << sr_strerror(ret);
    return 1;
  }

  if ((ret = sr_session_dev_add(sdi)) != SR_OK) {
    OLA_FATAL << "Error adding device to session via libsigrok driver " << driver->name << " (" << sr_strerror_name(ret) << "): " << sr_strerror(ret);
    return 1;
  }

  if ((ret = sr_config_set(sdi, NULL, SR_CONF_LIMIT_SAMPLES, g_variant_new_uint64(FLAGS_sigrok_samples))) != SR_OK) {
    OLA_FATAL << "Error setting config limit samples via libsigrok driver " << driver->name << " (" << sr_strerror_name(ret) << "): " << sr_strerror(ret);
    return 1;
  }

  // Use libsigrok functions here...

  if ((ret = sr_session_datafeed_callback_add(sigrok_feed_callback, &reader)) != SR_OK) {
    OLA_FATAL << "Error adding session datafeed callback via libsigrok (" << sr_strerror_name(ret) << "): " << sr_strerror(ret);
    return 1;
  }

  // Start acquisition on all devices.
  if ((ret = sr_session_start()) != SR_OK) {
    OLA_FATAL << "Error starting session (" << sr_strerror_name(ret) << "): " << sr_strerror(ret);
    return 1;
  }

  // Main loop, runs forever.
  sr_session_run();

  sr_session_stop();
  sr_session_dev_remove_all();

  //if ((ret = sr_session_destroy(sr_sess)) != SR_OK) {
  if ((ret = sr_session_destroy()) != SR_OK) {
    OLA_FATAL << "Error destroying libsigrok session (" << sr_strerror_name(ret) << "): " << sr_strerror(ret);
    return 1;
  }

  if ((ret = sr_exit(sr_ctx)) != SR_OK) {
    OLA_FATAL << "Error shutting down libsigrok (" << sr_strerror_name(ret) << "): " << sr_strerror(ret);
    return 1;
  }*/

  OLA_INFO << "Running...";
//  ss.RegisterSingleTimeout(3000, NewSingleCallback(DisplayReminder, &reader));
  thread.Start();
  ss.Run();
  thread.Stop();
  reader.Stop();
  return ola::EXIT_OK;
}
