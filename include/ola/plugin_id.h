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

enum ola_plugin_id_e {
  OLA_PLUGIN_ALL = 0,
  OLA_PLUGIN_DUMMY,
  OLA_PLUGIN_ARTNET,
  OLA_PLUGIN_SHOWNET,
  OLA_PLUGIN_ESPNET,
  OLA_PLUGIN_USBPRO,
  OLA_PLUGIN_OPENDMX,
  OLA_PLUGIN_SANDNET,
  OLA_PLUGIN_STAGEPROFI,
  OLA_PLUGIN_PATHPORT,
  OLA_PLUGIN_DMX4LINUX,
  OLA_PLUGIN_E131,
  OLA_PLUGIN_ACN,
  OLA_PLUGIN_LAST
};

typedef enum ola_plugin_id_e ola_plugin_id;

#ifdef __cplusplus
}
#endif

#endif
