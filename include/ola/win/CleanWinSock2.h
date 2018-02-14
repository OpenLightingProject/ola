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
 * CleanWinSock2.h
 * A common header that removes much of the name-space clutter that windows.h
 * and WinSock2.h creates
 * Copyright (C) 2014 Sean Sill
 */

#ifndef INCLUDE_OLA_WIN_CLEANWINSOCK2_H_
#define INCLUDE_OLA_WIN_CLEANWINSOCK2_H_

#ifdef _WIN32
#include <Winsock2.h>
// Some preprocessor magic to reduce Windows.h namespace pollution
#  ifdef AddPort
#    undef AddPort
#  endif  // AddPort
#  ifdef SendMessage
#    undef SendMessage
#  endif  // SendMessage
#endif  // _WIN32
#endif  // INCLUDE_OLA_WIN_CLEANWINSOCK2_H_

