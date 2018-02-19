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
 * JaRuleWidget.h
 * A Ja Rule widget.
 * Copyright (C) 2015 Simon Newton
 */

#ifndef LIBS_USB_JARULEWIDGET_H_
#define LIBS_USB_JARULEWIDGET_H_

#include <libusb.h>
#include <stdint.h>

#include <ola/io/ByteString.h>
#include <ola/rdm/UID.h>
#include <ola/thread/ExecutorInterface.h>

#include <string>
#include <vector>

#include "libs/usb/LibUsbAdaptor.h"
#include "libs/usb/JaRuleConstants.h"
#include "libs/usb/Types.h"

namespace ola {
namespace usb {

/**
 * @brief A Ja Rule hardware device (widget).
 *
 * Ja Rule devices may have more than one DMX/RDM port.
 *
 * This class provides two ways to control the ports on the device:
 *  - The low level SendCommand() method, which sends a single request and
 *    invokes a callback when the response is received.
 *  - The high level API where the port is accessed via a JaRulePortHandle.
 *    Since the JaRulePortHandle implements the
 *    ola::rdm::DiscoverableRDMControllerInterface, the usual RDM methods can
 *    be used.
 *
 * To obtain a JaRulePortHandle, call ClaimPort(), when you're finished with
 * the JaRulePortHandle you must call ReleasePort().
 */
class JaRuleWidget {
 public:
  /**
   * @brief Create a new Ja Rule widget.
   * @param executor The Executor to run the callbacks on.
   * @param adaptor The LibUsbAdaptor to use.
   * @param usb_device the libusb_device for the Ja Rule widget.
   */
  JaRuleWidget(ola::thread::ExecutorInterface *executor,
               AsynchronousLibUsbAdaptor *adaptor,
               libusb_device *usb_device);

  /**
   * @brief Destructor
   */
  ~JaRuleWidget();

  /**
   * @brief Initialize the Ja Rule widget.
   * @returns true if the USB device was opened and claimed correctly, false
   *   otherwise.
   */
  bool Init();

  /**
   * @brief The device ID of this widget.
   * @returns The USBDeviceID.
   */
  USBDeviceID GetDeviceId() const;

  /**
   * @brief Cancel all queued and inflight commands.
   * @param port_id The port id of the commands to cancel
   *
   * This will immediately run all CommandCompleteCallbacks with the
   * COMMAND_CANCELLED code.
   */
  void CancelAll(uint8_t port_id);

  /**
   * @brief The number of ports on the widget.
   * @pre Init() has been called and returned true;
   * @returns The number of ports.
   *
   * Ports are numbered consecutively from 0.
   */
  uint8_t PortCount() const;

  /**
   * @brief The UID of the widget.
   * @pre Init() has been called and returned true;
   * @returns The UID for the device.
   */
  ola::rdm::UID GetUID() const;

  /**
   * @brief Get the manufacturer string.
   * @pre Init() has been called and returned true;
   * @returns The manufacturer string.
   */
  std::string ManufacturerString() const;

  /**
   * @brief Get the product string.
   * @pre Init() has been called and returned true;
   * @returns The product string.
   */
  std::string ProductString() const;

  /**
   * @brief Claim a handle to a port.
   * @param port_index The port to claim.
   * @returns a port handle, ownership is not transferred. Will return NULL if
   *   the port id is invalid, or already claimed.
   */
  class JaRulePortHandle* ClaimPort(uint8_t port_index);

  /**
   * @brief Release a handle to a port.
   * @param port_index The port to claim
   * @returns a port handle, ownership is not transferred.
   */
  void ReleasePort(uint8_t port_index);

  /**
   * @brief The low level API to send a command to the widget.
   * @param port_index The port on which to send the command.
   * @param command the Command type.
   * @param data the payload data. The data is copied and can be freed once the
   *   method returns.
   * @param size the payload size.
   * @param callback The callback to run when the command completes.
   * This may be run immediately in some conditions.
   *
   * SendCommand() can be called from any thread, and messages will be queued.
   */
  void SendCommand(uint8_t port_index, CommandClass command,
                   const uint8_t *data, unsigned int size,
                   CommandCompleteCallback *callback);

 private:
  typedef std::vector<class JaRuleWidgetPort*> PortHandles;

  ola::thread::ExecutorInterface *m_executor;
  LibUsbAdaptor *m_adaptor;
  libusb_device *m_device;
  libusb_device_handle *m_usb_handle;
  ola::rdm::UID m_uid;  // The UID of the device, or 0000:00000000 if unset
  std::string m_manufacturer;
  std::string m_product;
  PortHandles m_ports;  // The list of port handles.

  bool InternalInit();

  static const uint8_t SUBCLASS_VALUE = 0xff;
  static const uint8_t PROTOCOL_VALUE = 0xff;

  DISALLOW_COPY_AND_ASSIGN(JaRuleWidget);
};
}  // namespace usb
}  // namespace ola
#endif  // LIBS_USB_JARULEWIDGET_H_
