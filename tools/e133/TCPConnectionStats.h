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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * TCPConnectionStats.h
 * Copyright (C) 2012 Simon Newton
 */

#include <stdint.h>
#include <ola/network/IPV4Address.h>


#ifndef TOOLS_E133_TCPCONNECTIONSTATS_H_
#define TOOLS_E133_TCPCONNECTIONSTATS_H_

/**
 * Container for stats about the TCP connection.
 */
class TCPConnectionStats {
 public:
    TCPConnectionStats()
      : ip_address(),
        connection_events(0),
        unhealthy_events(0) {
    }

    void ResetCounters() {
      connection_events = 0;
      unhealthy_events = 0;
    }

    ola::network::IPV4Address ip_address;
    uint16_t connection_events;
    uint16_t unhealthy_events;
};
#endif  // TOOLS_E133_TCPCONNECTIONSTATS_H_
