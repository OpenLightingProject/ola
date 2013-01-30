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
 * E131Include.h
 * On OS X systems with ossp/uuid.h installed we need to #include <ossp/uuid.h>
 * before pulling in <unistd.h> otherwise we get an error like:
 *   /opt/local/include/ossp/uuid.h:94: error: conflicting declaration 'typedef
 *     struct uuid_st uuid_t'
 *   /usr/include/unistd.h:133: error: 'uuid_t' has a previous declaration as
 *     'typedef unsigned char uuid_t [16]'
 *
 * To work around this, we include <ossp/uuid.h> once here, and include this
 * file first in all the E1.31 code.
 * Copyright (C) 2010 Simon Newton
 */

#ifndef PLUGINS_E131_E131_E131INCLUDES_H_
#define PLUGINS_E131_E131_E131INCLUDES_H_

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_OSSP_UUID_H
#include <ossp/uuid.h>
#endif

#endif  // PLUGINS_E131_E131_E131INCLUDES_H_
