/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * plugin_id.h
 * contains the plugin ids.
 * Copyright (C) 2005-2006 Simon Newton
 */

#ifndef PLUGIN_ID_H
#define PLUGIN_ID_H

#ifdef __cplusplus
extern "C" {
#endif

enum lla_plugin_id_e {
  LLA_PLUGIN_ALL = 0,
  LLA_PLUGIN_DUMMY,
  LLA_PLUGIN_ARTNET,
  LLA_PLUGIN_SHOWNET,
  LLA_PLUGIN_ESPNET,
  LLA_PLUGIN_USBPRO,
  LLA_PLUGIN_OPENDMX,
  LLA_PLUGIN_SANDNET,
  LLA_PLUGIN_STAGEPROFI,
  LLA_PLUGIN_PATHPORT,
  LLA_PLUGIN_LAST
};

typedef enum lla_plugin_id_e lla_plugin_id;

#ifdef __cplusplus
}
#endif

#endif
