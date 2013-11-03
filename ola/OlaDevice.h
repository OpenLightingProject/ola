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
 * OlaDevice.h
 * Interface to the OLA Client Device class
 * Copyright (C) 2005-2006 Simon Newton
 */

#ifndef OLA_OLADEVICE_H_
#define OLA_OLADEVICE_H_

#include <ola/api/ClientTypes.h>

/**
 * @file
 * @brief
 * @deprecated Include <ola/api/ClientTypes.h> instead.
 */

namespace ola {

// For backwards compatability:
typedef ola::api::OlaDevice OlaDevice;
typedef ola::api::OlaInputPort OlaInputPort;
typedef ola::api::OlaOutputPort OlaOutputPort;
typedef ola::api::OlaPlugin OlaPlugin;
typedef ola::api::OlaPort OlaPort;
typedef ola::api::OlaUniverse OlaUniverse;

}  // namespace ola
#endif  // OLA_OLADEVICE_H_
