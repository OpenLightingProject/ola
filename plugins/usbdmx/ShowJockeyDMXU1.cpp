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
 * ShowJockeyDMXU1.cpp
 * The synchronous and asynchronous ShowJockeyDMXU1 widgets.
 * Copyright (C) 2016 Nicolas Bertrand
 */

#include "plugins/usbdmx/ShowJockeyDMXU1.h"

#include <string.h>
#include <string>

#include "libs/usb/LibUsbAdaptor.h"
#include "ola/Constants.h"
#include "ola/Logging.h"
#include "ola/util/Utils.h"
#include "ola/strings/Format.h"
#include "plugins/usbdmx/AsyncUsbSender.h"
#include "plugins/usbdmx/ThreadedUsbSender.h"

namespace ola {
namespace plugin {
namespace usbdmx {

using std::string;
using ola::usb::LibUsbAdaptor;

namespace {

static const unsigned int URB_TIMEOUT_MS = 3000;


/*
 * Find the interface with the endpoint we're after. Usually this is interface
 * 1 but we check them all just in case.
 */
bool LocateInterface(LibUsbAdaptor *adaptor,
                     libusb_device *usb_device,
                     int *interface_number,
                     int *endpoint_addr,
                     int *max_packet_size) {
  struct libusb_config_descriptor *device_config;
  if (adaptor->GetConfigDescriptor(usb_device, 0, &device_config) != 0) {
    OLA_WARN << "Failed to get device config descriptor";
    return false;
  }

  OLA_DEBUG << static_cast<int>(device_config->bNumInterfaces)
            << " interfaces found";
  for (unsigned int i = 0; i < device_config->bNumInterfaces; i++) {
    const struct libusb_interface *interface = &device_config->interface[i];
    for (int j = 0; j < interface->num_altsetting; j++) {
      const struct libusb_interface_descriptor *iface_descriptor =
          &interface->altsetting[j];
      for (uint8_t k = 0; k < iface_descriptor->bNumEndpoints; k++) {
        const struct libusb_endpoint_descriptor *endpoint =
            &iface_descriptor->endpoint[k];
        OLA_DEBUG << "Interface " << i << ", altsetting " << j << ", endpoint "
                  << static_cast<int>(k) << ", endpoint address "
                  << ola::strings::ToHex(endpoint->bEndpointAddress);
        int current_endpoint_addr = endpoint->bEndpointAddress;
        bool isOutput = (current_endpoint_addr & LIBUSB_ENDPOINT_DIR_MASK) !=
                        LIBUSB_ENDPOINT_IN;
        if (endpoint->bmAttributes == LIBUSB_TRANSFER_TYPE_BULK && isOutput) {
          OLA_INFO << "Using interface " << i;
          *interface_number = i;
          *endpoint_addr = current_endpoint_addr;
          *max_packet_size = endpoint->wMaxPacketSize;
          adaptor->FreeConfigDescriptor(device_config);
          return true;
        }
      }
    }
  }
  OLA_WARN << "Failed to locate endpoint for ShowJockeyDMXU1 device.";
  adaptor->FreeConfigDescriptor(device_config);
  return false;
}
}  // namespace

// ShowJockeyDMXU1ThreadedSender
// -----------------------------------------------------------------------------

/*
 * Sends messages to a ShowJockeyDMXU1 device in a separate thread.
 */
class ShowJockeyDMXU1ThreadedSender: public ThreadedUsbSender {
 public:
  ShowJockeyDMXU1ThreadedSender(LibUsbAdaptor *adaptor,
                           libusb_device *usb_device,
                           libusb_device_handle *usb_handle,
                           int max_packet_size_out,
                           int endpoint);

 private:
  LibUsbAdaptor* const m_adaptor;
  int m_max_packet_size_out;
  int m_endpoint;

  bool TransmitBuffer(libusb_device_handle *handle,
                      const DmxBuffer &buffer);
  int bulkSync(libusb_device_handle *handle,
               int endpoint,
               int max_packet_size,
               unsigned char *buffer,
               int size);
};

ShowJockeyDMXU1ThreadedSender::ShowJockeyDMXU1ThreadedSender(
    LibUsbAdaptor *adaptor,
    libusb_device *usb_device,
    libusb_device_handle *usb_handle,
    int max_packet_size_out,
    int endpoint)
    : ThreadedUsbSender(usb_device, usb_handle),
      m_adaptor(adaptor),
      m_max_packet_size_out(max_packet_size_out),
      m_endpoint(endpoint) {
}

bool ShowJockeyDMXU1ThreadedSender::TransmitBuffer(libusb_device_handle *handle,
                                              const DmxBuffer &buffer) {
  if (!handle) {
    return false;
  }

  int ret_val = 0;
  int left_write_size = DMX_UNIVERSE_SIZE;
  uint16_t already_written_size = 0;
  unsigned int write_size = 0;
  int max_packet_size_out = m_max_packet_size_out;

  unsigned char *bulk_buffer = new unsigned char[max_packet_size_out]();

  int bulkDataSize = max_packet_size_out - 2;
  unsigned int slot = 0;

  while (left_write_size > 0) {
    memset(bulk_buffer, 0, max_packet_size_out);
    memcpy(bulk_buffer, &already_written_size, 2);

    if (left_write_size > bulkDataSize) {
      write_size = bulkDataSize;
    } else {
      write_size = left_write_size;
    }

    buffer.GetRange(slot, bulk_buffer + 2, &write_size);

    ret_val = bulkSync(handle, m_endpoint, max_packet_size_out,
                       bulk_buffer, write_size + 2);

    if (ret_val < 0) {
      return ret_val;
    }

    left_write_size -= write_size;
    already_written_size += write_size;
    slot += write_size;
  }

  if (bulk_buffer) {
    delete[] bulk_buffer;
    bulk_buffer = 0;
  }

  return true;
}

int ShowJockeyDMXU1ThreadedSender::bulkSync(libusb_device_handle *handle,
                                       int endpoint, int max_packet_size,
                                       unsigned char *buffer, int size) {
  if (!handle) {
    return -1;
  }

  if (size <= max_packet_size) {
    int transferred = -1;
    m_adaptor->BulkTransfer(handle, endpoint, buffer, size,
                            &transferred, URB_TIMEOUT_MS);

    return transferred;
  }

  return -1;
}

// SynchronousShowJockeyDMXU1
// -----------------------------------------------------------------------------

SynchronousShowJockeyDMXU1::SynchronousShowJockeyDMXU1(
    LibUsbAdaptor *adaptor,
    libusb_device *usb_device,
    const std::string &serial)
    : ShowJockeyDMXU1(adaptor, usb_device, serial) {
}

bool SynchronousShowJockeyDMXU1::Init() {
  libusb_device_handle *usb_handle;

  int interface_number;
  int endpoint;
  int max_packet_size;
  if (!LocateInterface(m_adaptor,
                       m_usb_device,
                       &interface_number,
                       &endpoint,
                       &max_packet_size)) {
    return false;
  }

  bool ok = m_adaptor->OpenDeviceAndClaimInterface(m_usb_device,
                                                   interface_number,
                                                   &usb_handle);
  if (!ok) {
    return false;
  }

  std::auto_ptr<ShowJockeyDMXU1ThreadedSender> sender(
      new ShowJockeyDMXU1ThreadedSender(m_adaptor,
                                        m_usb_device,
                                        usb_handle,
                                        max_packet_size,
                                        endpoint));
  if (!sender->Start()) {
    return false;
  }

  m_sender.reset(sender.release());
  return true;
}

bool SynchronousShowJockeyDMXU1::SendDMX(const DmxBuffer &buffer) {
  return m_sender.get() ? m_sender->SendDMX(buffer) : false;
}

// ShowJockeyDMXU1AsyncUsbSender
// -----------------------------------------------------------------------------
class ShowJockeyDMXU1AsyncUsbSender : public AsyncUsbSender {
 public:
  ShowJockeyDMXU1AsyncUsbSender(LibUsbAdaptor *adaptor,
                                libusb_device *usb_device,
                                int endpoint,
                                int max_packet_size_out,
                                libusb_device_handle *handle)
                                : AsyncUsbSender(adaptor, usb_device),
                                  m_endpoint(endpoint),
                                  m_max_packet_size_out(max_packet_size_out) {
    m_usb_handle = handle;
    m_tx_frame = NULL;
  }

  ~ShowJockeyDMXU1AsyncUsbSender() {
    CancelTransfer();
    delete[] m_tx_frame;
  }

  libusb_device_handle* SetupHandle() {
    return m_usb_handle;
  }

  bool PerformTransfer(const DmxBuffer &buffer) {
    uint16_t nb_sequence = DMX_MAX_SLOT_NUMBER / (m_max_packet_size_out - 2);
    nb_sequence += 1;
    int final_size = DMX_MAX_SLOT_NUMBER + (2 * nb_sequence);
    if (m_tx_frame == NULL) {
      m_tx_frame = new uint8_t[final_size]();
    }

    uint8_t *p_final_buffer = m_tx_frame;
    unsigned int to_write_size = m_max_packet_size_out - 2;
    uint16_t written_size = 0;
    for (int i = 0; i <= nb_sequence; ++i) {
      memcpy(p_final_buffer, &written_size, 2);
      p_final_buffer += 2;
      unsigned int need_to_write_size = DMX_MAX_SLOT_NUMBER - written_size;
      unsigned int will_write_size;
      if (to_write_size < need_to_write_size) {
        will_write_size = to_write_size;
      } else {
        will_write_size = need_to_write_size;
      }

      buffer.GetRange(written_size, p_final_buffer, &will_write_size);
      p_final_buffer += to_write_size;
      written_size += to_write_size;
    }

    FillBulkTransfer(m_endpoint, m_tx_frame, final_size,
                     URB_TIMEOUT_MS);
    return SubmitTransfer() == 0;
  }

 private:
  uint8_t *m_tx_frame;
  int m_endpoint;
  int m_max_packet_size_out;

  DISALLOW_COPY_AND_ASSIGN(ShowJockeyDMXU1AsyncUsbSender);
};

// AsynchronousShowJockeyDMXU1
// -----------------------------------------------------------------------------

AsynchronousShowJockeyDMXU1::AsynchronousShowJockeyDMXU1(
    LibUsbAdaptor *adaptor,
    libusb_device *usb_device,
    const std::string &serial)
    : ShowJockeyDMXU1(adaptor, usb_device, serial) {
}

bool AsynchronousShowJockeyDMXU1::Init() {
  int interface_number;
  int endpoint;
  int max_packet_size;
  if (!LocateInterface(m_adaptor,
                       m_usb_device,
                       &interface_number,
                       &endpoint,
                       &max_packet_size)) {
    return false;
  }
  libusb_device_handle *usb_handle;
  bool ok = m_adaptor->OpenDeviceAndClaimInterface(
      m_usb_device, interface_number, &usb_handle);

  if (!ok) {
    return false;
  }

  std::auto_ptr<ShowJockeyDMXU1AsyncUsbSender> sender(
      new ShowJockeyDMXU1AsyncUsbSender(m_adaptor,
                                        m_usb_device,
                                        endpoint,
                                        max_packet_size,
                                        usb_handle));
  m_sender.reset(sender.release());
  return true;
}

bool AsynchronousShowJockeyDMXU1::SendDMX(const DmxBuffer &buffer) {
  return m_sender->SendDMX(buffer);
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
