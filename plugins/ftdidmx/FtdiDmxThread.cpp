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
 * FtdiDmxThread.cpp
 * The FTDI usb chipset DMX plugin for ola
 * Copyright (C) 2011 Rui Barreiros
 *
 * Additional modifications to enable support for multiple outputs and
 * additional device ids did change the original structure.
 *
 * by E.S. Rosenberg a.k.a. Keeper of the Keys 5774/2014
 */

#include <math.h>
#include <unistd.h>

#include <string>
#include <queue>
#include <utility>

#include "ola/Clock.h"
#include "ola/Logging.h"
#include "ola/StringUtils.h"

#include "ola/rdm/RDMCommand.h"
#include "ola/rdm/RDMControllerInterface.h"
#include "ola/rdm/RDMCommandSerializer.h"
#include "ola/rdm/RDMResponseCodes.h"
#include "ola/rdm/DiscoveryAgent.h"

#include "plugins/ftdidmx/FtdiWidget.h"
#include "plugins/ftdidmx/FtdiDmxThread.h"

namespace ola {
namespace plugin {
namespace ftdidmx {

FtdiDmxThread::FtdiDmxThread(FtdiInterface *interface, unsigned int frequency)
  : m_granularity(UNKNOWN),
    m_interface(interface),
    m_term(false),
    m_frequency(frequency),
    m_transaction_number(0),
    m_discovery_agent(this),
    m_uid(0x7a70, 0x12345678),
    m_pending_request(nullptr),
    m_rdm_callback(nullptr),
    m_mute_complete(nullptr),
    m_unmute_complete(nullptr),
    m_branch_callback(nullptr) {
}

FtdiDmxThread::~FtdiDmxThread() {
  Stop();
}


/**
 * @brief Stop this thread
 */
bool FtdiDmxThread::Stop() {

  {
    ola::thread::MutexLocker locker(&m_term_mutex);
    m_term = true;
  }

  if(m_pending_request != nullptr) {
    m_pending_request = nullptr;
    destroyPendindingCallback(ola::rdm::RDM_FAILED_TO_SEND);
  }
  m_discovery_agent.Abort();
  return Join();
}


/**
 * @brief Copy a DMXBuffer to the output thread
 */
bool FtdiDmxThread::WriteDMX(const DmxBuffer &buffer) {
  {
    ola::thread::MutexLocker locker(&m_buffer_mutex);
    m_buffer.Set(buffer);
    return true;
  }
}

void FtdiDmxThread::SendRDMRequest(ola::rdm::RDMRequest *request,
                    ola::rdm::RDMCallback *callback) {
  ola::thread::MutexLocker locker(&m_rdm_mutex);
  if(m_pending_request == nullptr) {
      m_pending_request = request;
      m_rdm_callback = callback;
  } else {
    OLA_WARN << "Unable to queue RDM request, RDM operation already pending";
  }
  OLA_WARN << "Function not properly implemented yet, callback may not get called.";
}

void FtdiDmxThread::RunFullDiscovery(ola::rdm::RDMDiscoveryCallback *callback) {
  m_discovery_agent.StartFullDiscovery(ola::NewSingleCallback(this, &FtdiDmxThread::DiscoveryComplete, callback));
}

void FtdiDmxThread::RunIncrementalDiscovery(ola::rdm::RDMDiscoveryCallback *callback) {
  m_discovery_agent.StartIncrementalDiscovery(ola::NewSingleCallback(this, &FtdiDmxThread::DiscoveryComplete, callback));
}

/**
 * Called when the discovery process finally completes
 * @param callback the callback passed to StartFullDiscovery or
 * StartIncrementalDiscovery that we should execute.
 * @param status true if discovery worked, false otherwise
 * @param uids the UIDSet of UIDs that were found.
 */
void FtdiDmxThread::DiscoveryComplete(ola::rdm::RDMDiscoveryCallback *callback,
                                       bool,
                                       const ola::rdm::UIDSet &uids) {
  OLA_DEBUG << "FTDI discovery complete: " << uids;
  if (callback) {
    callback->Run(uids);
  }
}

/**
 * @brief Method called to cleanup any outstanding callbacks
 * @param state
 *
 * All callbacks except the RDMCallback lack a way of reporting an error state to the caller.
 */
void FtdiDmxThread::destroyPendindingCallback(ola::rdm::RDMStatusCode state) {
  MuteDeviceCallback *thread_mute_callback = nullptr;
  UnMuteDeviceCallback *thread_unmute_callback = nullptr;
  BranchCallback *thread_branch_callback = nullptr;
  ola::rdm::RDMCallback *thread_rdm_callback = nullptr;

  if(m_mute_complete != nullptr) {
    thread_mute_callback = m_mute_complete;
    m_mute_complete = nullptr;
    thread_mute_callback->Run(false);
  } else if(m_unmute_complete != nullptr) {
    thread_unmute_callback = m_unmute_complete;
    m_unmute_complete = nullptr;
    thread_unmute_callback->Run();
  } else if(m_branch_callback != nullptr) {
    thread_branch_callback = m_branch_callback;
    m_branch_callback = nullptr;
    thread_branch_callback->Run(nullptr, 0);
  } else if(m_rdm_callback != nullptr) {
    thread_rdm_callback = m_rdm_callback;
    m_rdm_callback = nullptr;
    ola::rdm::RunRDMCallback(thread_rdm_callback, state);
  }
}

void FtdiDmxThread::MuteDevice(const ola::rdm::UID &target,
                               MuteDeviceCallback *mute_complete) {
  ola::thread::MutexLocker locker(&m_rdm_mutex);
  if(m_pending_request == nullptr) {
    OLA_INFO << "Muting device";
    m_mute_complete = mute_complete;
    m_pending_request = ola::rdm::NewMuteRequest(m_uid, target, m_transaction_number += 1);
  } else {
      // Already pending request
    OLA_WARN << "Unable to queue Mute request, RDM operation already pending";
  }
}

void FtdiDmxThread::UnMuteAll(UnMuteDeviceCallback *unmute_complete) {
  ola::thread::MutexLocker locker(&m_rdm_mutex);
  if(m_pending_request == nullptr) {
    OLA_INFO << "Sending UnMuteAll";
    m_unmute_complete = unmute_complete;
    m_pending_request = ola::rdm::NewUnMuteRequest(m_uid, ola::rdm::UID::AllDevices(), m_transaction_number += 1);
  } else {
      // Already pending request
    OLA_WARN << "Unable to queue UnMuteAll request, RDM operation already pending";
  }
}

void FtdiDmxThread::Branch(const ola::rdm::UID &lower,
                           const ola::rdm::UID &upper,
                           BranchCallback *callback) {
  ola::thread::MutexLocker locker(&m_rdm_mutex);
  if(m_pending_request == nullptr) {
    OLA_INFO << "Sending branch";
    m_branch_callback = callback;
    m_pending_request = ola::rdm::NewDiscoveryUniqueBranchRequest(m_uid, lower, upper, m_transaction_number += 1);
  } else {
      // Already pending request
    OLA_WARN << "Unable to queue Branch request, RDM operation already pending";
  }
}

/**
 * @brief The method called by the thread
 */
void *FtdiDmxThread::Run() {
  OLA_INFO << "Starting FtdiDmxThread";
  TimeStamp ts1, ts2, ts3, lastDMX;
  Clock clock;
  CheckTimeGranularity();
  DmxBuffer buffer;
  bool sendRDM = false;
  TimeInterval elapsed, interval;
  int readBytes;
  unsigned char readBuffer[258];
  ola::io::ByteString packetBuffer;

  MuteDeviceCallback *thread_mute_callback = nullptr;
  UnMuteDeviceCallback *thread_unmute_callback = nullptr;
  BranchCallback *thread_branch_callback = nullptr;
  ola::rdm::RDMCallback *thread_rdm_callback = nullptr;
  ola::rdm::RDMRequest *thread_pending_request = nullptr;


  int frameTime = static_cast<int>(floor(
    (static_cast<double>(1000) / m_frequency) + static_cast<double>(0.5)));

  // Setup the interface
  if (!m_interface->IsOpen()) {
    m_interface->SetupOutput();
  }

  while (1) {
    {
      ola::thread::MutexLocker locker(&m_term_mutex);
      if (m_term) {
        break;
      }
    }

    {
      ola::thread::MutexLocker locker(&m_buffer_mutex);
      buffer.Set(m_buffer);
    }

    clock.CurrentTime(&ts1);
    if(m_pending_request != nullptr) {

      elapsed = ts1 - lastDMX;

      if(elapsed.InMilliSeconds() < 500) {
        if(!packetBuffer.empty()) {
          packetBuffer.clear();
        }

        if(!ola::rdm::RDMCommandSerializer::PackWithStartCode(*m_pending_request, &packetBuffer)) {
          OLA_WARN << "RDMCommandSerializer failed. Dropping packet.";
          m_pending_request = nullptr;

          destroyPendindingCallback(ola::rdm::RDM_FAILED_TO_SEND);
          sendRDM = false;
        } else {
          OLA_INFO << "OK To send RDM";
          sendRDM = true;
        }
      } else {
        OLA_INFO << "NOK to send RDM (DMX interval)";
        sendRDM = false;
      }
    } else {
      sendRDM = false;
    }

    if (!m_interface->SetBreak(true)) {
      goto framesleep;
    }

    if (m_granularity == GOOD) {
      usleep(DMX_BREAK);
    }

    if (!m_interface->SetBreak(false)) {
      goto framesleep;
    }

    if (m_granularity == GOOD) {
      usleep(DMX_MAB);
    }

    if(!sendRDM) {
      if (!m_interface->Write(buffer)) {
        goto framesleep;
      } else {
        clock.CurrentTime(&lastDMX);
      }
    } else {
      if(!m_pending_request->DestinationUID().IsBroadcast() || m_pending_request->IsDUB()) {
        if((readBytes = m_interface->WriteAndRead(&packetBuffer, readBuffer, sizeof(readBuffer), (m_pending_request->IsDUB() ? 58000 : 30000))) >= 0) {
          if(m_pending_request->IsDUB()) {
            if(m_branch_callback != nullptr) {
              thread_branch_callback = m_branch_callback;
              m_branch_callback = nullptr;
              m_pending_request = nullptr;
              thread_branch_callback->Run(readBuffer, readBytes);
            }
          } else {
              if(m_mute_complete != nullptr) {
                thread_mute_callback = m_mute_complete;
                m_mute_complete = nullptr;

                if(readBytes > 0 &&
                   rdm::RDMReply::FromFrame(rdm::RDMFrame(readBuffer+1, readBytes-1))->Response()->SourceUID() == m_pending_request->DestinationUID()) {
                  m_pending_request = nullptr;
                  OLA_INFO << "Mute callback(true)";
                  thread_mute_callback->Run(true);
                } else {
                  m_pending_request = nullptr;
                  OLA_INFO << "Mute callback(false)";
                  thread_mute_callback->Run(false);
                }
              } else if(m_rdm_callback != nullptr) {
                thread_rdm_callback = m_rdm_callback;
                m_rdm_callback = nullptr;

                if(readBytes > 0) {
                  thread_pending_request = m_pending_request;
                  m_pending_request = nullptr;
                  thread_rdm_callback->Run(rdm::RDMReply::FromFrame(rdm::RDMFrame(readBuffer+1, readBytes-1), thread_pending_request));
                } else {
                  m_pending_request = nullptr;
                  RunRDMCallback(thread_rdm_callback, rdm::RDM_TIMEOUT);
                }
              }
            }

        } else {
          // Something went wrong, already reported at hw level but we'll need to handle the callbacks
          // Strictly speaking we failed to receive OR send, I have proposed another code: RDM_HW_ERROR
          destroyPendindingCallback(ola::rdm::RDM_FAILED_TO_SEND);
        }
      } else if(m_interface->Write(&packetBuffer)) {
        if(m_unmute_complete != nullptr) {
          thread_unmute_callback = m_unmute_complete;
          m_unmute_complete = nullptr;
          OLA_INFO << "UnMuteAllCallback";
          m_pending_request = nullptr;
          thread_unmute_callback->Run();
        }
      }

/*      if(m_interface->Write(&packetBuffer)) {
          OLA_INFO << "RDM packet";
          if(m_pending_request->IsDUB()) {
            usleep(58000); //min time before next packet broadcast allowed
            readBytes = m_interface->Read(readBuffer, 258);
            OLA_INFO << "DUB Read: " << readBytes;
            if(m_branch_callback != nullptr) {
              thread_branch_callback = m_branch_callback;
              m_branch_callback = nullptr;
              m_pending_request = nullptr;
              thread_branch_callback->Run(readBuffer, readBytes);
            }
          }
          else if(!m_pending_request->DestinationUID().IsBroadcast()) {

            usleep(30000); //min time before next packet allowed
            readBytes = m_interface->Read(readBuffer, 258);

            if(m_mute_complete != nullptr) {
              thread_mute_callback = m_mute_complete;
              m_mute_complete = nullptr;

              if(rdm::RDMReply::FromFrame(rdm::RDMFrame(readBuffer, readBytes))->Response()->SourceUID() == m_pending_request->DestinationUID()) {
                m_pending_request = nullptr;
                thread_mute_callback->Run(true);
              } else {
                m_pending_request = nullptr;
                thread_mute_callback->Run(false);
              }
            } else if(m_rdm_callback != nullptr) {
              thread_rdm_callback = m_rdm_callback;
              m_rdm_callback = nullptr;

              if(readBytes > 0) {
                thread_pending_request = m_pending_request;
                m_pending_request = nullptr;
                thread_rdm_callback->Run(rdm::RDMReply::FromFrame(rdm::RDMFrame(readBuffer, readBytes), thread_pending_request));
              } else {
                m_pending_request = nullptr;
                RunRDMCallback(thread_rdm_callback, rdm::RDM_TIMEOUT);
              }
            }

          } else {
            if(m_unmute_complete != nullptr) {
              thread_unmute_callback = m_unmute_complete;
              m_unmute_complete = nullptr;
              OLA_INFO << "UnMuteAllCallback";
              m_pending_request = nullptr;
              thread_unmute_callback->Run();
            }
          }
      } else {
        if(m_branch_callback != nullptr) {

        } else if(m_mute_complete != nullptr) {

        } else if(m_unmute_complete != nullptr) {

        } else if(m_rdm_callback != nullptr) {

        }
      } */

      goto framesleep;
    }

  framesleep:
    // Sleep for the remainder of the DMX frame time
    clock.CurrentTime(&ts2);
    elapsed = ts2 - ts1;

    if (m_granularity == GOOD) {
      while (elapsed.InMilliSeconds() < frameTime) {
        usleep(1000);
        clock.CurrentTime(&ts2);
        elapsed = ts2 - ts1;
      }
    } else {
      // See if we can drop out of bad mode.
      usleep(1000);
      clock.CurrentTime(&ts3);
      interval = ts3 - ts2;
      if (interval.InMilliSeconds() < BAD_GRANULARITY_LIMIT) {
        m_granularity = GOOD;
        OLA_INFO << "Switching from BAD to GOOD granularity for ftdi thread";
      }

      elapsed = ts3 - ts1;
      while (elapsed.InMilliSeconds() < frameTime) {
        clock.CurrentTime(&ts2);
        elapsed = ts2 - ts1;
      }
    }
  }
  return NULL;
}


/**
 * @brief Check the granularity of usleep.
 */
void FtdiDmxThread::CheckTimeGranularity() {
  TimeStamp ts1, ts2;
  Clock clock;

  clock.CurrentTime(&ts1);
  usleep(1000);
  clock.CurrentTime(&ts2);

  TimeInterval interval = ts2 - ts1;
  m_granularity = (interval.InMilliSeconds() > BAD_GRANULARITY_LIMIT) ?
      BAD : GOOD;
  OLA_INFO << "Granularity for FTDI thread is "
           << ((m_granularity == GOOD) ? "GOOD" : "BAD");
}
}  // namespace ftdidmx
}  // namespace plugin
}  // namespace ola
