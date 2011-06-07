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
 * slp-locate.cpp
 * Copyright (C) 2011 Simon Newton
 *
 * Send a SrvRqst for E1.33 nodes and report the urls.
 */

#include <slp.h>
#include <stdio.h>
#include <ola/Logging.h>


SLPBoolean ServiceCallback(SLPHandle slp_handle,
                           const char* srvurl,
                           unsigned short lifetime,
                           SLPError errcode,
                           void* cookie) {
  if (errcode == SLP_OK) {
    OLA_INFO << "Service URL = " << srvurl;
    OLA_INFO << "Service Timeout = " << lifetime;
    *(SLPError*)cookie = SLP_OK;
  } else if (errcode == SLP_LAST_CALL) {
    *(SLPError*)cookie = SLP_OK;
  } else {
    *(SLPError*)cookie = errcode;
  }

  return SLP_TRUE;
  (void) slp_handle;
}


int main(int argc, char *argv[]) {
  ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);

  SLPHandle slp_handle;
  SLPError err = SLPOpen("en", SLP_FALSE, &slp_handle);
  if (err != SLP_OK) {
    OLA_INFO << "Error opening slp handle " << err;
    return err;
  }

  SLPError callbackerr;
  err = SLPFindSrvs(slp_handle,
                    "esta.e133",
                    0,  // use configured scopes
                    0,  // no attr filter
                    ServiceCallback,
                    &callbackerr);

  if ((err != SLP_OK)) {
    OLA_INFO << "Error finding service with slp " << err;
    return err;
  }

  if (callbackerr != SLP_OK) {
    OLA_INFO << "Error finding service with slp " << callbackerr;
    return callbackerr;
  }

  SLPClose(slp_handle);
  (void) argc;
  (void) argv;
}
