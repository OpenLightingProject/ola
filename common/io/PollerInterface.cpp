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
 * PollerInterface.cpp
 * A poller provides the select loop.
 * Copyright (C) 2013 Simon Newton
 */

#include "common/io/PollerInterface.h"

#include <string>

namespace ola {
namespace io {

/**
 * @brief The number of descriptors registered for read events.
 */
const char PollerInterface::K_READ_DESCRIPTOR_VAR[] = "ss-read-descriptors";

/**
 * @brief The number of descriptors registered for write events.
 */
const char PollerInterface::K_WRITE_DESCRIPTOR_VAR[] = "ss-write-descriptor";

/**
 * @brief The number of connected descriptors registered for read events.
 */
const char PollerInterface::K_CONNECTED_DESCRIPTORS_VAR[] =
    "ss-connected-descriptors";

/**
 * @brief The time spent in the event loop.
 */
const char PollerInterface::K_LOOP_TIME[] = "ss-loop-time";

/**
 * @brief The number of iterations through the event loop.
 */
const char PollerInterface::K_LOOP_COUNT[] = "ss-loop-count";

}  // namespace io
}  // namespace ola
