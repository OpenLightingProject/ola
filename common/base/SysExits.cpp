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
 * SysExits.cpp
 * Exit codes.
 *
 * Some platforms (android) don't provide sysexits.h. To work around this we
 * define our own exit codes, which use the system values if provided, or
 * otherwise default values.
 */

/**
 * @addtogroup sysexit
 * @{
 * @file SysExits.cpp
 * @}
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H

#ifdef HAVE_SYSEXITS_H
#include <sysexits.h>
#else
// Copied from the Berkeley sources.
/**
 * @addtogroup sysexit
 * @{
 */

/**
 * @name Fallback values
 * @details These are the values we fall back to in case the os doesn't provide
 * exit codes
 * @{
 */
#define EX_OK       0   /**< @brief successful termination */
#define EX__BASE    64  /**< @brief base value for error messages */
#define EX_USAGE    64  /**< @brief command line usage error */
#define EX_DATAERR  65  /**< @brief data format error */
#define EX_NOINPUT  66  /**< @brief cannot open input */
#define EX_NOUSER   67  /**< @brief addressee unknown */
#define EX_NOHOST   68  /**< @brief host name unknown */
#define EX_UNAVAILABLE  69  /**< @brief service unavailable */
#define EX_SOFTWARE 70  /**< @brief internal software error */
#define EX_OSERR    71  /**< @brief system error (e.g. can't fork) */
#define EX_OSFILE   72  /**< @brief critical OS file missing */
#define EX_CANTCREAT    73  /**< @brief can't create (user) output file */
#define EX_IOERR    74  /**< @brief input/output error */
#define EX_TEMPFAIL 75  /**< @brief temp failure; user is invited to retry */
#define EX_PROTOCOL 76  /**< @brief remote error in protocol */
#define EX_NOPERM   77  /**< @brief permission denied */
#define EX_CONFIG   78  /**< @brief configuration error */
#define EX__MAX 78  /**< @brief maximum listed value */

/**
 * @}
 * @}
 */

#endif  // HAVE_SYSEXITS_H

#include "ola/base/SysExits.h"

namespace ola {

/**
 * @addtogroup sysexit
 * @{
 */

const int EXIT_OK = EX_OK;
const int EXIT__BASE = EX__BASE;
const int EXIT_USAGE = EX_USAGE;
const int EXIT_DATAERR = EX_DATAERR;
const int EXIT_NOINPUT = EX_NOINPUT;
const int EXIT_NOUSER = EX_NOUSER;
const int EXIT_NOHOST = EX_NOHOST;
const int EXIT_UNAVAILABLE = EX_UNAVAILABLE;
const int EXIT_SOFTWARE = EX_SOFTWARE;
const int EXIT_OSERR = EX_OSERR;
const int EXIT_OSFILE = EX_OSFILE;
const int EXIT_CANTCREAT = EX_CANTCREAT;
const int EXIT_IOERR = EX_IOERR;
const int EXIT_TEMPFAIL = EX_TEMPFAIL;
const int EXIT_PROTOCOL = EX_PROTOCOL;
const int EXIT_NOPERM = EX_NOPERM;
const int EXIT_CONFIG = EX_CONFIG;
const int EXIT__MAX = EX__MAX;

/**@}*/
}  // namespace ola
