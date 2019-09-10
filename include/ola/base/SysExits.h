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
 * SysExits.h
 * Exit codes.
 *
 * Some platforms (android) don't provide sysexits.h. To work around this we
 * define our own exit codes, which use the system values if provided, or
 * otherwise default values.
 */

/**
 * @defgroup sysexit System Exit Values
 * @brief Exit codes
 * @details Some platforms (Android) don't provide sysexits.h. To work around
 * this we define our own exit codes, which use the system values if provided
 * or otherwise our default values.
 */

/**
 * @addtogroup sysexit
 * @{
 * @file SysExits.h
 * @}
 */

#ifndef INCLUDE_OLA_BASE_SYSEXITS_H_
#define INCLUDE_OLA_BASE_SYSEXITS_H_

namespace ola {

/**
 * @addtogroup sysexit
 * @{
 */

extern const int EXIT_OK; /**< @brief successful termination*/
extern const int EXIT__BASE; /**< @brief base value for error messages */
extern const int EXIT_USAGE; /**< @brief command line usage error*/
extern const int EXIT_DATAERR; /**< @brief data format error*/
extern const int EXIT_NOINPUT;  /**< @brief cannot open input*/
extern const int EXIT_NOUSER; /**< @brief addresses unknown*/
extern const int EXIT_NOHOST; /**< @brief host name unknown*/
extern const int EXIT_UNAVAILABLE; /**< @brief service unavailable*/
extern const int EXIT_SOFTWARE; /**< @brief internal software error*/
extern const int EXIT_OSERR; /**< @brief system error (e.g can't fork)*/
extern const int EXIT_OSFILE; /**< @brief critical OS file missing*/
extern const int EXIT_CANTCREAT; /**< @brief can't create (user) output file*/
extern const int EXIT_IOERR; /**< @brief input/output error*/
extern const int EXIT_TEMPFAIL; /**< @brief temp failure; user can rety*/
extern const int EXIT_PROTOCOL; /**< @brief remote error in protocol*/
extern const int EXIT_NOPERM; /**< @brief permission denied*/
extern const int EXIT_CONFIG; /**< @brief configuration error*/
extern const int EXIT__MAX; /**< @brief maximum listed value*/

/**@}*/
}  // namespace ola
#endif  // INCLUDE_OLA_BASE_SYSEXITS_H_
