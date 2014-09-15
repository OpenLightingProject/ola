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
 * OlaClientWrapper.h
 * This provides a helpful wrapper for the OlaClient & OlaCallbackClient
 * classes.
 * Copyright (C) 2005 Simon Newton
 */

#ifndef OLA_OLACLIENTWRAPPER_H_
#define OLA_OLACLIENTWRAPPER_H_

#include <ola/client/ClientWrapper.h>
#include <ola/OlaCallbackClient.h>

namespace ola {

/**
 * @brief A ClientWrapper that uses the OlaCallbackClient.
 * @deprecated Use ola::client::OlaClientWrapper instead.
 */
typedef ola::client::GenericClientWrapper<OlaCallbackClient>
    OlaCallbackClientWrapper;

}  // namespace ola
#endif  // OLA_OLACLIENTWRAPPER_H_
