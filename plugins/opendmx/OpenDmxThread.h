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
 *
 * OpenDmxThread.h
 * Thread for the open dmx device
 * Copyright (C) 2005  Simon Newton
 */

#ifndef OPENDMXTHREAD_H
#define OPENDMXTHREAD_H

#include <stdint.h>
#include <pthread.h>

#include <string>

using namespace std;

#define MAX_DMX 512

class OpenDmxThread {

  public:
    OpenDmxThread();
    ~OpenDmxThread();

    int start (string *path);
    int stop();
    int write_dmx(uint8_t *data , int channels);
    void *run(string *path);

  private:
    int do_write(uint8_t *buf, int length);

    int m_fd;
    uint8_t m_dmx[MAX_DMX];
    pthread_mutex_t m_mutex;
    bool m_term;
    pthread_mutex_t m_term_mutex;
    pthread_cond_t m_term_cond;
    pthread_t m_tid;

};

#endif
