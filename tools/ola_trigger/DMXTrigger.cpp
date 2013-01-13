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
 * DMXTrigger.cpp
 * Copyright (C) 2011 Simon Newton
 */

#include <ola/DmxBuffer.h>
#include <ola/Logging.h>
#include <algorithm>
#include <vector>

#include "tools/ola_trigger/DMXTrigger.h"

using ola::DmxBuffer;


/**
 * Create a new trigger
 */
DMXTrigger::DMXTrigger(Context *context,
                       const SlotVector &actions)
    : m_context(context),
      m_slots(actions) {
  sort(m_slots.begin(), m_slots.end());
}


/**
 * Called when new DMX arrives.
 */
void DMXTrigger::NewDMX(const DmxBuffer &data) {
  SlotVector::iterator iter = m_slots.begin();
  for (; iter != m_slots.end(); iter++) {
    uint16_t slot_number = (*iter)->SlotOffset();
    if (slot_number >= data.Size()) {
      // the DMX frame was too small
      break;
    }
    (*iter)->TakeAction(m_context, data.Get(slot_number));
  }
}
