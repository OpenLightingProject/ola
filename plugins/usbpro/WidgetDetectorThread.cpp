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
 * WidgetDetectorThread.cpp
 * A thread that periodically looks for USB Pro devices, and runs the callback
 * if they are valid widgets.
 * Copyright (C) 2011 Simon Newton
 */


#include <string.h>
#include <string>
#include <vector>

#include "ola/Callback.h"
#include "ola/Constants.h"
#include "ola/Logging.h"
#include "ola/StringUtils.h"
#include "ola/file/Util.h"
#include "ola/io/Descriptor.h"
#include "ola/io/Serial.h"
#include "ola/stl/STLUtils.h"
#include "plugins/usbpro/ArduinoWidget.h"
#include "plugins/usbpro/BaseUsbProWidget.h"
#include "plugins/usbpro/DmxTriWidget.h"
#include "plugins/usbpro/DmxterWidget.h"
#include "plugins/usbpro/EnttecUsbProWidget.h"
#include "plugins/usbpro/RobeWidget.h"
#include "plugins/usbpro/RobeWidgetDetector.h"
#include "plugins/usbpro/UltraDMXProWidget.h"
#include "plugins/usbpro/UsbProWidgetDetector.h"
#include "plugins/usbpro/WidgetDetectorThread.h"

namespace ola {
namespace plugin {
namespace usbpro {

using ola::io::ConnectedDescriptor;
using std::string;
using std::vector;


/**
 * Constructor
 * @param handler the NewWidgetHandler object to call when we find new widgets.
 * @param ss the SelectServer to use when calling the handler object. This is
 * also used by some of the widgets so it should be the same SelectServer that
 * you intend to use the Widgets with.
 * @param usb_pro_timeout the time in ms between each USB Pro discovery message.
 * @param robe_timeout the time in ms between each Robe discovery message.
 */
WidgetDetectorThread::WidgetDetectorThread(
  NewWidgetHandler *handler,
  ola::io::SelectServerInterface *ss,
  unsigned int usb_pro_timeout,
  unsigned int robe_timeout)
    : ola::thread::Thread(),
      m_other_ss(ss),
      m_handler(handler),
      m_is_running(false),
      m_usb_pro_timeout(usb_pro_timeout),
      m_robe_timeout(robe_timeout) {
  if (!m_handler)
    OLA_FATAL << "No new widget handler registered.";
}


/**
 * Set the directory in which to look for usb widgets. This should be called
 * before Run() since it doesn't do any locking.
 * @param directory the directory path e.g. /dev
 */
void WidgetDetectorThread::SetDeviceDirectory(const string &directory) {
  m_directory = directory;
}


/**
 * Set the list of prefixes to consider as possible devices. This should be
 * called before Run() since it doesn't do any locking.
 * @param prefixes a vector of strings e.g. ["ttyUSB"]
 */
void WidgetDetectorThread::SetDevicePrefixes(const vector<string> &prefixes) {
  m_prefixes = prefixes;
}


/**
 * Set the list of devices we wish to ignore
 * @param devices a list of devices paths, e.g. /dev/ttyUSB0.NNNNNN
 */
void WidgetDetectorThread::SetIgnoredDevices(const vector<string> &devices) {
  m_ignored_devices.clear();
  vector<string>::const_iterator iter = devices.begin();
  for (; iter != devices.end(); ++iter) {
    m_ignored_devices.insert(*iter);
  }
}

/**
 * Run the discovery thread.
 */
void *WidgetDetectorThread::Run() {
  if (!m_widget_detectors.empty()) {
    OLA_WARN << "List of widget detectors isn't empty!";
  } else {
    m_widget_detectors.push_back(new UsbProWidgetDetector(
        &m_ss,
        ola::NewCallback(this, &WidgetDetectorThread::UsbProWidgetReady),
        ola::NewCallback(this, &WidgetDetectorThread::DescriptorFailed),
        m_usb_pro_timeout));
    m_widget_detectors.push_back(new RobeWidgetDetector(
        &m_ss,
        ola::NewCallback(this, &WidgetDetectorThread::RobeWidgetReady),
        ola::NewCallback(this, &WidgetDetectorThread::DescriptorFailed),
        m_robe_timeout));
  }
  RunScan();
  m_ss.RegisterRepeatingTimeout(
      SCAN_INTERVAL_MS,
      ola::NewCallback(this, &WidgetDetectorThread::RunScan));
  m_ss.Execute(
      ola::NewSingleCallback(this, &WidgetDetectorThread::MarkAsRunning));
  m_ss.Run();
  m_ss.DrainCallbacks();

  // This will trigger a call to InternalFreeWidget for any remaining widgets
  STLDeleteElements(&m_widget_detectors);

  if (!m_active_descriptors.empty())
    OLA_WARN << m_active_descriptors.size() << " are still active";

  ActiveDescriptors::const_iterator iter = m_active_descriptors.begin();
  for (; iter != m_active_descriptors.end(); ++iter) {
    OLA_INFO  << iter->first;
  }
  m_widget_detectors.clear();
  return NULL;
}


/**
 * Stop the discovery thread.
 */
bool WidgetDetectorThread::Join(void *ptr) {
  m_ss.Terminate();
  return ola::thread::Thread::Join(ptr);
}


/**
 * Indicate that this widget is no longer is use and can be deleted.
 */
void WidgetDetectorThread::FreeWidget(SerialWidgetInterface *widget) {
  // We have to remove the descriptor from the ss before closing.
  m_other_ss->RemoveReadDescriptor(widget->GetDescriptor());
  widget->GetDescriptor()->Close();

  m_ss.Execute(
      ola::NewSingleCallback(this,
                             &WidgetDetectorThread::InternalFreeWidget,
                             widget));
}


/**
 * Block until the thread is up and running
 */
void WidgetDetectorThread::WaitUntilRunning() {
  m_mutex.Lock();
  if (!m_is_running)
    m_condition.Wait(&m_mutex);
  m_mutex.Unlock();
}


/**
 * Scan for widgets, and launch the discovery process for any that we don't
 * already know about.
 */
bool WidgetDetectorThread::RunScan() {
  vector<string> device_paths;
  if (!ola::file::FindMatchingFiles(m_directory, m_prefixes, &device_paths)) {
    // We want to run the scan next time in case the problem is resolved.
    return true;
  }

  vector<string>::iterator it;
  for (it = device_paths.begin(); it != device_paths.end(); ++it) {
    if (m_active_paths.find(*it) != m_active_paths.end()) {
      continue;
    }

    if (m_ignored_devices.find(*it) != m_ignored_devices.end()) {
      continue;
    }

    // FreeBSD has .init and .lock files which we want to skip
    if (StringEndsWith(*it, ".init") || StringEndsWith(*it, ".lock")) {
      continue;
    }

    OLA_INFO << "Found potential USB Serial device at " << *it;
    ConnectedDescriptor *descriptor = BaseUsbProWidget::OpenDevice(*it);
    if (!descriptor) {
      continue;
    }

    OLA_DEBUG << "New descriptor @ " << descriptor << " for " << *it;
    PerformDiscovery(*it, descriptor);
  }
  return true;
}

/**
 * Start the discovery sequence for a widget.
 */
void WidgetDetectorThread::PerformDiscovery(const string &path,
                                            ConnectedDescriptor *descriptor) {
  m_active_descriptors[descriptor] = DescriptorInfo(path, -1);
  m_active_paths.insert(path);
  PerformNextDiscoveryStep(descriptor);
}

/**
 * Called when a new widget becomes ready. Ownership of both objects transferrs
 * to use.
 */
void WidgetDetectorThread::UsbProWidgetReady(
    ConnectedDescriptor *descriptor,
    const UsbProWidgetInformation *information) {
  // we're no longer interested in events from this widget
  m_ss.RemoveReadDescriptor(descriptor);

  if (!m_handler) {
    OLA_WARN << "No callback defined for new Usb Pro Widgets.";
    FreeDescriptor(descriptor);
    delete information;
    return;
  }

  switch (information->esta_id) {
    case DMX_KING_ESTA_ID:
      if (information->device_id == DMX_KING_ULTRA_PRO_ID) {
        // The Ultra device has two outputs
        DispatchWidget(
            new UltraDMXProWidget(descriptor),
            information);
        return;
      } else {
        // DMXKing devices are drop in replacements for a Usb Pro
        EnttecUsbProWidget::EnttecUsbProWidgetOptions options(
            information->esta_id, information->serial);
        if (information->device_id == DMX_KING_ULTRA_RDM_ID) {
          options.enable_rdm = true;
        }
        DispatchWidget(
            new EnttecUsbProWidget(m_other_ss, descriptor, options),
            information);
        return;
      }
    case GODDARD_ESTA_ID:
      if (information->device_id == GODDARD_DMXTER4_ID ||
          information->device_id == GODDARD_DMXTER4A_ID ||
          information->device_id == GODDARD_MINI_DMXTER4_ID) {
        DispatchWidget(
            new DmxterWidget(
              descriptor,
              information->esta_id,
              information->serial),
            information);
        return;
      }
      break;
    case JESE_ESTA_ID:
      if (information->device_id == JESE_DMX_TRI_MK1_ID ||
          information->device_id == JESE_RDM_TRI_MK1_ID ||
          information->device_id == JESE_RDM_TRI_MK2_ID ||
          information->device_id == JESE_RDM_TXI_MK2_ID ||
          information->device_id == JESE_DMX_TRI_MK1_SE_ID) {
        DispatchWidget(
            new DmxTriWidget(
              m_other_ss,
              descriptor),
            information);
        return;
      }
      break;
    case OPEN_LIGHTING_ESTA_CODE:
      if (information->device_id == OPEN_LIGHTING_RGB_MIXER_ID ||
          information->device_id == OPEN_LIGHTING_PACKETHEADS_ID) {
        DispatchWidget(
            new ArduinoWidget(
              descriptor,
              information->esta_id,
              information->serial),
            information);
        return;
      }
      break;
  }
  OLA_WARN << "Defaulting to a Usb Pro device";
  if (information->dual_port) {
    OLA_INFO << "Found and unlocked an Enttec USB Pro Mk II";
  }
  EnttecUsbProWidget::EnttecUsbProWidgetOptions options(
      information->esta_id, information->serial);
  options.dual_ports = information->dual_port;
  if (information->has_firmware_version) {
    // 2.4 is the first version that properly supports RDM.
    options.enable_rdm = information->firmware_version >= 0x0204;
    if (options.enable_rdm) {
      // TODO(Peter): Double check the firmware versioning
      options.no_rdm_dub_timeout = information->firmware_version >= 0x040f;
      OLA_DEBUG << "USB Pro no RDM DUB timeout behaviour: "
                << options.no_rdm_dub_timeout;
    } else {
      OLA_WARN << "USB Pro Firmware >= 2.4 is required for RDM support, this "
               << "widget is running " << (information->firmware_version >> 8)
               << "." << (information->firmware_version & 0xff);
    }
  }
  DispatchWidget(
      new EnttecUsbProWidget(m_other_ss, descriptor, options), information);
}


/**
 * Called when we discovery a robe widget.
 */
void WidgetDetectorThread::RobeWidgetReady(
    ConnectedDescriptor *descriptor,
    const RobeWidgetInformation *info) {
  // we're no longer interested in events from this descriptor
  m_ss.RemoveReadDescriptor(descriptor);
  RobeWidget *widget = new RobeWidget(descriptor, info->uid);

  if (m_handler) {
    DispatchWidget(widget, info);
  } else {
    OLA_WARN << "No callback defined for new Robe Widgets.";
    InternalFreeWidget(widget);
    delete info;
  }
}


/**
 * Called when this descriptor fails discovery
 */
void WidgetDetectorThread::DescriptorFailed(
    ConnectedDescriptor *descriptor) {
  m_ss.RemoveReadDescriptor(descriptor);
  if (descriptor->ValidReadDescriptor()) {
    PerformNextDiscoveryStep(descriptor);
  } else {
    FreeDescriptor(descriptor);
  }
}


/**
 * Perform the next step in discovery for this descriptor.
 * @pre the descriptor exists in m_active_descriptors
 */
void WidgetDetectorThread::PerformNextDiscoveryStep(
    ConnectedDescriptor *descriptor) {

  DescriptorInfo &descriptor_info = m_active_descriptors[descriptor];
  descriptor_info.second++;

  if (static_cast<unsigned int>(descriptor_info.second) ==
      m_widget_detectors.size()) {
    OLA_INFO << "no more detectors to try for  " << descriptor;
    FreeDescriptor(descriptor);
  } else {
    OLA_INFO << "trying stage " << descriptor_info.second << " for " <<
      descriptor;
    m_ss.AddReadDescriptor(descriptor);
    bool ok = m_widget_detectors[descriptor_info.second]->Discover(descriptor);
    if (!ok) {
      m_ss.RemoveReadDescriptor(descriptor);
      FreeDescriptor(descriptor);
    }
  }
}


/**
 * Free the widget and the associated descriptor.
 */
void WidgetDetectorThread::InternalFreeWidget(SerialWidgetInterface *widget) {
  ConnectedDescriptor *descriptor = widget->GetDescriptor();
  // remove descriptor from our ss if it's still there
  if (descriptor->ValidReadDescriptor()) {
    m_ss.RemoveReadDescriptor(descriptor);
  }
  delete widget;
  FreeDescriptor(descriptor);
}


/**
 * Free a descriptor and remove it from the map.
 * @param descriptor the ConnectedDescriptor to free
 */
void WidgetDetectorThread::FreeDescriptor(ConnectedDescriptor *descriptor) {
  DescriptorInfo &descriptor_info = m_active_descriptors[descriptor];

  m_active_paths.erase(descriptor_info.first);
  io::ReleaseUUCPLock(descriptor_info.first);
  m_active_descriptors.erase(descriptor);
  delete descriptor;
}


/**
 * Dispatch the widget to the caller's thread
 */
template<typename WidgetType, typename InfoType>
void WidgetDetectorThread::DispatchWidget(WidgetType *widget,
                                          const InfoType *information) {
  // default the on remove to us
  widget->GetDescriptor()->SetOnClose(NewSingleCallback(
        this,
        &WidgetDetectorThread::FreeWidget,
        reinterpret_cast<SerialWidgetInterface*>(widget)));
  ola::SingleUseCallback0<void> *cb =
    ola::NewSingleCallback(
         this,
         &WidgetDetectorThread::SignalNewWidget<WidgetType, InfoType>,
         widget,
         information);
  m_other_ss->Execute(cb);
}


/**
 * Call the handler to indicate there is a new widget available.
 */
template<typename WidgetType, typename InfoType>
void WidgetDetectorThread::SignalNewWidget(WidgetType *widget,
                                           const InfoType *information) {
  const InfoType info(*information);
  delete information;
  m_other_ss->AddReadDescriptor(widget->GetDescriptor());
  m_handler->NewWidget(widget, info);
}


/**
 * Mark this thread as running
 */
void WidgetDetectorThread::MarkAsRunning() {
  m_mutex.Lock();
  m_is_running = true;
  m_mutex.Unlock();
  m_condition.Signal();
}
}  // namespace usbpro
}  // namespace plugin
}  // namespace ola
