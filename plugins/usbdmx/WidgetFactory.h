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
#include <string>
#include "ola/Logging.h"
#include "ola/base/Macro.h"
#include "ola/stl/STLUtils.h"

namespace ola {

namespace usb {
class JaRuleWidget;
}  // namespace usb

namespace plugin {
namespace usbdmx {

/**
 * @brief Receives notifications when Widgets are added or removed.
 *
 * Classes implementing the WidgetObserver can be used with WidgetFactories to
 * receive notifications when widgets are added.
 *
 * On adding a new Widget, the appropriate NewWidget() method is called. The
 * observer can mark a widget as in-use by returning true.
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
   * @brief Called when a new DMXCProjects Nodle U1 widget is added.
   * @param widget the new Widget, ownership is not transferred but the object
   *   may be used until the corresponding WidgetRemoved() call is made.
   * @returns true if the widget has been claimed, false if the widget was
   *   ignored.
   */
  virtual bool NewWidget(class DMXCProjectsNodleU1 *widget) = 0;

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
  virtual bool NewWidget(ola::usb::JaRuleWidget *widget) = 0;

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
};

/**
 * @brief Creates new Widget objects to represent DMX USB hardware.
 *
 * WidgetFactories are called when new USB devices are located. By inspecting
 * the device's vendor and product ID, they may choose to create a new Widget
 * object. The WidgetFactory then calls the WidgetObserver object to indicate a
 * new Widget has been added.
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
   * @brief The name of this factory.
   * @returns The name of this factory.
   */
  virtual std::string Name() const = 0;
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
  explicit BaseWidgetFactory(const std::string &name) : m_name(name) {}

  std::string Name() const { return m_name; }

 protected:
  /**
   * @brief Initialize a widget and notify the observer.
   * @param observer The WidgetObserver to notify of the new widget.
   * @param widget the new Widget, ownership is transferred.
   * @returns True if the widget was added, false otherwise.
   */
  bool AddWidget(WidgetObserver *observer, WidgetType *widget);

 private:
  const std::string m_name;

  DISALLOW_COPY_AND_ASSIGN(BaseWidgetFactory);
};

template <typename WidgetType>
bool BaseWidgetFactory<WidgetType>::AddWidget(WidgetObserver *observer,
                                              WidgetType *widget) {
  if (!widget->Init()) {
    delete widget;
    return false;
  }

  if (!observer->NewWidget(widget)) {
    delete widget;
    return false;
  }

  return true;
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_WIDGETFACTORY_H_
