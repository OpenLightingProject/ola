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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * OlaDevice.h
 * Interface to the OLA Client Device class
 * Copyright (C) 2005 Simon Newton
 */

#ifndef OLA_OLADEVICE_H_
#define OLA_OLADEVICE_H_

#include <ola/client/ClientTypes.h>

/**
 * @file
 * @brief
 * @deprecated Include <ola/client/ClientTypes.h> instead.
 */

namespace ola {

// For backwards compatability:
typedef ola::client::OlaDevice OlaDevice;
typedef ola::client::OlaInputPort OlaInputPort;
typedef ola::client::OlaOutputPort OlaOutputPort;
typedef ola::client::OlaPlugin OlaPlugin;
typedef ola::client::OlaPort OlaPort;
typedef ola::client::OlaUniverse OlaUniverse;

}  // namespace ola
#endif  // OLA_OLADEVICE_H_
