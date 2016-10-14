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
 * DMXCreator.cpp
 * The synchronous and asynchronous DMXCreator widgets.
 * Copyright (C) 2014 Simon Newton
 */

#include "plugins/usbdmx/DMXCreator.h"

#include <unistd.h>
#include <string>
#include <string.h>

#include "libs/usb/LibUsbAdaptor.h"
#include "ola/Logging.h"
#include "ola/Constants.h"
#include "plugins/usbdmx/AsyncUsbSender.h"
#include "plugins/usbdmx/ThreadedUsbSender.h"

/**
 * DMXCreator sends two URB packets for each change: 
 * 1. A constant byte string to endpoint 1 that indicates new data.
 * 2. The actual DMX data to endpoint 2.
 */

namespace ola {
namespace plugin {
namespace usbdmx {

using std::string;
using ola::usb::LibUsbAdaptor;

namespace {

static const unsigned char ENDPOINT_1 = 0x01;
static const unsigned char ENDPOINT_2 = 0x02;
static const unsigned int URB_TIMEOUT_MS_SYNC = 1000;
static const unsigned int URB_TIMEOUT_MS_ASYNC = 500;
static const unsigned int CHANNELS_PER_PACKET = 256;

void AsyncCallback(struct libusb_transfer *transfer);

}  // namespace

// DMXCreatorThreadedSender
// -----------------------------------------------------------------------------

/**
 * Sends messages to a DMXCreator device in a separate thread.
 */
class DMXCreatorThreadedSender: public ThreadedUsbSender {
 public:
	DMXCreatorThreadedSender(LibUsbAdaptor *adaptor,
											libusb_device *usb_device,
											libusb_device_handle *handle)
			: ThreadedUsbSender(usb_device, handle),
				m_adaptor(adaptor) {
		m_control_setup_buffer = new uint8_t[CHANNELS_PER_PACKET];
	}

 private:
	uint8_t *m_control_setup_buffer;
	LibUsbAdaptor* const m_adaptor;


	bool TransmitBuffer(libusb_device_handle *handle,
											const DmxBuffer &buffer);
};

bool DMXCreatorThreadedSender::TransmitBuffer(libusb_device_handle *handle,
																				 const DmxBuffer &buffer) {
  unsigned int length = CHANNELS_PER_PACKET;

	uint8_t *tmp_buffer = new uint8_t[CHANNELS_PER_PACKET];
	memset(tmp_buffer, 0, length);
	buffer.Get(tmp_buffer, &length);

	if (memcmp(tmp_buffer, m_control_setup_buffer, CHANNELS_PER_PACKET) == 0) {
		// no change -> don't send anything
		return true;
	}

	unsigned char status_buffer[6] = {
		0x80, 0x01, 0x00, 0x00, 0x00, 0x01
	};

	int bytes_sent = 0;

	length = CHANNELS_PER_PACKET;
	memset(m_control_setup_buffer, 0, length);
	buffer.Get(m_control_setup_buffer, &length);

	int r = m_adaptor->BulkTransfer(
		handle,
		ENDPOINT_1,
		status_buffer,
		sizeof(status_buffer),
		&bytes_sent,
		URB_TIMEOUT_MS_SYNC
	);

	OLA_INFO << "Sending status bytes returned " << r;

	// Sometimes we get PIPE errors, those are non-fatal
	if (r < 0 && r != LIBUSB_ERROR_PIPE) {
		OLA_WARN << "Sending status bytes failed";
		return false;
	}

	bytes_sent = 0;
	r = m_adaptor->BulkTransfer(
		handle,
		ENDPOINT_2,
		const_cast<unsigned char*>(m_control_setup_buffer),
		CHANNELS_PER_PACKET,
		&bytes_sent,
		URB_TIMEOUT_MS_SYNC
	);
	OLA_INFO << "Sending data bytes returned " << r;
	return r >= 0 || r == LIBUSB_ERROR_PIPE;
}


// SynchronousDMXCreator
// -----------------------------------------------------------------------------

SynchronousDMXCreator::SynchronousDMXCreator(LibUsbAdaptor *adaptor,
																					 libusb_device *usb_device,
																					 const string &serial)
		: DMXCreator(adaptor, usb_device, serial) {
}

bool SynchronousDMXCreator::Init() {
	libusb_device_handle *usb_handle;

	bool ok = m_adaptor->OpenDeviceAndClaimInterface(
			m_usb_device, 0, &usb_handle);
	if (!ok) {
		return false;
	}

	std::auto_ptr<DMXCreatorThreadedSender> sender(
			new DMXCreatorThreadedSender(m_adaptor, m_usb_device, usb_handle));
	if (!sender->Start()) {
		return false;
	}
	m_sender.reset(sender.release());
	return true;
}

bool SynchronousDMXCreator::SendDMX(const DmxBuffer &buffer) {
	return m_sender.get() ? m_sender->SendDMX(buffer) : false;
}

// DMXCreatorAsyncUsbSender
// -----------------------------------------------------------------------------
class DMXCreatorAsyncUsbSender : public AsyncUsbSender {
 public:
	DMXCreatorAsyncUsbSender(LibUsbAdaptor *adaptor, libusb_device *usb_device)
			: AsyncUsbSender(adaptor, usb_device),
			  m_adaptor(adaptor),
			  m_usb_device(usb_device) {
		m_control_setup_buffer = new uint8_t[CHANNELS_PER_PACKET];
	}

	~DMXCreatorAsyncUsbSender() {
		CancelTransfer();
		delete[] m_control_setup_buffer;
	}

	libusb_device_handle* SetupHandle() {
		libusb_device_handle *usb_handle;
		bool ok = m_adaptor->OpenDeviceAndClaimInterface(
				m_usb_device, 0, &usb_handle);
		return ok ? usb_handle : NULL;
	}

	bool PerformTransfer(const DmxBuffer &buffer) {
		unsigned char status_buffer[6] = {
			0x80, 0x01, 0x00, 0x00, 0x00, 0x01
		};

	  unsigned int length = CHANNELS_PER_PACKET;
		memset(m_control_setup_buffer, 0, length);
		buffer.Get(m_control_setup_buffer, &length);

		m_adaptor->FillBulkTransfer(
			m_transfer, m_usb_handle, ENDPOINT_1, status_buffer,
			sizeof(status_buffer), &AsyncCallback, this, URB_TIMEOUT_MS_ASYNC
		);

		return (SubmitTransfer() == 0);
	}

	void PerformSecondTransfer() {
		FillBulkTransfer(
			ENDPOINT_2,
			const_cast<unsigned char*>(m_control_setup_buffer),
			CHANNELS_PER_PACKET,
			URB_TIMEOUT_MS_ASYNC
		);
		SubmitTransfer();
	}


 private:
	uint8_t *m_control_setup_buffer;
	ola::usb::LibUsbAdaptor* const m_adaptor;
	libusb_device* const m_usb_device;

	DISALLOW_COPY_AND_ASSIGN(DMXCreatorAsyncUsbSender);
};

// AsynchronousDMXCreator
// -----------------------------------------------------------------------------

AsynchronousDMXCreator::AsynchronousDMXCreator(
		LibUsbAdaptor *adaptor,
		libusb_device *usb_device,
		const string &serial)
		: DMXCreator(adaptor, usb_device, serial) {
	m_sender.reset(new DMXCreatorAsyncUsbSender(m_adaptor, usb_device));
}

bool AsynchronousDMXCreator::Init() {
	return m_sender->Init();
}

bool AsynchronousDMXCreator::SendDMX(const DmxBuffer &buffer) {
	return m_sender->SendDMX(buffer);
}

namespace {

void AsyncCallback(struct libusb_transfer *transfer) {
  OLA_DEBUG << "Callback called, libusb_transfer_status " << transfer->status;

  DMXCreatorAsyncUsbSender *sender = reinterpret_cast<DMXCreatorAsyncUsbSender*>(
    transfer->user_data);

	sender->PerformSecondTransfer();
}

}

}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
