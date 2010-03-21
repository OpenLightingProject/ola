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
 * PortDecorators.h
 * Header file for the Port Decorators classes
 * Copyright (C) 2010 Simon Newton
 */

#ifndef INCLUDE_OLAD_PORTDECORATORS_H_
#define INCLUDE_OLAD_PORTDECORATORS_H_

#include <ola/Logging.h>
#include <olad/Port.h>
#include <olad/TokenBucket.h>

namespace ola {

/*
 * A Decorator for an Output Port that throttles the writes
 */
class ThrottledOutputPortDecorator: public OutputPortDecorator {
  public:
    explicit ThrottledOutputPortDecorator(
        OutputPort *port,
        const TimeStamp *wake_time,
        unsigned int initial_count,
        unsigned int rate):
        OutputPortDecorator(port),
        m_bucket(initial_count, rate, rate, *wake_time),
        m_wake_time(wake_time) {}

    bool WriteDMX(const DmxBuffer &buffer, uint8_t priority) {
      if (m_bucket.GetToken(*m_wake_time))
        return m_port->WriteDMX(buffer, priority);
      else
        OLA_INFO << "Port rated limited, dropping frame";
      return true;
    }

  private:
    TokenBucket m_bucket;
    const TimeStamp *m_wake_time;
};
}  // ola
#endif  // INCLUDE_OLAD_PORTDECORATORS_H_
