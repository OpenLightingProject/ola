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
 * SlpThread.cpp
 * Copyright (C) 2011 Simon Newton
 */

#include <slp.h>
#include <ola/Logging.h>
#include <sstream>
#include <string>
#include "SlpThread.h"


using std::string;

// The service name we use for SLP
const char BaseSlpAction::SERVICE_NAME[] = "esta.e133";


void RegisterCallback(SLPHandle slp_handle, SLPError errcode, void* cookie) {
  SLPError *error = static_cast<SLPError*>(cookie);
  *error = errcode;
  (void) slp_handle;
}


/**
 * This is what we pass as a void* to the slp callback
 */
typedef struct {
  SLPError error;
  url_vector *urls;
} slp_cookie;


/*
 * This is called by the SLP library.
 */
SLPBoolean ServiceCallback(SLPHandle slp_handle,
                           const char* srvurl,
                           unsigned short lifetime,
                           SLPError errcode,
                           void *raw_cookie) {
  slp_cookie *cookie = static_cast<slp_cookie*>(raw_cookie);

  if (errcode == SLP_OK) {
    cookie->urls->push_back(string(srvurl));
    cookie->error = SLP_OK;
  } else if (errcode == SLP_LAST_CALL) {
    cookie->error = SLP_OK;
  } else {
    cookie->error = errcode;
  }

  return SLP_TRUE;
  (void) slp_handle;
  (void) lifetime;
}


/**
 * Perform the discovery action
 */
void SlpDiscoveryAction::Perform(SLPHandle handle) {
  slp_cookie cookie;
  cookie.urls = m_urls;

  SLPError err = SLPFindSrvs(handle,
                             SERVICE_NAME,
                             0,  // use configured scopes
                             0,  // no attr filter
                             ServiceCallback,
                             &cookie);

  OLA_INFO << "slp call returned";
  m_ok = true;

  if ((err != SLP_OK)) {
    OLA_INFO << "Error finding service with slp " << err;
    m_ok = false;
  }

  if (cookie.error != SLP_OK) {
    OLA_INFO << "Error finding service with slp " << cookie.error;
    m_ok = false;
  }
}


/**
 * Perform the register action
 */
void SlpRegistrationAction::Perform(SLPHandle handle) {
  unsigned short s = SLPGetRefreshInterval();
  OLA_INFO << "min interval is " << s;

  std::stringstream str;
  str << "service:" << SERVICE_NAME << "://" << m_url;
  SLPError callbackerr;
  SLPError err = SLPReg(handle,
                        str.str().c_str(),
                        m_lifetime,
                        0,
                        "",
                        SLP_TRUE,
                        RegisterCallback,
                        &callbackerr);


  if ((err != SLP_OK)) {
    OLA_INFO << "Error registering service with slp " << err;
    m_ok = false;
  }

  if (callbackerr != SLP_OK) {
    OLA_INFO << "Error registering service with slp " << callbackerr;
    m_ok = false;
  }
}


/**
 * Perform the deregister action
 */
void SlpDeRegistrationAction::Perform(SLPHandle handle) {
  std::stringstream str;
  str << "service:" << SERVICE_NAME << "://" << m_url;
  SLPError callbackerr;
  SLPError err = SLPDereg(handle,
                          str.str().c_str(),
                          RegisterCallback,
                          &callbackerr);


  if ((err != SLP_OK)) {
    OLA_INFO << "Error deregistering service with slp " << err;
    m_ok = false;
  }

  if (callbackerr != SLP_OK) {
    OLA_INFO << "Error deregistering service with slp " << callbackerr;
    m_ok = false;
  }
}


/**
 * Create a new resolver thread. This doesn't actually start it.
 * @param ss the select server to use to handle the callbacks.
 */
SlpThread::SlpThread(ola::network::SelectServer *ss)
  : OlaThread(),
    m_main_ss(ss),
    m_init_ok(false) {
  m_incoming_socket.SetOnData(ola::NewCallback(this, &SlpThread::NewRequest));
  m_outgoing_socket.SetOnData(ola::NewCallback(this,
                                               &SlpThread::RequestComplete));
  pthread_mutex_init(&m_incomming_mutex, NULL);
  pthread_mutex_init(&m_outgoing_mutex, NULL);
}


/**
 * Clean up
 */
SlpThread::~SlpThread() {
  if (m_init_ok)
    SLPClose(m_slp_handle);
  pthread_mutex_destroy(&m_incomming_mutex);
  pthread_mutex_destroy(&m_outgoing_mutex);
}


/**
 * Setup the SLP Thread
 */
bool SlpThread::Init() {
  if (m_init_ok)
    return true;

  if (!m_incoming_socket.Init())
    return false;

  if (!m_outgoing_socket.Init()) {
    m_incoming_socket.Close();
    return false;
  }

  SLPError err = SLPOpen("en", SLP_FALSE, &m_slp_handle);
  if (err != SLP_OK) {
    OLA_INFO << "Error opening slp handle " << err;
    m_incoming_socket.Close();
    m_outgoing_socket.Close();
    return false;
  }
  m_ss.AddSocket(&m_incoming_socket);
  m_main_ss->AddSocket(&m_outgoing_socket);
  m_init_ok = true;
  return true;
}


/**
 * Start the slp resolver thread
 */
bool SlpThread::Start() {
  if (!m_init_ok)
    return false;
  return OlaThread::Start();
}


/**
 * Stop the resolver thread
 */
bool SlpThread::Join(void *ptr) {
  m_ss.Terminate();
  // kick the select server so the wake up is immediate
  WakeUpSocket(&m_incoming_socket);
  return OlaThread::Join(ptr);
}


/**
 * Schedule a discovery.
 * @param callback the callback to be invoked once the discovery is complete
 * @param urls a pointer to a vector which will be populated with the urls we
 * find.
 *
 * This call returns immediately. Upon completion, the callback will be invoked
 * in the thread running the select server passed to SlpThread's constructor.
 */
void SlpThread::Discover(slp_discovery_callback *callback,
                         url_vector *urls) {
  SlpDiscoveryAction *action = new SlpDiscoveryAction(callback, urls);
  pthread_mutex_lock(&m_incomming_mutex);
  m_incoming_queue.push(action);
  pthread_mutex_unlock(&m_incomming_mutex);

  WakeUpSocket(&m_incoming_socket);
}


/**
 * Register a url.
 * This registers a url with slpd, which either registers it with the DA if
 * there is one, or responds to Findsrv requests as a SA.
 * @param callback the callback to run when the registration is complete
 * @param url the url to register
 * @param lifetime the time in seconds for this registration to remain valid
 */
void SlpThread::Register(slp_registration_callback *callback,
                         const string &url,
                         unsigned short lifetime) {
  SlpRegistrationAction *action = new SlpRegistrationAction(
    callback,
    url,
    lifetime);
  pthread_mutex_lock(&m_incomming_mutex);
  m_incoming_queue.push(action);
  pthread_mutex_unlock(&m_incomming_mutex);

  WakeUpSocket(&m_incoming_socket);
}


/**
 * DeRegister a url.
 * This deregisters a url with slpd, which either deregisters it with the DA if
 * there is one, or stops responding to Findsrv requests as a SA.
 * @param callback the callback to run when the registration is complete
 * @param url the url to register
 */
void SlpThread::DeRegister(slp_registration_callback *callback,
                           const string &url) {
  SlpDeRegistrationAction *action = new SlpDeRegistrationAction(
    callback,
    url);
  pthread_mutex_lock(&m_incomming_mutex);
  m_incoming_queue.push(action);
  pthread_mutex_unlock(&m_incomming_mutex);

  WakeUpSocket(&m_incoming_socket);
}


/**
 * Entry point to the new thread
 */
void *SlpThread::Run() {
  m_ss.Run();
  return NULL;
}


/**
 * Called when a new request arrives
 */
void SlpThread::NewRequest() {
  EmptySocket(&m_incoming_socket);

  while (true) {
    pthread_mutex_lock(&m_incomming_mutex);
    if (m_incoming_queue.empty()) {
      pthread_mutex_unlock(&m_incomming_mutex);
      return;
    }

    OLA_INFO << "new slp action";

    BaseSlpAction *action = m_incoming_queue.front();
    m_incoming_queue.pop();
    pthread_mutex_unlock(&m_incomming_mutex);

    // this blocks and can take a while
    action->Perform(m_slp_handle);

    pthread_mutex_lock(&m_outgoing_mutex);
    m_outgoing_queue.push(action);
    pthread_mutex_unlock(&m_outgoing_mutex);
    WakeUpSocket(&m_outgoing_socket);
  }
}


/**
 * Called in the main thread when a request completes
 */
void SlpThread::RequestComplete() {
  OLA_INFO << "request complete";
  EmptySocket(&m_outgoing_socket);

  while (true) {
    pthread_mutex_lock(&m_outgoing_mutex);
    if (m_outgoing_queue.empty()) {
      pthread_mutex_unlock(&m_outgoing_mutex);
      return;
    }

    BaseSlpAction *action = m_outgoing_queue.front();
    m_outgoing_queue.pop();
    pthread_mutex_unlock(&m_outgoing_mutex);

    action->RequestComplete();
    delete action;
  }
}


/**
 * Wake up the select server on the other end of this socket by sending some
 * data.
 */
void SlpThread::WakeUpSocket(ola::network::LoopbackSocket *socket) {
  uint8_t wake_up = 'a';
  socket->Send(&wake_up, sizeof(wake_up));
}


/**
 * Read (and discard) any pending data on a socket
 */
void SlpThread::EmptySocket(ola::network::LoopbackSocket *socket) {
  while (socket->DataRemaining()) {
    uint8_t message;
    unsigned int size;
    socket->Receive(&message, sizeof(message), size);
  }
}
