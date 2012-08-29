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
 * SLPServiceImpl.h
 * Copyright (C) 2012 Simon Newton
 */


#ifndef TOOLS_SLP_SLPSERVICEIMPL_H_
#define TOOLS_SLP_SLPSERVICEIMPL_H_

#include "tools/slp/SLP.pb.h"

namespace ola {
namespace slp {

class SLPNode;

/**
 * The implementation of the SLP Service.
 */
class SLPServiceImpl : public OLASLPService {
  public:
    SLPServiceImpl(SLPNode *node)
        : m_node(node) {
    }
    ~SLPServiceImpl() {}

    void FindService(::google::protobuf::RpcController* controller,
                     const ServiceRequest* request,
                     ServiceReply* response,
                     ::google::protobuf::Closure* done);

    void RegisterService(::google::protobuf::RpcController* controller,
                         const ServiceRegistration* request,
                         ServiceAck* response,
                         ::google::protobuf::Closure* done);

    void DeRegisterService(::google::protobuf::RpcController* controller,
                           const ServiceDeRegistration* request,
                           ServiceAck* response,
                           ::google::protobuf::Closure* done);

  private:
    SLPNode *m_node;
};
}  // slp
}  // ola
#endif  // TOOLS_SLP_SLPSERVICEIMPL_H_
