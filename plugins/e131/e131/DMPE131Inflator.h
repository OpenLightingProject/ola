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
 * DMPE131Inflator.h
 * This is a subclass of the DMPInflator which knows how to handle DMP over
 * E1.31 messages.
 * Copyright (C) 2009 Simon Newton
 */

#ifndef PLUGINS_E131_E131_DMPE131INFLATOR_H_
#define PLUGINS_E131_E131_DMPE131INFLATOR_H_

#include <map>
#include <vector>
#include "ola/Clock.h"
#include "ola/Callback.h"
#include "ola/DmxBuffer.h"
#include "plugins/e131/e131/DMPInflator.h"

namespace ola {
namespace plugin {
namespace e131 {

class DMPE131Inflator: public DMPInflator {
  friend class DMPE131InflatorTest;

  public:
    explicit DMPE131Inflator(bool ignore_preview):
      DMPInflator(),
      m_ignore_preview(ignore_preview) {
    }
    ~DMPE131Inflator();

    bool SetHandler(unsigned int universe, ola::DmxBuffer *buffer,
                    uint8_t *priority, ola::Callback0<void> *handler);
    bool RemoveHandler(unsigned int universe);

    void RegisteredUniverses(std::vector<unsigned int> *universes);

  protected:
    virtual bool HandlePDUData(uint32_t vector,
                               const HeaderSet &headers,
                               const uint8_t *data,
                               unsigned int pdu_len);

  private:
    typedef struct {
      CID cid;
      uint8_t sequence;
      TimeStamp last_heard_from;
      DmxBuffer buffer;
    } dmx_source;

    typedef struct {
      DmxBuffer *buffer;
      Callback0<void> *closure;
      uint8_t active_priority;
      uint8_t *priority;
      std::vector<dmx_source> sources;
    } universe_handler;

    std::map<unsigned int, universe_handler> m_handlers;
    bool m_ignore_preview;
    ola::Clock m_clock;

    bool TrackSourceIfRequired(universe_handler *universe_data,
                               const HeaderSet &headers,
                               DmxBuffer **buffer);

    // The max number of sources we'll track per universe.
    static const uint8_t MAX_MERGE_SOURCES = 6;
    static const uint8_t MAX_PRIORITY = 200;
    // ignore packets that differ by less than this amount from the last one
    static const int8_t SEQUENCE_DIFF_THRESHOLD = -20;
    // expire sources after 2.5s
    static const TimeInterval EXPIRY_INTERVAL;
};
}  // namespace e131
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_E131_E131_DMPE131INFLATOR_H_
