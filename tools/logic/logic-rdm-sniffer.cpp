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

/**
 * Process a DMX signal.
 *
 * See E1.11 for the details. It generally goes something like
 *  Mark (Idle) - High
 *  Break - Low
 *  Mark After Break - High
 * start bit (low)
 * LSB to MSB
 * 2 stop bits (high)
 */
class DMXSignalProcessor {
  public:
    typedef Callback2<void, const uint8_t *data, unsigned int length>
      DataCallback;

    DMXSignalProcessor(DataCallback *callback,
                       unsigned int sample_rate)
        : m_callback(callbacks),
          m_sample_rate(sample_rate),
          m_state(IDLE),
          m_ticks(0)m
          m_microseconds_per_tick(1000000.0 / sample_rate) {
      if (m_sample_rate % DMX_BITRATE) {
        OLA_WARN << "Sample rate is not a multiple of " << DMX_BITRATE;
      }
    }

    // Reset the processor. Used if there is a gap in the stream.
    void Reset() {
      SetState(IDLE);
    }

    // Process more data.
    void Process(bool *ptr, unsigned int size) {
      for (unsigned int i = 0 ; i < size; i++) {
        ProcessBit(ptr[i]);
      }
    }

  private:
    enum State {
      UNDEFINED,  // when the signal is low and we have no idea where we are.
      IDLE,
      BREAK,
      MAB,
      START_BIT,
    }

    DataCallback *m_callback;
    unsigned int m_sample_rate;
    State m_state;
    unsigned int m_ticks;
    double m_microseconds_per_tick;

    void ProcessBit(bool bit) {
      switch (m_state) {
        case UNDEFINED:
          if (bit) {
            SetState(IDLE);
          }
          break;
        case IDLE:
          if (bit) {
            m_ticks++;
          } else {
            SetState(BREAK);
          }
          break;
        case BREAK:
          if (bit) {
            if (DurationExceeds(MIN_BREAK_TIME)) {
              SetState(MAB);
            } else {
              OLA_INFO << "Break too short, was "
                       << TicksAsMicroSeconds(m_ticks);
              SetState(IDLE);
            }
          } else {
            m_ticks++;
          }
          break;
        case MAB:
          if (bit) {
            m_ticks++;
            if (DurationExceeds(MAX_MAB_TIME)) {
              SetState(IDLE, m_ticks);
            }
          } else {
            if (DurationExceeds(MIN_MAB_TIME)) {
              SetState(START_BIT);
            } else {
              OLA_INFO << "Mark too short, was "
                       << TicksAsMicroSeconds(m_ticks);
              SetState(UNDEFINED);
            }
          }
          break;
        case START_BIT:
          if (bit) {

          } else {


          }
          break;
      }
    }

    // 
    void SetState(State state, unsigned int ticks = 1) {
      m_state = state;
      m_ticks = ticks;
    }

    // Return true if the current number of ticks exceeds micro_seconds.
    // Due to sampling this can be wrong by +- m_microseconds_per_tick.
    bool DurationExceeds(double micro_seconds) {
      return m_ticks * m_microseconds_per_tick > micro_seconds;
    }

    // Return the current number of ticks in microseconds.
    double TicksAsMicroSeconds() {
      return m_ticks * m_microseconds_per_tick;
    }

    static const unsigned int DMX_BITRATE = 250000;
    // These are all in microseconds.
    static const double MIN_BREAK_TIME = 88.0;
    static const double MIN_MAB_TIME = 1.0;
    static const double MAX_MAB_TIME = 1000000.0;
};

class LogicReader {
  public:
    explicit LogicReader(SelectServer *ss)
      : m_device_id(0),
        m_logic(NULL),
        m_ss(ss) {
    }

    void DeviceConnected(U64 device, GenericInterface *interface);
    void DeviceDisconnected(U64 device);
    void DataReceived(U64 device, const uint8_t *data, uint32_t data_length);

    void Stop();

  private:
    U64 m_device_id;  // GUARDED_BY(mu_);
    LogicInterface *m_logic;  // GUARDED_BY(mu_);
    Mutex m_mu;

    SelectServer *m_ss;

    void ProcessData(const uint8_t *data, uint32_t data_length);

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

/**
 * Called by the receive thread when new data arrives
 * @param device the device id which produced the data
 * @param data pointer to the data, ownership is transferred, use
 *   DeleteU8ArrayPtr to free.
 * @param data_length the size of the data
 */
void LogicReader::DataReceived(U64 device, const uint8_t *data,
                               uint32_t data_length) {
  {
    MutexLocker lock(&m_mu);
    if (device != m_device_id) {
      DevicesManagerInterface::DeleteU8ArrayPtr(data);
      return;
    }
  }
  m_ss->Execute(
      NewSingleCallback(this, &LogicReader::ProcessData, data, data_length));
}

/**
 *
 */
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
void ProcessData::ProcessData(const uint8_t *data, uint32_t data_length) {
  OLA_INFO << "processing data " << data_length;

  bool *
  for (uint32_t i =0; i < data_length; i++) {

  }

  DevicesManagerInterface::DeleteU8ArrayPtr(data);
}


// SaleaeDeviceApi callbacks
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

void OnReadData(U64 device_id, uint8_t *data, uint32_t data_length,
                void *user_data) {
  if (!user_data) {
    DevicesManagerInterface::DeleteU8ArrayPtr(data);
    return;
  }
  LogicReader *reader = (LogicReader*) user_data;  // NOLINT
  reader->DataReceived(device_id, data, data_length);
}

void OnError(U64 device_id, void *user_data) {
  OLA_INFO << "A device reported an Error.";
  (void) device_id;
  (void) user_data;
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
