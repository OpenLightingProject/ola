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
 * PortConstants.h
 * Header file for the Port Constants.
 * Copyright (C) 2005-2010 Simon Newton
 */

#ifndef INCLUDE_OLAD_PORTCONSTANTS_H_
#define INCLUDE_OLAD_PORTCONSTANTS_H_
/**
 * @addtogroup olad
 * @{
 * @file PortConstants.h
 * @brief Different priority modes and priority capabilities. Please make sure
 * and visit [Merging Algorithms]
 * (http://opendmx.net/index.php/OLA_Merging_Algorithms) for more information.
 * @}
 */

namespace ola {

  /**
   * @addtogroup olad
   * @{
   */

  /**
   * @brief Defines the different priority modes that OLA supports.
   */
  typedef enum {
    /** Allows Port to inherit the priority of incoming data */
    PRIORITY_MODE_INHERIT,
    /** The Port has a static priority set by the user and cannot
     * inherit priorities from the incoming data*/
    PRIORITY_MODE_STATIC,
    /** Port can be used as an end condition for iterating through the
     * modes*/
    PRIORITY_MODE_END,
  } port_priority_mode;

  /**
   * @brief Defines the priority capability of a Port.
   */
  typedef enum {
    /** Port doesn't support priorities at all*/
    CAPABILITY_NONE,
    /** Port allows a static priority assignment */
    CAPABILITY_STATIC,
    /** Port can either inherit or use a static assignment */
    CAPABILITY_FULL,
  } port_priority_capability;

  /**@} End olad group */

}  // namespace ola
#endif  // INCLUDE_OLAD_PORTCONSTANTS_H_
