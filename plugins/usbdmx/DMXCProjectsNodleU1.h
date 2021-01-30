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
 * DMXCProjectsNodleU1.h
 * The synchronous and asynchronous Nodle widgets.
 * Copyright (C) 2015 Stefan Krupop
 */

#ifndef PLUGINS_USBDMX_DMXCPROJECTSNODLEU1_H_
#define PLUGINS_USBDMX_DMXCPROJECTSNODLEU1_H_

#include <libusb.h>
#include <memory>
#include <string>

#include "libs/usb/LibUsbAdaptor.h"
#include "ola/Callback.h"
#include "ola/DmxBuffer.h"
#include "ola/base/Macro.h"
#include "ola/thread/Mutex.h"
#include "olad/PluginAdaptor.h"
#include "plugins/usbdmx/Widget.h"

namespace ola {
namespace plugin {
namespace usbdmx {

/**
 * @brief The interface for the Nodle Widgets
 */
class DMXCProjectsNodleU1: public SimpleWidget {
 public:
  explicit DMXCProjectsNodleU1(ola::usb::LibUsbAdaptor *adaptor,
                               libusb_device *usb_device,
                               PluginAdaptor *plugin_adaptor,
                               const std::string &serial,
                               unsigned int mode)
      : SimpleWidget(adaptor, usb_device),
        m_serial(serial),
        m_mode(mode),
        m_plugin_adaptor(plugin_adaptor) {
  }

  /**
   * @brief Get the serial number of this widget.
   * @returns The serial number of the widget.
   */
  std::string SerialNumber() const {
    return m_serial;
  }

  /**
   * @brief Get the current mode of this widget.
   * @returns The mode of the widget.
   */
  unsigned int Mode() const {
    return m_mode;
  }

  virtual void SetDmxCallback(Callback0<void> *callback) = 0;
  virtual const DmxBuffer &GetDmxInBuffer() = 0;

  static int NODLE_DEFAULT_MODE;
  static int NODLE_MIN_MODE;
  static int NODLE_MAX_MODE;

  static int OUTPUT_ENABLE_MASK;
  static int INPUT_ENABLE_MASK;

 private:
  std::string m_serial;
  bool m_rp2040AdvancedMode;

 protected:
  unsigned int m_mode;
  PluginAdaptor *m_plugin_adaptor;
};

/**
 * @brief An Nodle widget that uses synchronous libusb operations.
 *
 * Internally this spawns a new thread to avoid blocking SendDMX() calls.
 */
class SynchronousDMXCProjectsNodleU1: public DMXCProjectsNodleU1 {
 public:
  /**
   * @brief Create a new SynchronousDMXCProjectsNodleU1.
   * @param adaptor the LibUsbAdaptor to use.
   * @param usb_device the libusb_device to use for the widget.
   * @param plugin_adaptor the PluginAdaptor used to execute callbacks
   * @param serial the serial number of this widget
   * @param mode the send/receive mode to be used by the widget.
   */
  SynchronousDMXCProjectsNodleU1(ola::usb::LibUsbAdaptor *adaptor,
                                 libusb_device *usb_device,
                                 PluginAdaptor *plugin_adaptor,
                                 const std::string &serial,
                                 unsigned int mode);

  bool Init();

  bool SendDMX(const DmxBuffer &buffer);

  void SetDmxCallback(Callback0<void> *callback);
  const DmxBuffer &GetDmxInBuffer();

 private:
  libusb_device* const m_usb_device;
  std::auto_ptr<class DMXCProjectsNodleU1ThreadedSender> m_sender;
  std::auto_ptr<class DMXCProjectsNodleU1ThreadedReceiver> m_receiver;

  DISALLOW_COPY_AND_ASSIGN(SynchronousDMXCProjectsNodleU1);
};

/**
 * @brief An Nodle widget that uses asynchronous libusb operations.
 */
class AsynchronousDMXCProjectsNodleU1 : public DMXCProjectsNodleU1 {
 public:
  /**
   * @brief Create a new AsynchronousDMXCProjectsNodleU1.
   * @param adaptor the LibUsbAdaptor to use.
   * @param usb_device the libusb_device to use for the widget.
   * @param plugin_adaptor the PluginAdaptor used to execute callbacks
   * @param serial the serial number of this widget
   * @param mode the send/receive mode to be used by the widget.
   */
  AsynchronousDMXCProjectsNodleU1(ola::usb::LibUsbAdaptor *adaptor,
                                  libusb_device *usb_device,
                                  PluginAdaptor *plugin_adaptor,
                                  const std::string &serial,
                                  unsigned int mode);

  bool Init();

  bool SendDMX(const DmxBuffer &buffer);

  void SetDmxCallback(Callback0<void> *callback);
  const DmxBuffer &GetDmxInBuffer();

 private:
  std::auto_ptr<class DMXCProjectsNodleU1AsyncUsbSender> m_sender;
  std::auto_ptr<class DMXCProjectsNodleU1AsyncUsbReceiver> m_receiver;
  DmxBuffer m_buffer;

  DISALLOW_COPY_AND_ASSIGN(AsynchronousDMXCProjectsNodleU1);
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_DMXCPROJECTSNODLEU1_H_
