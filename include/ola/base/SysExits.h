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
 * SysExits.h
 * Exit codes.
 *
 * Some platforms (android) don't provide sysexits.h. To work around this we
 * define our own exit codes, which use the system values if provided, or
 * otherwise default values.
 */

#ifndef INCLUDE_OLA_BASE_SYSEXITS_H_
#define INCLUDE_OLA_BASE_SYSEXITS_H_

namespace ola {

extern const int EXIT_OK;
extern const int EXIT__BASE;
extern const int EXIT_USAGE;
extern const int EXIT_DATAERR;
extern const int EXIT_NOINPUT;
extern const int EXIT_NOUSER;
extern const int EXIT_NOHOST;
extern const int EXIT_UNAVAILABLE;
extern const int EXIT_SOFTWARE;
extern const int EXIT_OSERR;
extern const int EXIT_OSFILE;
extern const int EXIT_CANTCREAT;
extern const int EXIT_IOERR;
extern const int EXIT_TEMPFAIL;
extern const int EXIT_PROTOCOL;
extern const int EXIT_NOPERM;
extern const int EXIT_CONFIG;
extern const int EXIT__MAX;

}  // namespace ola
#endif  // INCLUDE_OLA_BASE_SYSEXITS_H_
