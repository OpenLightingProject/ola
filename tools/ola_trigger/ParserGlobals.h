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
 * ParserGlobals.h
 * Copyright (C) 2011 Simon Newton
 *
 * Unfortunately we have to use global variables because there isn't a way
 * to pass in data during the parse stage.
 */

#ifndef TOOLS_OLA_TRIGGER_PARSERGLOBALS_H_
#define TOOLS_OLA_TRIGGER_PARSERGLOBALS_H_

#include <map>

// The context object
extern class Context *global_context;

// A map of slot offsets to SlotAction objects
typedef std::map<uint16_t, class Slot*> SlotActionMap;
extern SlotActionMap global_slots;

#endif  // TOOLS_OLA_TRIGGER_PARSERGLOBALS_H_
