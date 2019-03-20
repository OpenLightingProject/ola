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
#include "ola/Constants.h"

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

FtdiDmxThread::FtdiDmxThread(FtdiInterface *interface,
                             unsigned int frequency,
                             unsigned int serial)
  : m_timer("FtdiDmxThread"),
    m_granularity(UNKNOWN),
    m_interface(interface),
    m_term(false),
    m_frequency(frequency),
    m_transaction_number(0),
    m_discovery_agent(this),
    m_uid(OPEN_LIGHTING_ESTA_CODE, serial),
    m_pending_request(nullptr),
    m_rdm_callback(nullptr),
    m_mute_complete(nullptr),
    m_unmute_complete(nullptr),
    m_branch_callback(nullptr) {
  m_timer.setCaller("FtdiDmxThread " + m_interface->Description());

  if (serial == 0) {
    std::srand(std::time(nullptr));
    unsigned int deviceId = std::rand();
    OLA_WARN << "Setting Device ID to random value due to lack of serial: "
             << deviceId;
    m_uid = ola::rdm::UID(OPEN_LIGHTING_ESTA_CODE, deviceId);
  }
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

  if (m_pending_request != nullptr) {
    destroyPendingRequest();
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
  if (m_pending_request == nullptr) {
      m_pending_request = request;
      m_rdm_callback = callback;
  } else {
    OLA_WARN << "Unable to queue RDM request, RDM operation already pending";
  }
}

void FtdiDmxThread::RunFullDiscovery(ola::rdm::RDMDiscoveryCallback *callback) {
  m_discovery_agent.StartFullDiscovery(
      ola::NewSingleCallback(this,
                             &FtdiDmxThread::DiscoveryComplete,
                             callback));
}

void FtdiDmxThread::RunIncrementalDiscovery(rdm::RDMDiscoveryCallback *cb) {
  m_discovery_agent.StartIncrementalDiscovery(
        ola::NewSingleCallback(this,
                               &FtdiDmxThread::DiscoveryComplete,
                               cb));
}

/**
 * Called when the discovery process finally completes
 * @param callback the callback passed to StartFullDiscovery or
 * StartIncrementalDiscovery that we should execute.
 * @param status true if discovery worked, false otherwise
 * @param uids the UIDSet of UIDs that were found.
 */
void FtdiDmxThread::DiscoveryComplete(ola::rdm::RDMDiscoveryCallback *callback,
                                      bool status,
                                      const ola::rdm::UIDSet &uids) {
  if (status) {
    OLA_DEBUG << "FTDI discovery complete: " << uids;
  } else {
    OLA_WARN << "FTDI discovery failed";
  }
  if (callback) {
    callback->Run(uids);
  }
}

/**
 * @brief Method called to cleanup any outstanding callbacks
 * @param state state to return to caller when possible.
 *
 * @note All callbacks except the RDMCallback lack a way of reporting an error
 * state to the caller.
 */
void FtdiDmxThread::destroyPendindingCallback(ola::rdm::RDMStatusCode state) {
  MuteDeviceCallback *thread_mute_callback = nullptr;
  UnMuteDeviceCallback *thread_unmute_callback = nullptr;
  BranchCallback *thread_branch_callback = nullptr;
  ola::rdm::RDMCallback *thread_rdm_callback = nullptr;

  destroyPendingRequest();

  if (m_mute_complete != nullptr) {
    thread_mute_callback = m_mute_complete;
    m_mute_complete = nullptr;
    thread_mute_callback->Run(false);
  } else if (m_unmute_complete != nullptr) {
    thread_unmute_callback = m_unmute_complete;
    m_unmute_complete = nullptr;
    thread_unmute_callback->Run();
  } else if (m_branch_callback != nullptr) {
    thread_branch_callback = m_branch_callback;
    m_branch_callback = nullptr;
    thread_branch_callback->Run(nullptr, 0);
  } else if (m_rdm_callback != nullptr) {
    thread_rdm_callback = m_rdm_callback;
    m_rdm_callback = nullptr;
    ola::rdm::RunRDMCallback(thread_rdm_callback, state);
  }
}

void FtdiDmxThread::destroyPendingRequest() {
  ola::rdm::RDMRequest *tmp_rdm_request = m_pending_request;

  m_pending_request = nullptr;
  delete tmp_rdm_request;
}

void FtdiDmxThread::MuteDevice(const ola::rdm::UID &target,
                               MuteDeviceCallback *mute_complete) {
  ola::thread::MutexLocker locker(&m_rdm_mutex);
  if (m_pending_request == nullptr) {
    OLA_INFO << "Muting device";
    m_mute_complete = mute_complete;
    m_pending_request = ola::rdm::NewMuteRequest(m_uid,
                                                 target,
                                                 m_transaction_number);
    m_transaction_number++;
  } else {
    // Already pending request
    OLA_WARN << "Unable to queue Mute request, "
             << "RDM operation already pending";
  }
}

void FtdiDmxThread::UnMuteAll(UnMuteDeviceCallback *unmute_complete) {
  ola::thread::MutexLocker locker(&m_rdm_mutex);
  if (m_pending_request == nullptr) {
    OLA_INFO << "Sending UnMuteAll";
    m_unmute_complete = unmute_complete;
    m_pending_request = ola::rdm::NewUnMuteRequest(m_uid,
                                                   ola::rdm::UID::AllDevices(),
                                                   m_transaction_number);
    m_transaction_number++;
  } else {
    // Already pending request
    OLA_WARN << "Unable to queue UnMuteAll request, "
             << "RDM operation already pending";
  }
}

void FtdiDmxThread::Branch(const ola::rdm::UID &lower,
                           const ola::rdm::UID &upper,
                           BranchCallback *callback) {
  ola::thread::MutexLocker locker(&m_rdm_mutex);
  if (m_pending_request == nullptr) {
    OLA_INFO << "Sending branch";
    m_branch_callback = callback;
    m_pending_request =
        ola::rdm::NewDiscoveryUniqueBranchRequest(m_uid,
                                                  lower,
                                                  upper,
                                                  m_transaction_number);
    m_transaction_number++;
  } else {
    // Already pending request
    OLA_WARN << "Unable to queue Branch request, "
             << "RDM operation already pending";
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
  unsigned int additionalWait = 0;
  unsigned char readBuffer[258];
  ola::io::ByteString packetBuffer;

  MuteDeviceCallback *thread_mute_callback = nullptr;
  UnMuteDeviceCallback *thread_unmute_callback = nullptr;
  BranchCallback *thread_branch_callback = nullptr;
  ola::rdm::RDMCallback *thread_rdm_callback = nullptr;

  ola::rdm::RDMReply *received_reply = nullptr;

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
    if (m_pending_request != nullptr) {
      elapsed = ts1 - lastDMX;

      if (elapsed.InMilliSeconds() < HALF_SECOND_MS) {
        if (!packetBuffer.empty()) {
          packetBuffer.clear();
        }

        if (!ola::rdm::RDMCommandSerializer::PackWithStartCode(
              *m_pending_request, &packetBuffer)) {
          OLA_WARN << "RDMCommandSerializer failed. Dropping packet.";

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
      m_timer.usleep(DMX_BREAK);
    }

    if (!m_interface->SetBreak(false)) {
      goto framesleep;
    }

    if (m_granularity == GOOD) {
      m_timer.usleep(DMX_MAB);
    }

    if (!sendRDM) {
      if (!m_interface->Write(buffer)) {
        goto framesleep;
      } else {
        clock.CurrentTime(&lastDMX);
      }
    } else {
      if (m_interface->Write(&packetBuffer)) {
          OLA_INFO << "RDM packet written to line";
          if (m_pending_request->IsDUB()) {
            m_timer.usleep(MIN_WAIT_DUB_US);

            readBytes = m_interface->Read(readBuffer, sizeof(readBuffer));
            OLA_INFO << "DUB Read: " << readBytes;
            if (m_branch_callback != nullptr) {
              thread_branch_callback = m_branch_callback;
              m_branch_callback = nullptr;
              destroyPendingRequest();
              thread_branch_callback->Run(readBuffer,
                                          (readBytes >= 0 ? readBytes : 0));
            }
          } else if (!m_pending_request->DestinationUID().IsBroadcast()) {
            m_timer.usleep(MIN_WAIT_RDM_US);
            readBytes = m_interface->Read(readBuffer, sizeof(readBuffer));

            if (readBytes > 0) {
              /*
               * The following section of code tries to deal with replies that
               * are being broadcast too slowly, I don't have equipment that
               * malfunctions in this way so I have no way of testing this.
               */
              if (readBytes < 4) {
                OLA_WARN << "FTDI Didn't receive at least 4B during minWait";
                additionalWait = (MIN_WAIT_RDM_US / readBytes)*(4 - readBytes);
                m_timer.usleep(additionalWait);
                readBytes += m_interface->Read(readBuffer + readBytes,
                                               sizeof(readBuffer) - readBytes);
              }
              /*
               * This section of code does minimal verification of the received
               * frame.
               * This assumes that the 4th byte in the buffer is the 3rd byte of
               * the RDM Frame which defines length and no checksum has been done
               * yet.
               */
              if (readBytes >= 4) {
                if (readBuffer[0] == 0x00 && readBuffer[1] == 0xcc) {
                  while (((readBytes - 1) < readBuffer[3]) &&
                         elapsed.InMilliSeconds() <= 1250) {
                    OLA_WARN << "FTDI Didn't receive full frame during minWait";
                    additionalWait = (MIN_WAIT_RDM_US / readBytes) *
                        (readBuffer[3] - readBytes + 1);
                    m_timer.usleep(additionalWait);
                    readBytes += m_interface->Read(
                          readBuffer + readBytes,
                          sizeof(readBuffer) - readBytes);

                    clock.CurrentTime(&ts2);
                    elapsed = ts2 - ts1;
                  }
                  if (readBytes < (readBuffer[3] + 1)) {
                    OLA_WARN << "Discarding due to timeout.";
                    destroyPendindingCallback(rdm::RDM_TIMEOUT);
                  } else {
                    received_reply = rdm::RDMReply::FromFrame(
                          rdm::RDMFrame(readBuffer+1, readBytes-1),
                          m_pending_request);

                    if (received_reply != nullptr) {
                      if (m_mute_complete != nullptr) {
                        thread_mute_callback = m_mute_complete;
                        m_mute_complete = nullptr;

                        if (received_reply->Response()->SourceUID() ==
                            m_pending_request->DestinationUID()) {
                          destroyPendingRequest();
                          thread_mute_callback->Run(true);
                        } else {
                          destroyPendingRequest();
                          thread_mute_callback->Run(false);
                        }
                      } else if (m_rdm_callback != nullptr) {
                        thread_rdm_callback = m_rdm_callback;
                        m_rdm_callback = nullptr;

                        if (readBytes > 0) {
                          destroyPendingRequest();
                          thread_rdm_callback->Run(received_reply);
                        } else {
                          destroyPendingRequest();
                          RunRDMCallback(thread_rdm_callback, rdm::RDM_TIMEOUT);
                        }
                      }
                    } else {
                      OLA_WARN << "received reply is nullptr";
                      destroyPendindingCallback(rdm::RDM_INVALID_RESPONSE);
                    }
                    // Reset reply buffer.
                    if (received_reply != nullptr) {
                      rdm::RDMReply *tmp = received_reply;
                      received_reply = nullptr;
                      delete tmp;
                    }
                  }  // End handling seemingly valid data
                } else {
                  destroyPendindingCallback(rdm::RDM_INVALID_RESPONSE);
                }
              } else {
                destroyPendindingCallback(rdm::RDM_TIMEOUT);
              }
            } else {
              destroyPendindingCallback(rdm::RDM_TIMEOUT);
            }
          } else {
            if (m_unmute_complete != nullptr) {
              thread_unmute_callback = m_unmute_complete;
              m_unmute_complete = nullptr;
              destroyPendingRequest();
              thread_unmute_callback->Run();
            } else if (m_rdm_callback != nullptr) {
              thread_rdm_callback = m_rdm_callback;
              m_rdm_callback = nullptr;
              destroyPendingRequest();
              ola::rdm::RunRDMCallback(thread_rdm_callback,
                                       ola::rdm::RDM_WAS_BROADCAST);
            }
          }
      } else {
        /* Something went wrong, already reported at hw level
         * but we'll need to handle the callbacks.
         */
        destroyPendindingCallback(ola::rdm::RDM_FAILED_TO_SEND);
      }  // End of Write loop */

      goto framesleep;
    }

  framesleep:
    // Sleep for the remainder of the DMX frame time
    clock.CurrentTime(&ts2);
    elapsed = ts2 - ts1;

    if (m_granularity == GOOD) {
      while (elapsed.InMilliSeconds() < frameTime) {
        m_timer.usleep(1000);
        clock.CurrentTime(&ts2);
        elapsed = ts2 - ts1;
      }
    } else {
      // See if we can drop out of bad mode.
      CheckTimeGranularity();
      m_timer.usleep(1000);
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
  m_timer.CheckTimeGranularity(8, 4);
  m_granularity = m_timer.getGranularity();
}
}  // namespace ftdidmx
}  // namespace plugin
}  // namespace ola
