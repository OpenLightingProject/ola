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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Strerror_r.h
 * Declaration of strerror_r that is XSI-compliant.
 * Copyright (C) 2018 Shenghao Yang
 */

#ifndef OLAD_STRERROR_R_H_
#define OLAD_STRERROR_R_H_

namespace ola {

/*
 * @brief XSI-compliant version of @ref strerror_r()
 
 * See strerror(3) for more details.
 */
int Strerror_r(int errnum, char* buf, size_t buflen);

}  // namespace ola

#endif  // OLAD_STRERROR_R_H_
