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
 * Systemd.h
 * Interface to systemd functionality.
 * Copyright (C) 2018 Shenghao Yang
 */

#ifndef OLAD_SYSTEMD_H_
#define OLAD_SYSTEMD_H_

namespace ola {

/*
 * @brief Notify systemd about daemon state changes.
 *  
 * This function logs on failures to queue notifications, but only if the
 * notification socket environment variable is set.
 *  
 * @param unset_environment whether to unset the notification socket environment
 *        variable so child processes cannot utilize it.
 * @param state state block to pass to systemd.
 * @return value returned from \ref sd_notify()
 */
int notify_systemd(int unset_environment, const char *state);

/* 
 * @brief Tests whether the systemd notification socket is available.
 * @return @code true if the socket is available, @code false if not.
 */
bool notify_available();

}  // namespace ola
#endif  // OLAD_SYSTEMD_H_
