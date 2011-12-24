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
 * DMXTrigger.h
 * Copyright (C) 2011 Simon Newton
 */


#ifndef TOOLS_DMX_TRIGGER_DMXTRIGGER_H_
#define TOOLS_DMX_TRIGGER_DMXTRIGGER_H_

#include <ola/DmxBuffer.h>
#include <vector>

#include "tools/ola_trigger/Action.h"

using ola::DmxBuffer;


/*
 * The class which manages the triggering.
 */
class DMXTrigger {
  public:
    typedef std::vector<SlotActions*> SlotActionVector;

    DMXTrigger(Context *context, const SlotActionVector &actions);
    ~DMXTrigger() {}

    void NewDMX(const DmxBuffer &data);

  private:
    Context *m_context;
    DmxBuffer m_last_buffer;
    SlotActionVector m_slot_actions;  // kept sorted
};
#endif  // TOOLS_DMX_TRIGGER_DMXTRIGGER_H_
