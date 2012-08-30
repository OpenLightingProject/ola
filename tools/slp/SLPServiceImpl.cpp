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
#include "tools/slp/SLPNode.h"


namespace ola {
namespace slp {


/**
 * Find Service request.
 */
void SLPServiceImpl::FindService(::google::protobuf::RpcController* controller,
                                 const ServiceRequest* request,
                                 ServiceReply* response,
                                 ::google::protobuf::Closure* done) {
  (void) controller;
  OLA_INFO << "Recv FindService request";
  m_node->FindService(
      request->service(),
      NewSingleCallback(this,
                        &SLPServiceImpl::FindServiceHandler,
                        response,
                        done));
}


/**
 * Register service request.
 */
void SLPServiceImpl::RegisterService(
    ::google::protobuf::RpcController* controller,
    const ServiceRegistration* request,
    ServiceAck* response,
    ::google::protobuf::Closure* done) {
  OLA_INFO << "Recv RegisterService request";
  (void) controller;
  (void) request;
  response->set_error_code(0);
  done->Run();
}


void SLPServiceImpl::DeRegisterService(
    ::google::protobuf::RpcController* controller,
    const ServiceDeRegistration* request,
    ServiceAck* response,
    ::google::protobuf::Closure* done) {
  OLA_INFO << "Recv DeRegisterService request";
  (void) controller;
  (void) request;
  response->set_error_code(0);
  done->Run();
}


/**
 * Called when FindService completes.
 */
void SLPServiceImpl::FindServiceHandler(ServiceReply* response,
                                        ::google::protobuf::Closure* done) {
  OLA_INFO << "Find service complete";
  (void) response;
  done->Run();
}
}  // slp
}  // ola
