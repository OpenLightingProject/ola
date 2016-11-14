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
 * ShowJockey.cpp
 * The synchronous and asynchronous ShowJockey widgets.
 * Copyright (C) 2016 Nicolas Bertrand
 */

#include "plugins/usbdmx/ShowJockey.h"

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
                     int *maxPacketSize) {
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
          *maxPacketSize = endpoint->wMaxPacketSize;
          adaptor->FreeConfigDescriptor(device_config);
          return true;
        }
      }
    }
  }
  OLA_WARN << "Failed to locate endpoint for ShowJockey device.";
  adaptor->FreeConfigDescriptor(device_config);
  return false;
}
}  // namespace

// ShowJockeyThreadedSender
// -----------------------------------------------------------------------------

/*
 * Sends messages to a ShowJockey device in a separate thread.
 */
class ShowJockeyThreadedSender: public ThreadedUsbSender {
 public:
  ShowJockeyThreadedSender(LibUsbAdaptor *adaptor,
                           libusb_device *usb_device,
                           libusb_device_handle *usb_handle,
                           int maxPacketSizeOut,
                           int endpoint);

 private:
  LibUsbAdaptor* const m_adaptor;
  int m_maxPacketSizeOut;
  int m_endpoint;

  bool TransmitBuffer(libusb_device_handle *handle,
                      const DmxBuffer &buffer);
  int bulkSync(libusb_device_handle *handle,
               int endpoint,
               int maxPacketSize,
               unsigned char *buffer,
               int size);
};

ShowJockeyThreadedSender::ShowJockeyThreadedSender(
    LibUsbAdaptor *adaptor,
    libusb_device *usb_device,
    libusb_device_handle *usb_handle,
    int maxPacketSizeOut,
    int endpoint)
    : ThreadedUsbSender(usb_device, usb_handle),
      m_adaptor(adaptor),
      m_maxPacketSizeOut(maxPacketSizeOut),
      m_endpoint(endpoint) {
}

bool ShowJockeyThreadedSender::TransmitBuffer(libusb_device_handle *handle,
                                              const DmxBuffer &buffer) {
  if (!handle) {
    return false;
  }

  int retVal = 0;
  int leftWriteSize = 512;
  uint16_t alreadyWrittenSize = 0;
  unsigned int writeSize = 0;
  int maxPacketSizeOut = m_maxPacketSizeOut;

  unsigned char *bulkBuffer = new unsigned char[maxPacketSizeOut]();

  int bulkDataSize = maxPacketSizeOut - 2;
  unsigned int slot = 0;

  while (leftWriteSize > 0) {
    memset(bulkBuffer, 0, maxPacketSizeOut);
    memcpy(bulkBuffer, &alreadyWrittenSize, 2);

    if (leftWriteSize > bulkDataSize) {
      writeSize = bulkDataSize;
    } else {
      writeSize = leftWriteSize;
    }

    buffer.GetRange(slot, bulkBuffer+2, &writeSize);

    retVal = bulkSync(handle, m_endpoint, maxPacketSizeOut,
                      bulkBuffer, writeSize+2);

    if (retVal < 0) {
      return retVal;
    }

    leftWriteSize -= writeSize;
    alreadyWrittenSize += writeSize;
    slot += writeSize;
  }

  if (bulkBuffer) {
    delete[] bulkBuffer;
    bulkBuffer = 0;
  }

  return true;
}

int ShowJockeyThreadedSender::bulkSync(libusb_device_handle *handle,
                                       int endpoint, int maxPacketSize,
                                       unsigned char *buffer, int size) {
  if (!handle) {
    return -1;
  }

  if (size <= maxPacketSize) {
    int transferred = -1;
    m_adaptor->BulkTransfer(handle, endpoint, buffer, size,
                            &transferred, URB_TIMEOUT_MS);

    return transferred;
  } else {
    int retVal = 1;
    unsigned char *buf = buffer;
    int nPacket = maxPacketSize;
    int nLeft = size;
    int transferred = -1;
    int transferredAll = 0;

    while (1) {
      if (nLeft <= 0) {
        break;
      }

      retVal = m_adaptor->BulkTransfer(handle, endpoint, buffer, nPacket,
                                       &transferred, URB_TIMEOUT_MS);

      if (retVal) {
        break;
      }

      buf += transferred;
      nLeft -= transferred;
      nPacket = maxPacketSize;
      transferredAll += transferred;
    }

    if (transferredAll == size) {
      return transferredAll;
    }
  }

  return -1;
}

// SynchronousShowJockey
// -----------------------------------------------------------------------------

SynchronousShowJockey::SynchronousShowJockey(
    LibUsbAdaptor *adaptor,
    libusb_device *usb_device,
    const std::string &serial)
    : ShowJockey(adaptor, usb_device, serial) {
}

bool SynchronousShowJockey::Init() {
  libusb_device_handle *usb_handle;

  int interface_number;
  int endpoint;
  int maxPacketSize;
  if (!LocateInterface(m_adaptor,
                       m_usb_device,
                       &interface_number,
                       &endpoint,
                       &maxPacketSize)) {
    return false;
  }

  bool ok = m_adaptor->OpenDeviceAndClaimInterface(m_usb_device,
                                                   interface_number,
                                                   &usb_handle);
  if (!ok) {
    return false;
  }

  std::auto_ptr<ShowJockeyThreadedSender> sender(
      new ShowJockeyThreadedSender(m_adaptor,
                                   m_usb_device,
                                   usb_handle,
                                   maxPacketSize,
                                   endpoint));
  if (!sender->Start()) {
    return false;
  }

  m_sender.reset(sender.release());
  return true;
}

bool SynchronousShowJockey::SendDMX(const DmxBuffer &buffer) {
  return m_sender.get() ? m_sender->SendDMX(buffer) : false;
}

// ShowJockeyAsyncUsbSender
// -----------------------------------------------------------------------------
class ShowJockeyAsyncUsbSender : public AsyncUsbSender {
 public:
  ShowJockeyAsyncUsbSender(LibUsbAdaptor *adaptor,
                           libusb_device *usb_device,
                           int endpoint,
                           int maxPacketSizeOut,
                           libusb_device_handle *handle)
                           : AsyncUsbSender(adaptor, usb_device),
                             m_endpoint(endpoint),
                             m_maxPacketSizeOut(maxPacketSizeOut) {
    m_usb_handle = handle;
    m_tx_frame = NULL;
  }

  ~ShowJockeyAsyncUsbSender() {
    CancelTransfer();
    delete[] m_tx_frame;
  }

  libusb_device_handle* SetupHandle() {
    return m_usb_handle;
  }

  bool PerformTransfer(const DmxBuffer &buffer) {
    uint16_t numberOfSequence = DMX_MAX_SLOT_NUMBER / (m_maxPacketSizeOut-2);
    int finalSize = DMX_MAX_SLOT_NUMBER + 2*numberOfSequence;
    if (m_tx_frame == NULL) {
      m_tx_frame = new uint8_t[finalSize]();
    }

    uint8_t *currentBuffer = new uint8_t[DMX_MAX_SLOT_NUMBER]();
    memcpy(currentBuffer, buffer.GetRaw(), DMX_MAX_SLOT_NUMBER);
    uint8_t *p_currentBuffer = currentBuffer;
    uint8_t *p_finalBuffer = m_tx_frame;
    int toWriteSize = m_maxPacketSizeOut-2;
    uint16_t writtenSize = 0;
    for (int i = 0; i <= numberOfSequence; ++i) {
      memcpy(p_finalBuffer, &writtenSize, 2);
      p_finalBuffer += 2;
      int needToWriteSize = DMX_MAX_SLOT_NUMBER - writtenSize;
      int willWriteSize;
      if (toWriteSize < needToWriteSize) {
        willWriteSize = toWriteSize;
      } else {
        willWriteSize = needToWriteSize;
      }

      memcpy(p_finalBuffer, p_currentBuffer, willWriteSize);
      p_finalBuffer += toWriteSize;
      p_currentBuffer += toWriteSize;
      writtenSize += toWriteSize;
    }

    delete[] currentBuffer;
    FillBulkTransfer(m_endpoint, m_tx_frame, finalSize,
                     URB_TIMEOUT_MS);
    return SubmitTransfer() == 0;
  }

 private:
  uint8_t *m_tx_frame;
  int m_endpoint;
  int m_maxPacketSizeOut;

  DISALLOW_COPY_AND_ASSIGN(ShowJockeyAsyncUsbSender);
};

// AsynchronousShowJockey
// -----------------------------------------------------------------------------

AsynchronousShowJockey::AsynchronousShowJockey(
    LibUsbAdaptor *adaptor,
    libusb_device *usb_device,
    const std::string &serial)
    : ShowJockey(adaptor, usb_device, serial) {
}

bool AsynchronousShowJockey::Init() {
  int interface_number;
  int endpoint;
  int maxPacketSize;
  if (!LocateInterface(m_adaptor,
                       m_usb_device,
                       &interface_number,
                       &endpoint,
                       &maxPacketSize)) {
    return false;
  }
  libusb_device_handle *usb_handle;
  bool ok = m_adaptor->OpenDeviceAndClaimInterface(
      m_usb_device, interface_number, &usb_handle);

  if (!ok) {
    return false;
  }

  std::auto_ptr<ShowJockeyAsyncUsbSender> sender(
      new ShowJockeyAsyncUsbSender(m_adaptor,
                                   m_usb_device,
                                   endpoint,
                                   maxPacketSize,
                                   usb_handle));
  m_sender.reset(sender.release());
  return true;
}

bool AsynchronousShowJockey::SendDMX(const DmxBuffer &buffer) {
  return m_sender->SendDMX(buffer);
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
