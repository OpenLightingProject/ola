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
 * IOCtl.h
 * This is a minimal definition of ioctl() to allow the UART DMX plugin
 * to compile with custom baud rate support on newer systems which don't
 * have <stropts.h>.
 */

#ifndef INCLUDE_OLA_IO_IOCTL_H_
#define INCLUDE_OLA_IO_IOCTL_H_

extern "C" {
  extern int ioctl(int f, unsigned long r, ...) __THROW;  // NOLINT(runtime/int)
}

#endif  // INCLUDE_OLA_IO_IOCTL_H_
