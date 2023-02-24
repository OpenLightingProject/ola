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
 * DMXTrigger.h
 * Copyright (C) 2011 Simon Newton
 */


#ifndef TOOLS_OLA_TRIGGER_DMXTRIGGER_H_
#define TOOLS_OLA_TRIGGER_DMXTRIGGER_H_

#include <ola/DmxBuffer.h>
#include <vector>

#include "tools/ola_trigger/Action.h"

/*
 * The class which manages the triggering.
 */
class DMXTrigger {
 public:
    typedef std::vector<Slot*> SlotVector;

    DMXTrigger(Context *context, const SlotVector &actions);
    ~DMXTrigger() {}

    void NewDMX(const ola::DmxBuffer &data);

 private:
    Context *m_context;
    SlotVector m_slots;  // kept sorted
};
#endif  // TOOLS_OLA_TRIGGER_DMXTRIGGER_H_
