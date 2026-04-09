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
 * DMPE131Inflator.h
 * This is a subclass of the DMPInflator which knows how to handle DMP over
 * E1.31 messages.
 * Copyright (C) 2009 Simon Newton
 */

#ifndef LIBS_ACN_DMPE131INFLATOR_H_
#define LIBS_ACN_DMPE131INFLATOR_H_

#include <map>
#include <vector>
#include "ola/Clock.h"
#include "ola/Callback.h"
#include "ola/DmxBuffer.h"
#include "libs/acn/DMPInflator.h"

namespace ola {
namespace acn {

class DMPE131Inflator: public DMPInflator {
  friend class DMPE131InflatorTest;

 public:
  explicit DMPE131Inflator(bool ignore_preview):
    DMPInflator(),
    m_ignore_preview(ignore_preview) {
  }
  ~DMPE131Inflator();

  bool SetHandler(uint16_t universe, ola::DmxBuffer *buffer,
                  uint8_t *priority, ola::Callback0<void> *handler);
  bool RemoveHandler(uint16_t universe);

  void RegisteredUniverses(std::vector<uint16_t> *universes);

 protected:
  virtual bool HandlePDUData(uint32_t vector,
                             const HeaderSet &headers,
                             const uint8_t *data,
                             unsigned int pdu_len);

 private:
  typedef struct {
    ola::acn::CID cid;
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

  typedef std::map<uint16_t, universe_handler> UniverseHandlers;

  UniverseHandlers m_handlers;
  bool m_ignore_preview;
  ola::Clock m_clock;

  bool TrackSourceIfRequired(universe_handler *universe_data,
                             const HeaderSet &headers,
                             DmxBuffer **buffer);

  // The max number of sources we'll track per universe.
  static const uint8_t MAX_MERGE_SOURCES = 6;
  // The max merge priority.
  static const uint8_t MAX_E131_PRIORITY = 200;
  // ignore packets that differ by less than this amount from the last one
  static const int8_t SEQUENCE_DIFF_THRESHOLD = -20;
  // expire sources after 2.5s
  static const TimeInterval EXPIRY_INTERVAL;
};
}  // namespace acn
}  // namespace ola
#endif  // LIBS_ACN_DMPE131INFLATOR_H_
