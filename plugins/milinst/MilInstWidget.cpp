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
 * MilInstWidget.cpp
 * This is the base widget class
 * Copyright (C) 2013 Peter Newman
 */

#include <string.h>
#include <algorithm>
#include "ola/Callback.h"
#include "ola/Logging.h"
#include "plugins/milinst/MilInstWidget.h"

namespace ola {
namespace plugin {
namespace milinst {

/*
 * New widget
 */
MilInstWidget::~MilInstWidget() {
OLA_DEBUG << "MilInstWidget: ~";
  if (m_socket) {
    m_socket->Close();
    delete m_socket;
  }
}


/*
 * Disconnect from the widget
 */
int MilInstWidget::Disconnect() {
OLA_DEBUG << "MilInstWidget: disconnect";
  m_socket->Close();
  return 0;
}


/*
 * Called when there is data to read
 */
void MilInstWidget::SocketReady() {
OLA_DEBUG << "MilInstWidget: soc ready";
  //while (m_socket->DataRemaining() > 0) {
    //DoRecv();
  //}
}


void MilInstWidget::Timeout() {
  OLA_DEBUG << "MilInstWidget: timing out";
  if (m_ss)
    m_ss->Terminate();
}

/*
 * Check if this is actually a MilInst device
 * @return true if this is a milinst,  false otherwise
 */
bool MilInstWidget::DetectDevice() {
return true;
 // if (m_ss)
 //   delete m_ss;
////
 // m_got_response = false;
 // m_ss = new SelectServer();
 // m_ss->AddReadDescriptor(m_socket, false);
 // m_ss->RegisterSingleTimeout(
 //     100,
 //     ola::NewSingleCallback(this, &MilInstWidget::Timeout));
////
 // // try a command, we should get a response
 // SetChannel(0, 0);
////
 // m_ss->Run();
 // delete m_ss;
 // m_ss = NULL;
 // if (m_got_response)
 //   return true;

 // m_socket->Close();
 // return false;
}
}  // namespace milinst
}  // namespace plugin
}  // namespace ola
