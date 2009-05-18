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
 * SimpleRpcController.cpp
 * The Simple RPC Controller
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <lla/Logging.h>
#include "SimpleRpcController.h"

using namespace lla::rpc;

SimpleRpcController::SimpleRpcController() :
  m_failed(false),
  m_cancelled(false),
  m_error_text(""),
  m_callback(NULL) {
}

void SimpleRpcController::Reset() {
  m_failed = false;
  m_cancelled = false;
  if (m_callback)
    LLA_FATAL << "calling reset() while an rpc is in progress, we're " <<
      "leaking memory!";
  m_callback = NULL;
}

void SimpleRpcController::StartCancel() {
  m_cancelled = true;
  if (m_callback)
    m_callback->Run();
}

void SimpleRpcController::SetFailed(const string &reason) {
  m_failed = true;
  m_error_text = reason;
}
