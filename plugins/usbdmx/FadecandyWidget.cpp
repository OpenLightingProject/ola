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
 * FadecandyWidget.cpp
 * The synchronous and asynchronous Fadecandy widgets.
 * Copyright (C) 2014 Simon Newton
 */

#include "plugins/usbdmx/FadecandyWidget.h"

#include <string.h>
#include <unistd.h>
#include <algorithm>
#include <limits>
#include <string>

#include "ola/Constants.h"
#include "ola/Logging.h"
#include "ola/StringUtils.h"
#include "ola/util/Utils.h"
#include "plugins/usbdmx/AsyncUsbSender.h"
#include "plugins/usbdmx/LibUsbAdaptor.h"
#include "plugins/usbdmx/ThreadedUsbSender.h"

namespace ola {
namespace plugin {
namespace usbdmx {

using std::string;

namespace {

static const unsigned char ENDPOINT = 0x01;
// 2s is a really long time. Can we reduce this?
static const unsigned int URB_TIMEOUT_MS = 2000;
static const int INTERFACE = 0;

// A data frame.
static const uint8_t TYPE_FRAMEBUFFER = 0x00;
// The color lookup table
static const uint8_t TYPE_LUT = 0x40;
// The initial setup message.
static const uint8_t CONFIG_MESSAGE = 0x80;
// The final packet in a set.
static const uint8_t FINAL = 0x20;

// Options used in the first data byte of the config message.
static const uint8_t OPTION_NO_DITHERING = 0x01;
static const uint8_t OPTION_NO_INTERPOLATION = 0x02;
// static const uint8_t OPTION_NO_ACTIVITY_LED  = 0x03;
// static const uint8_t OPTION_LED_CONTROL = 0x04;

// Each 'packet' is 63 bytes, or 21 RGB pixels.
enum { SLOTS_PER_PACKET = 63 };
static const uint8_t PACKETS_PER_UPDATE = 25;

PACK(
struct fadecandy_packet {
  uint8_t type;
  uint8_t data[SLOTS_PER_PACKET];

  void Reset() {
    type = 0;
    memset(data, 0, sizeof(data));
  }

  fadecandy_packet() {
    Reset();
  }
});

bool InitializeWidget(LibUsbAdaptor *adaptor,
                      libusb_device_handle *usb_handle) {
  // Set the fadecandy configuration.
  fadecandy_packet packet;
  packet.type = CONFIG_MESSAGE;
  packet.data[0] = OPTION_NO_DITHERING | OPTION_NO_INTERPOLATION;

  // packet.data[0] = OPTION_NO_ACTIVITY_LED;  // Manual control of LED
  // packet.data[0] |= OPTION_LED_CONTROL;  // Manual LED state

  int bytes_sent = 0;
  adaptor->BulkTransfer(usb_handle, ENDPOINT,
                        reinterpret_cast<unsigned char*>(&packet),
                        sizeof(packet), &bytes_sent, URB_TIMEOUT_MS);
  OLA_INFO << "Config transferred " << bytes_sent << " bytes";

  // Build the Look Up Table
  uint16_t lut[3][257];
  memset(&lut, 0, sizeof(lut));
  for (unsigned int channel = 0; channel < 3; channel++) {
    for (unsigned int value = 0; value < 257; value++) {
      // Fadecandy Python Example
      // lut[channel][value] = std::min(
      //     static_cast<int>(std::numeric_limits<uint16_t>::max()),
      //     int(pow(value / 256.0, 2.2) *
      //         (std::numeric_limits<uint16_t>::max() + 1)));
      // 1:1
      lut[channel][value] = std::min(
          static_cast<unsigned int>(std::numeric_limits<uint16_t>::max()),
          (value << 8));
      OLA_DEBUG << "Generated LUT for channel " << channel << " value "
                << value << " with val " << lut[channel][value];
    }
  }

  OLA_INFO << "LUT size " << (sizeof(lut) / 2);
  unsigned int index = 0;
  packet.Reset();

  // Transfer the lookup table.
  for (unsigned int channel = 0; channel < 3; channel++) {
    for (unsigned int value = 0; value < 257; value++) {
      unsigned int entry = (channel * 257) + value;
      unsigned int packet_entry = entry % 31;
      OLA_DEBUG << "Working on channel " << channel << " value " << value
                << " (" << IntToHexString(value) << ") with entry " << entry
                << ", packet entry " << packet_entry << " with val "
                << IntToHexString(lut[channel][value]);
      ola::utils::SplitUInt16(lut[channel][value],
                              &packet.data[packet_entry + 1],
                              &packet.data[packet_entry]);
      if ((packet_entry == 30) || (entry == ((3*257) - 1))) {
        packet.type = TYPE_LUT | index;
        if (entry == ((3*257) - 1)) {
          OLA_DEBUG << "Setting final flag on packet";
          packet.type = FINAL;
        }
        packet.type = TYPE_LUT | index;
        packet.data[0] = 0;  // Reserved

        // Send the data
        int lut_txed = 0;

        // TODO(Peter): Fix the calculations and transmit this
        // m_adaptor->BulkTransfer(usb_handle,
        //                         ENDPOINT,
        //                         packet,
        //                         sizeof(packet),
        //                         &lut_txed,
        //                         URB_TIMEOUT_MS);

        OLA_INFO << "LUT packet " << index << " transferred " << lut_txed
                 << " bytes";

        // Get ready for the next packet
        index++;
        packet.Reset();
      }
    }
  }
  return true;
}

void UpdatePacketsWithDMX(fadecandy_packet packets[PACKETS_PER_UPDATE],
                          const DmxBuffer &buffer) {
  for (unsigned int packet_index = 0; packet_index < PACKETS_PER_UPDATE;
       packet_index++) {
    packets[packet_index].Reset();

    unsigned int dmx_offset = packet_index * SLOTS_PER_PACKET;
    unsigned int slots_in_packet = SLOTS_PER_PACKET;
    buffer.GetRange(dmx_offset, packets[packet_index].data,
                    &slots_in_packet);

    packets[packet_index].type = TYPE_FRAMEBUFFER | packet_index;
    if (packet_index == PACKETS_PER_UPDATE - 1) {
      packets[packet_index].type |= FINAL;
    }
  }
}

}  // namespace

// FadecandyThreadedSender
// -----------------------------------------------------------------------------

/*
 * Sends messages to a Fadecandy device in a separate thread.
 */
class FadecandyThreadedSender: public ThreadedUsbSender {
 public:
  FadecandyThreadedSender(LibUsbAdaptor *adaptor,
                         libusb_device *usb_device,
                         libusb_device_handle *handle)
      : ThreadedUsbSender(usb_device, handle),
        m_adaptor(adaptor) {
  }

 private:
  LibUsbAdaptor* const m_adaptor;
  fadecandy_packet m_data_packets[PACKETS_PER_UPDATE];

  bool TransmitBuffer(libusb_device_handle *handle,
                      const DmxBuffer &buffer);
};

bool FadecandyThreadedSender::TransmitBuffer(libusb_device_handle *handle,
                                             const DmxBuffer &buffer) {
  UpdatePacketsWithDMX(m_data_packets, buffer);

  int bytes_sent = 0;
  // We do a single bulk transfer of the entire data, rather than on transfer
  // for each 64 bytes.
  int r = m_adaptor->BulkTransfer(
      handle, ENDPOINT,
      reinterpret_cast<unsigned char*>(&m_data_packets),
      sizeof(m_data_packets), &bytes_sent,
      URB_TIMEOUT_MS);
  return r == 0;
}

// SynchronousFadecandyWidget
// -----------------------------------------------------------------------------

SynchronousFadecandyWidget::SynchronousFadecandyWidget(
    LibUsbAdaptor *adaptor,
    libusb_device *usb_device,
    const std::string &serial)
    : FadecandyWidget(adaptor, serial),
      m_usb_device(usb_device) {
}

bool SynchronousFadecandyWidget::Init() {
  libusb_device_handle *usb_handle;

  bool ok = m_adaptor->OpenDeviceAndClaimInterface(
      m_usb_device, INTERFACE, &usb_handle);
  if (!ok) {
    return false;
  }

  if (!InitializeWidget(m_adaptor, usb_handle)) {
    m_adaptor->Close(usb_handle);
    return false;
  }

  std::auto_ptr<FadecandyThreadedSender> sender(
      new FadecandyThreadedSender(m_adaptor, m_usb_device, usb_handle));
  if (!sender->Start()) {
    return false;
  }
  m_sender.reset(sender.release());
  return true;
}

bool SynchronousFadecandyWidget::SendDMX(const DmxBuffer &buffer) {
  return m_sender.get() ? m_sender->SendDMX(buffer) : false;
}

// FadecandyAsyncUsbSender
// -----------------------------------------------------------------------------
class FadecandyAsyncUsbSender : public AsyncUsbSender {
 public:
  FadecandyAsyncUsbSender(LibUsbAdaptor *adaptor,
                         libusb_device *usb_device)
      : AsyncUsbSender(adaptor, usb_device) {
  }

  libusb_device_handle* SetupHandle();

  bool PerformTransfer(const DmxBuffer &buffer);

 private:
  fadecandy_packet m_data_packets[PACKETS_PER_UPDATE];

  DISALLOW_COPY_AND_ASSIGN(FadecandyAsyncUsbSender);
};

libusb_device_handle* FadecandyAsyncUsbSender::SetupHandle() {
  libusb_device_handle *usb_handle;
  if (!m_adaptor->OpenDeviceAndClaimInterface(
          m_usb_device, INTERFACE, &usb_handle)) {
    return NULL;
  }

  if (!InitializeWidget(m_adaptor, usb_handle)) {
    m_adaptor->Close(usb_handle);
    return NULL;
  }
  return usb_handle;
}

bool FadecandyAsyncUsbSender::PerformTransfer(const DmxBuffer &buffer) {
  UpdatePacketsWithDMX(m_data_packets, buffer);
  // We do a single bulk transfer of the entire data, rather than on transfer
  // for each 64 bytes.
  FillBulkTransfer(ENDPOINT,
                   reinterpret_cast<unsigned char*>(&m_data_packets),
                   sizeof(m_data_packets),
                   URB_TIMEOUT_MS);
  return SubmitTransfer() == 0;
}

// AsynchronousFadecandyWidget
// -----------------------------------------------------------------------------

AsynchronousFadecandyWidget::AsynchronousFadecandyWidget(
    LibUsbAdaptor *adaptor,
    libusb_device *usb_device,
    const std::string &serial)
    : FadecandyWidget(adaptor, serial) {
  m_sender.reset(new FadecandyAsyncUsbSender(m_adaptor, usb_device));
}

bool AsynchronousFadecandyWidget::Init() {
  return m_sender->Init();
}

bool AsynchronousFadecandyWidget::SendDMX(const DmxBuffer &buffer) {
  return m_sender->SendDMX(buffer);
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
