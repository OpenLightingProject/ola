
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
 * LlaServerServiceImpl.h
 * Implemtation of the LlaService interface
 * Copyright (C) 2005 - 2008 Simon Newton
 */

#include "common/protocol/Lla.pb.h"

#ifndef LLA_CLIENT_SERVICE_IMPL_H
#define LLA_CLIENT_SERVICE_IMPL_H

namespace lla {

using namespace lla::proto;

class LlaClientServiceImpl: public lla::proto::LlaClientService {
  public:
    LlaClientServiceImpl(class LlaClientObserver *observer):
      m_observer(observer) {}
    ~LlaClientServiceImpl() {}

    void UpdateDmxData(::google::protobuf::RpcController* controller,
                       const DmxData* request,
                       Ack* response,
                       ::google::protobuf::Closure* done);
    void SetObserver(LlaClientObserver *observer) { m_observer = observer; }
  private:
    class LlaClientObserver *m_observer;
};


} // lla

#endif
