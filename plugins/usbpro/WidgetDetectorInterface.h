/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * WidgetDetectorInterface.h
 * The interface for WidgetDetectors.
 * Copyright (C) 2011 Simon Newton
 */

#ifndef PLUGINS_USBPRO_WIDGETDETECTORINTERFACE_H_
#define PLUGINS_USBPRO_WIDGETDETECTORINTERFACE_H_


namespace ola {
namespace io {
  class ConnectedDescriptor;
}

namespace plugin {
namespace usbpro {


/*
 * A WidgetDetector takes a ConnectedDescriptor and performs a discovery
 * routine on it. Individual WidgetDetector specify the behaviour when the
 * discovery routine passes or fails.
 */
class WidgetDetectorInterface {
  public:
    WidgetDetectorInterface() {}
    virtual ~WidgetDetectorInterface() {}

    virtual bool Discover(ola::io::ConnectedDescriptor *descriptor) = 0;
};
}  // usbpro
}  // plugin
}  // ola
#endif  // PLUGINS_USBPRO_WIDGETDETECTORINTERFACE_H_
