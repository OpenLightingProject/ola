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
 * OlaThread.cpp
 * A simple thread class
 * Copyright (C) 2010 Simon Newton
 */

#include "ola/Logging.h"
#include "ola/OlaThread.h"

namespace ola {

/*
 * Called by the new thread
 */
void *StartThread(void *d) {
  OlaThread *thread = static_cast<OlaThread*>(d);
  return thread->Run();
}


/*
 * Start this thread
 */
bool OlaThread::Start() {
  int ret = pthread_create(&m_thread_id,
                           NULL,
                           ola::StartThread,
                           static_cast<void*>(this));
  if (ret) {
    OLA_WARN << "pthread create failed";
    return false;
  }
  m_running = true;
  return true;
}


/*
 * Join this thread
 */
bool OlaThread::Join(void *ptr) {
  if (m_running) {
    int ret = pthread_join(m_thread_id, &ptr);
    m_running = false;
    return 0 == ret;
  }
  return false;
}
}  // ola
