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
 * SLPServiceImpl.cpp
 * Copyright (C) 2012 Simon Newton
 */

#include <ola/Logging.h>
#include "tools/slp/SLPServiceImpl.h"


namespace ola {
namespace slp {


/**
 * Service lookup request.
 */
void SLPServiceImpl::FindService(::google::protobuf::RpcController* controller,
                                 const ::ola::slp::ServiceRequest* request,
                                 ::ola::slp::ServiceReply* response,
                                 ::google::protobuf::Closure* done) {
  OLA_INFO << "Recv FindService request";
  (void) controller;
  (void) request;
  (void) response;
  done->Run();
}


/**
 * Register service request.
 */
void SLPServiceImpl::RegisterService(
    ::google::protobuf::RpcController* controller,
    const ::ola::slp::ServiceRegistration* request,
    ::ola::slp::ServiceAck* response,
    ::google::protobuf::Closure* done) {
  OLA_INFO << "Recv RegisterService request";
  (void) controller;
  (void) request;
  response->set_error_code(0);
  done->Run();
}
}  // slp
}  // ola
