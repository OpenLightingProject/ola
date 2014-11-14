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
 * Widget.h
 * A generic USB Widget.
 * Copyright (C) 2014 Simon Newton
 */

#ifndef PLUGINS_USBDMX_WIDGET_H_
#define PLUGINS_USBDMX_WIDGET_H_

#include "ola/DmxBuffer.h"

namespace ola {
namespace plugin {
namespace usbdmx {

/**
 * @brief A generic USB Widget.
 */
class Widget {
 public:
  virtual ~Widget() {}

  virtual bool Init() = 0;

  virtual bool SendDMX(const DmxBuffer &buffer) = 0;
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_WIDGET_H_
