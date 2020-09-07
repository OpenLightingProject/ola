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
 * StrError_R.h
 * Helper functions for strerror_r and a declaration of strerror_r that is
 * XSI-compliant.
 * Copyright (C) 2018 Shenghao Yang
 */

/**
 * @defgroup strerror Description of system error codes
 * @brief Error descriptions
 * @details Convenience functions to obtain descriptions of system error codes.
 * The functions and variables in this group are only defined if \c strerror_r()
 * is available.
 */

/**
 * @addtogroup strerror
 * @{
 * @file StrError_R.h
 * @}
 */

#ifndef INCLUDE_OLA_BASE_STRERROR_R_H_
#define INCLUDE_OLA_BASE_STRERROR_R_H_

#include <string>

namespace ola {

/**
 * @addtogroup strerror
 * @{
 */

/**
 * @brief Length of the internal buffer used for @ref StrError_R().
 * 
 * If the length of the system-provided error description exceeds the length
 * of this buffer minus one, then the output of @ref StrError_R() will only
 * include the numerical error value provided.
 */
extern const int StrError_R_BufSize;

/**
 * @brief XSI-compliant version of \c strerror_r().
 *
 * @see https://linux.die.net/man/3/strerror for more details.
 */
int StrError_R_XSI(int errnum, char* buf, size_t buflen);

/**
 * @brief Convenience function that wraps @ref StrError_R_XSI().
 * 
 * @param errnum error value to generate the textual description for.
 * @return Textual description of the error value. If the description is
 * truncated due to an insufficient buffer length, the description will
 * be in the form: "errno = errnum".
 * @sa StrError_R_BufSize for information regarding truncation.
 */
std::string StrError_R(int errnum);

/**@}*/
}  // namespace ola

#endif  // INCLUDE_OLA_BASE_STRERROR_R_H_
