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
 * Strerror_r.h
 * Declaration of strerror_r that is XSI-compliant.
 * Copyright (C) 2018 Shenghao Yang
 */

/**
 * @defgroup strerror Description of system error codes
 * @brief Error descriptions
 * @details Some libraries provide their own version of functions to
 * retrieve error descriptions that may not be compliant with
 * adopted specifications. To work around that, we have a wrapper
 * function that provides the standards-compliant interface at all times.
 */

/**
 * @addtogroup strerror
 * @{
 * @file Strerror_r.h
 * @}
 */

#ifndef INCLUDE_OLA_BASE_STRERROR_R_H_
#define INCLUDE_OLA_BASE_STRERROR_R_H_

namespace ola {

/**
 * @addtogroup strerror
 * @{
 */

/*
 * @brief XSI-compliant version of @ref strerror_r()
 
 * See strerror(3) for more details.
 */
int Strerror_r(int errnum, char* buf, size_t buflen);

/**@}*/
}  // namespace ola

#endif  // INCLUDE_OLA_BASE_STRERROR_R_H_
