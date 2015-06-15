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
#include "ola/base/Macro.h"
#include "ola/stl/STLUtils.h"

namespace ola {
namespace plugin {
namespace usbdmx {

/**
 * @brief Receives notifications when Widgets are added or removed.
 *
 * Classes implementing the WidgetObserver can be used with WidgetFactories to
 * receive notifcations when widgets are added or removed.
 *
 * On adding a new Widget, the appropriate NewWidget() method is called. The
 * observer can mark a widget as in-use by returning true.
 *
 * When widgets are removed, the appropriate WidgetRemoved() method is called.
 * The observer must not access the removed widget object once the call to
 * WidgetRemoved() completes.
 */
class WidgetObserver {
 public:
  virtual ~WidgetObserver() {}

  /**
   * @brief Called when a new AnymauDMX is added.
   * @param widget the new Widget, ownership is not transferred but the object
   *   may be used until the corresponding WidgetRemoved() call is made.
   * @returns true if the widget has been claimed, false if the widget was
   *   ignored.
   */
  virtual bool NewWidget(class AnymauDMX *widget) = 0;

  /**
   * @brief Called when a new EurolitePro is added.
   * @param widget the new Widget, ownership is not transferred but the object
   *   may be used until the corresponding WidgetRemoved() call is made.
   * @returns true if the widget has been claimed, false if the widget was
   *   ignored.
   */
  virtual bool NewWidget(class EurolitePro *widget) = 0;

  /**
   * @brief Called when a new Ja Rule widget is added.
   * @param widget the new Widget, ownership is not transferred but the object
   *   may be used until the corresponding WidgetRemoved() call is made.
   * @returns true if the widget has been claimed, false if the widget was
   *   ignored.
   */
  virtual bool NewWidget(class JaRuleWidget *widget) = 0;

  /**
   * @brief Called when a new ScanlimeFadecandy is added.
   * @param widget the new Widget, ownership is not transferred but the object
   *   may be used until the corresponding WidgetRemoved() call is made.
   * @returns true if the widget has been claimed, false if the widget was
   *   ignored.
   */
  virtual bool NewWidget(class ScanlimeFadecandy *widget) = 0;

  /**
   * @brief Called when a new Sunlite is added.
   * @param widget the new Widget, ownership is not transferred but the object
   *   may be used until the corresponding WidgetRemoved() call is made.
   * @returns true if the widget has been claimed, false if the widget was
   *   ignored.
   */
  virtual bool NewWidget(class Sunlite *widget) = 0;

  /**
   * @brief Called when a new VellemanK8062 is added.
   * @param widget the new Widget, ownership is not transferred but the object
   *   may be used until the corresponding WidgetRemoved() call is made.
   * @returns true if the widget has been claimed, false if the widget was
   *   ignored.
   */
  virtual bool NewWidget(class VellemanK8062 *widget) = 0;

  /**
   * @brief Called when an AnymauDMX is removed.
   * @param widget the Widget that has been removed.
   *
   * It is an error to use the widget once this call completes.
   */
  virtual void WidgetRemoved(class AnymauDMX *widget) = 0;

  /**
   * @brief Called when a EurolitePro is removed.
   * @param widget the Widget that has been removed.
   *
   * It is an error to use the widget once this call completes.
   */
  virtual void WidgetRemoved(class EurolitePro *widget) = 0;

  /**
   * @brief Called when a Ja Rule widget is removed.
   * @param widget the Widget that has been removed.
   *
   * It is an error to use the widget once this call completes.
   */
  virtual void WidgetRemoved(class JaRuleWidget *widget) = 0;

  /**
   * @brief Called when a ScanlimeFadecandy is removed.
   * @param widget the Widget that has been removed.
   *
   * It is an error to use the widget once this call completes.
   */
  virtual void WidgetRemoved(class ScanlimeFadecandy *widget) = 0;

  /**
   * @brief Called when a Sunlite is removed.
   * @param widget the Widget that has been removed.
   *
   * It is an error to use the widget once this call completes.
   */
  virtual void WidgetRemoved(class Sunlite *widget) = 0;

  /**
   * @brief Called when a VellemanK8062 is removed.
   * @param widget the Widget that has been removed.
   *
   * It is an error to use the widget once this call completes.
   */
  virtual void WidgetRemoved(class VellemanK8062 *widget) = 0;
};

/**
 * @brief Creates new Widget objects to represent DMX USB hardware.
 *
 * WidgetFactories are called when new USB devices are located. By inspecting
 * the device's vendor and product ID, they may choose to create a new Widget
 * object. The WidgetFactory then calls the WidgetObserver object to indicate a
 * new Widget has been added.
 *
 * When a USB device is removed, the factory that created a Widget from the
 * device has it's DeviceRemoved() method called. The factory should then
 * invoke WidgetRemoved on the observer object.
 */
class WidgetFactory {
 public:
  virtual ~WidgetFactory() {}

  /**
   * @brief Called when a new USB device is added.
   * @param observer The WidgetObserver to notify if this results in a new
   *   widget.
   * @param usb_device the libusb_device that was added.
   * @param descriptor the libusb_device_descriptor that corresponds to the
   *   usb_device.
   * @returns True if this factory has claimed the usb_device, false otherwise.
   */
  virtual bool DeviceAdded(
      WidgetObserver *observer,
      libusb_device *usb_device,
      const struct libusb_device_descriptor &descriptor) = 0;

  /**
   * @brief Called when a USB device is removed.
   * @param observer The WidgetObserver to notify if this action results in a
   *   widget removal.
   * @param usb_device the libusb_device that was removed.
   */
  virtual void DeviceRemoved(WidgetObserver *observer,
                             libusb_device *usb_device) = 0;
};

/**
 * @brief A partial implementation of WidgetFactory.
 *
 * This handles the mapping of libusb_devices to widgets and notifying the
 * observer when widgets are added or removed.
 */
template <typename WidgetType>
class BaseWidgetFactory : public WidgetFactory {
 public:
  BaseWidgetFactory() {}

  void DeviceRemoved(WidgetObserver *observer,
                     libusb_device *device);

 protected:
  /**
   * @brief Check if this factory is already using this device.
   * @param usb_device The libusb_device to check for.
   * @returns true if we already have a widget registered for this device,
   *   false otherwise.
   */
  bool HasDevice(libusb_device *usb_device) {
    return STLContains(m_widget_map, usb_device);
  }

  /**
   * @brief Return the number of active widgets.
   * @returns The number of active widgets.
   */
  unsigned int DeviceCount() const {
    return m_widget_map.size();
  }

  /**
   * @brief Initialize a widget and notify the observer.
   * @param observer The WidgetObserver to notify of the new widget.
   * @param usb_device the libusb_device associated with the widget.
   * @param widget the new Widget, ownership is transferred.
   * @returns True if the widget was added, false otherwise.
   */
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
void BaseWidgetFactory<WidgetType>::DeviceRemoved(WidgetObserver *observer,
                                                  libusb_device *usb_device) {
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
