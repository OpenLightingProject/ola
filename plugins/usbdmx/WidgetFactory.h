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
 * WidgetFactory.h
 * Creates USB Widgets.
 * Copyright (C) 2014 Simon Newton
 */

#ifndef PLUGINS_USBDMX_WIDGETFACTORY_H_
#define PLUGINS_USBDMX_WIDGETFACTORY_H_

#include <libusb.h>
#include <map>
#include "ola/Logging.h"
#include "ola/stl/STLUtils.h"

namespace ola {
namespace plugin {
namespace usbdmx {

/**
 * @brief Receives notifications when new widgets are detected.
 */
class WidgetObserver {
 public:
  virtual ~WidgetObserver() {}

  /**
   * @brief Called when a new generic widget is found.
   * @param widget the new Widget.
   * @returns true if the widget is now in use, false if the widget was
   *   ignored.
   */
  virtual bool NewWidget(class Widget *widget) = 0;
  virtual bool NewWidget(class AnymaWidget *widget) = 0;
  virtual bool NewWidget(class EuroliteProWidget *widget) = 0;
  virtual bool NewWidget(class SunliteWidget *widget) = 0;

  /**
   * @brief Called when a generic widget is removed.
   * @param widget the Widget that has been removed.
   *
   * It is an error to use the widget once this call completes.
   */
  virtual void WidgetRemoved(class Widget *widget) = 0;

  virtual void WidgetRemoved(class AnymaWidget *widget) = 0;
  virtual void WidgetRemoved(class EuroliteProWidget *widget) = 0;
  virtual void WidgetRemoved(class SunliteWidget *widget) = 0;
};

/**
 * @brief 
 */
class WidgetFactory {
 public:
  virtual ~WidgetFactory() {}

  /**
   * @brief 
   * @param observer The WidgetObserver to notify if this results in a new
   * widget.
   * @param device the libusb_device that was added.
   * @param descriptor the libusb_device_descriptor that corresponds to the
   *   libusb_device.
   * @returns True if this factory has claimed the usb_device, false otherwise.
   */
  virtual bool DeviceAdded(
      WidgetObserver *observer,
      libusb_device *device,
      const struct libusb_device_descriptor &descriptor) = 0;

  /**
   * @brief 
   * @param observer The WidgetObserver to notify if this results in a widget
   *   removal.
   */
  virtual void DeviceRemoved(WidgetObserver *observer,
                             libusb_device *device) = 0;
};

/**
 * @brief A partial implementation of WidgetFactory.
 *
 * This handles the mapping of libusb_devices to widgets and notifying the
 * observer.
 */
template <typename WidgetType>
class BaseWidgetFactory : public WidgetFactory {
 public:
  BaseWidgetFactory() {}

  void DeviceRemoved(WidgetObserver *observer,
                     libusb_device *device);

 protected:
  bool HasDevice(libusb_device *device) {
    return STLContains(m_widget_map, device);
  }

  bool AddWidget(WidgetObserver *observer, libusb_device *usb_device,
                 WidgetType *widget);

 private:
  typedef std::map<libusb_device*, WidgetType*> WidgetMap;

  WidgetMap m_widget_map;

  DISALLOW_COPY_AND_ASSIGN(BaseWidgetFactory<WidgetType>);
};

template <typename WidgetType>
bool BaseWidgetFactory<WidgetType>::AddWidget(WidgetObserver *observer,
                                              libusb_device *usb_device,
                                              WidgetType *widget) {
  if (!widget->Init()) {
    delete widget;
    return false;
  }

  if (!observer->NewWidget(widget)) {
    delete widget;
    return false;
  }

  WidgetType *old_widget = STLReplacePtr(&m_widget_map, usb_device, widget);
  if (old_widget) {
    // This should never happen.
    OLA_WARN << "Widget conflict for " << usb_device;
    observer->WidgetRemoved(old_widget);
    delete old_widget;
  }
  return true;
}

template <typename WidgetType>
void BaseWidgetFactory<WidgetType>::DeviceRemoved(
    WidgetObserver *observer,
    libusb_device *usb_device) {
  OLA_INFO << "Removing device " << usb_device;
  WidgetType *widget = STLLookupAndRemovePtr(&m_widget_map, usb_device);
  if (widget) {
    observer->WidgetRemoved(widget);
    delete widget;
  }
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_WIDGETFACTORY_H_
