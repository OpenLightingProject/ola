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
 * OpenSLPThread.cpp
 * Copyright (C) 2011 Simon Newton
 */

#include <slp.h>
#include <ola/Logging.h>
#include <string>
#include "tools/e133/SlpConstants.h"
#include "tools/e133/OpenSLPThread.h"

using std::string;
using ola::slp::URLEntries;


void RegisterCallback(SLPHandle slp_handle, SLPError errcode, void *cookie) {
  SLPError *error = static_cast<SLPError*>(cookie);
  *error = errcode;
  (void) slp_handle;
}


/**
 * This is what we pass as a void* to the slp callback
 */
typedef struct {
  SLPError error;
  URLEntries urls;
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
    cookie->urls.push_back(ola::slp::URLEntry(srvurl, lifetime));
    cookie->error = SLP_OK;
  } else if (errcode == SLP_LAST_CALL) {
    cookie->error = SLP_OK;
  } else {
    cookie->error = errcode;
  }

  return SLP_TRUE;
  (void) slp_handle;
}


/**
 * Create a new resolver thread. This doesn't actually start it.
 * @param ss the select server to use to handle the callbacks.
 */
OpenSLPThread::OpenSLPThread(ola::thread::ExecutorInterface *executor,
                             unsigned int discovery_interval)
    : BaseSLPThread(executor, discovery_interval) {
}


/**
 * Clean up
 */
OpenSLPThread::~OpenSLPThread() {
  Cleanup();
}


/**
 * Setup the SLP Thread
 */
bool OpenSLPThread::Init() {
  if (m_init_ok)
    return true;

  SLPError err = SLPOpen("en", SLP_FALSE, &m_slp_handle);
  if (err != SLP_OK) {
    OLA_INFO << "Error opening slp handle " << err;
    return false;
  }
  return BaseSLPThread::Init();
}


/**
 * Clean up
 */
void OpenSLPThread::Cleanup() {
  if (m_init_ok)
    SLPClose(m_slp_handle);
  m_init_ok = false;
}

void OpenSLPThread::RunDiscovery(InternalDiscoveryCallback *callback,
                                 const string &service) {
  slp_cookie cookie;
  SLPError err = SLPFindSrvs(m_slp_handle,
                             service.c_str(),
                             0,  // use configured scopes
                             0,  // no attr filter
                             ServiceCallback,
                             &cookie);
  bool ok = true;

  if ((err != SLP_OK)) {
    OLA_INFO << "Error finding service with slp " << err;
    ok = false;
  }

  if (cookie.error != SLP_OK) {
    OLA_INFO << "Error finding service with slp " << cookie.error;
    ok = false;
  }
  callback->Run(ok, cookie.urls);
}

void OpenSLPThread::RegisterSLPService(RegistrationCallback *callback,
                                       const string& url,
                                       unsigned short lifetime) {
  SLPError callbackerr = SLP_OK;
  SLPError err = SLPReg(m_slp_handle, url.c_str(), lifetime, 0, "",
                        SLP_TRUE, RegisterCallback, &callbackerr);

  bool ok = true;
  if ((err != SLP_OK)) {
    OLA_INFO << "Error registering service with slp " << err;
    ok = false;
  }

  if (callbackerr != SLP_OK) {
    OLA_INFO << "Error registering service with slp " << callbackerr;
    ok = false;
  }
  callback->Run(ok);
}


void OpenSLPThread::DeRegisterSLPService(RegistrationCallback *callback,
                                         const string& url) {
  SLPError callbackerr = SLP_OK;
  SLPError err = SLPDereg(m_slp_handle, url.c_str(), RegisterCallback,
                          &callbackerr);

  bool ok = true;
  if ((err != SLP_OK)) {
    OLA_INFO << "Error deregistering service with slp " << err;
    ok = false;
  }

  if (callbackerr != SLP_OK) {
    OLA_INFO << "Error deregistering service with slp " << callbackerr;
    ok = false;
  }
  callback->Run(ok);
}
