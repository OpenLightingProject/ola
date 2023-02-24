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
 * VariableInterpolator.h
 * Copyright (C) 2011 Simon Newton
 */


#ifndef TOOLS_OLA_TRIGGER_VARIABLEINTERPOLATOR_H_
#define TOOLS_OLA_TRIGGER_VARIABLEINTERPOLATOR_H_

#include <string>
#include "tools/ola_trigger/Context.h"

bool InterpolateVariables(const std::string &input,
                          std::string *output,
                          const Context &context);

#endif  // TOOLS_OLA_TRIGGER_VARIABLEINTERPOLATOR_H_
