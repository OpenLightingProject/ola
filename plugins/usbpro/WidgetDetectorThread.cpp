/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * WidgetDetectorThread.cpp
 * A thread that periodically looks for usb pro devices, and runs the callback
 * if they are valid widgets.
 * Copyright (C) 2011 Simon Newton
 */


#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <iostream>
#include <string>
#include <vector>

#include "ola/Logging.h"
#include "ola/Callback.h"
#include "ola/network/Socket.h"
#include "plugins/usbpro/BaseUsbProWidget.h"
#include "plugins/usbpro/RobeWidget.h"
#include "plugins/usbpro/RobeWidgetDetector.h"
#include "plugins/usbpro/UsbProWidgetDetector.h"
#include "plugins/usbpro/WidgetDetectorThread.h"

namespace ola {
namespace plugin {
namespace usbpro {


/**
 * Constructor
 */
WidgetDetectorThread::WidgetDetectorThread(
  UsbProCallback *usb_pro_callback,
  RobeCallback *robe_callback)
    : m_usb_pro_callback(usb_pro_callback),
      m_robe_callback(robe_callback) {
  if (!m_usb_pro_callback)
    OLA_FATAL << "No callback registered for the WidgetDetectorThread";
  if (!m_robe_callback)
    OLA_FATAL << "No callback registered for the WidgetDetectorThread";
}


/**
 * Clean up
 */
WidgetDetectorThread::~WidgetDetectorThread() {
  if (m_usb_pro_callback)
    delete m_usb_pro_callback;
  if (m_robe_callback)
    delete m_robe_callback;
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
 * Run the discovery thread.
 */
void *WidgetDetectorThread::Run() {
  if (!m_widget_detectors.empty()) {
    OLA_WARN << "List of widget detectors isn't empty!";
  } else {
    m_widget_detectors.push_back(new UsbProWidgetDetector(
        &m_ss,
        ola::NewCallback(this, &WidgetDetectorThread::UsbProWidgetReady),
        ola::NewCallback(this, &WidgetDetectorThread::DescriptorFailed)));
    m_widget_detectors.push_back(new RobeWidgetDetector(
        &m_ss,
        ola::NewCallback(this, &WidgetDetectorThread::RobeWidgetReady),
        ola::NewCallback(this, &WidgetDetectorThread::DescriptorFailed)));
  }
  RunScan();
  m_ss.RegisterRepeatingTimeout(
      SCAN_INTERVAL_MS,
      ola::NewCallback(this, &WidgetDetectorThread::RunScan));
  m_ss.Run();

  vector<WidgetDetectorInterface*>::const_iterator iter =
  m_widget_detectors.begin();
  for (;iter != m_widget_detectors.end(); ++iter)
    // this will trigger a call to InternalFreeWidget for any remaining widgets
    delete *iter;

  if (!m_active_descriptors.empty())
    OLA_WARN << m_active_descriptors.size() << " are still active";

  ActiveDescriptors::const_iterator iter2 = m_active_descriptors.begin();
  for (; iter2 != m_active_descriptors.end(); ++iter2) {
    OLA_INFO  << iter2->first;
  }
  m_widget_detectors.clear();
  return NULL;
}


/**
 * Stop the discovery thread.
 */
bool WidgetDetectorThread::Join(void *ptr) {
  m_ss.Terminate();
  return OlaThread::Join(ptr);
}


/**
 * Indicate that this widget is no longer is use and can be deleted.
 * This can be called from any thread.
 */
void WidgetDetectorThread::FreeWidget(UsbWidgetInterface *widget) {
  m_ss.Execute(
      ola::NewSingleCallback(this,
                             &WidgetDetectorThread::InternalFreeWidget,
                             widget));
}


/**
 * Scan for widgets, and launch the discovery process for any that we don't
 * already know about.
 */
bool WidgetDetectorThread::RunScan() {
  vector<string> device_paths;
  FindCandiateDevices(&device_paths);

  vector<string>::iterator it;
  for (it = device_paths.begin(); it != device_paths.end(); ++it) {
    if (m_active_paths.find(*it) != m_active_paths.end())
      continue;
    OLA_INFO << "Found potential USB Pro like device at " << *it;
    ola::network::ConnectedDescriptor *descriptor =
      BaseUsbProWidget::OpenDevice(*it);
    if (!descriptor)
      continue;

    OLA_INFO << "new descriptor @ " << descriptor << " for " << *it;
    m_active_descriptors[descriptor] = DescriptorInfo(*it, -1);
    m_active_paths.insert(*it);
    PerformNextDiscoveryStep(descriptor);
  }
  return true;
}


/*
 * Look for candidate devices in m_directory
 * @param device_paths a pointer to a vector to be populated with paths in
 * m_directory that match m_prefixes.
 */
void WidgetDetectorThread::FindCandiateDevices(vector<string> *device_paths) {
  if (!(m_directory.empty() || m_prefixes.empty())) {
    DIR *dp;
    struct dirent dir_ent;
    struct dirent *dir_ent_p;
    if ((dp  = opendir(m_directory.data())) == NULL) {
        OLA_WARN << "Could not open " << m_directory << ":" << strerror(errno);
        return;
    }

    readdir_r(dp, &dir_ent, &dir_ent_p);
    while (dir_ent_p != NULL) {
      vector<string>::const_iterator iter;
      for (iter = m_prefixes.begin(); iter != m_prefixes.end();
           ++iter) {
        if (!strncmp(dir_ent_p->d_name, iter->data(), iter->size())) {
          stringstream str;
          str << m_directory << "/" << dir_ent_p->d_name;
          device_paths->push_back(str.str());
        }
      }
      readdir_r(dp, &dir_ent, &dir_ent_p);
    }
    closedir(dp);
  }
}


/**
 * Called when a new widget becomes ready. Ownership of both objects transferrs
 * to use.
 */
void WidgetDetectorThread::UsbProWidgetReady(
    BaseUsbProWidget *widget,
    const UsbProWidgetInformation *info) {
  // we're no longer interested in events from this widget
  m_ss.RemoveReadDescriptor(widget->GetDescriptor());

  if (m_usb_pro_callback) {
    // default the remove handler to us.
    widget->SetOnRemove(NewSingleCallback(this,
                        &WidgetDetectorThread::FreeWidget,
                        reinterpret_cast<UsbWidgetInterface*>(widget)));
    m_usb_pro_callback->Run(widget, info);
  } else {
    OLA_WARN << "No callback defined for new Usb Pro Widgets.";
    InternalFreeWidget(widget);
    delete info;
  }
}


/**
 * Called when we discovery a robe widget.
 */
void WidgetDetectorThread::RobeWidgetReady(
    RobeWidget *widget,
    const RobeWidgetInformation *info) {
  // we're no longer interested in events from this widget
  m_ss.RemoveReadDescriptor(widget->GetDescriptor());

  if (m_robe_callback) {
    // default the remove handler to us.
    widget->SetOnRemove(NewSingleCallback(this,
                        &WidgetDetectorThread::FreeWidget,
                        reinterpret_cast<UsbWidgetInterface*>(widget)));
    m_robe_callback->Run(widget, info);
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
    ola::network::ConnectedDescriptor *descriptor) {
  m_ss.RemoveReadDescriptor(descriptor);
  PerformNextDiscoveryStep(descriptor);
}


/**
 * Perform the next step in discovery for this descriptor.
 * @pre the descriptor exists in m_active_descriptors
 */
void WidgetDetectorThread::PerformNextDiscoveryStep(
    ola::network::ConnectedDescriptor *descriptor) {

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
    if (!ok)
      FreeDescriptor(descriptor);
  }
}


/**
 * Free the widget and the associated descriptor.
 */
void WidgetDetectorThread::InternalFreeWidget(UsbWidgetInterface *widget) {
  ola::network::ConnectedDescriptor *descriptor = widget->GetDescriptor();
  // remove descriptor from our ss if it's there
  OLA_INFO << "removing descriptor " << descriptor;
  m_ss.RemoveReadDescriptor(descriptor);
  delete widget;

  FreeDescriptor(descriptor);
  OLA_INFO << "internal free done";
}


/**
 * Free a descriptor and remove it from the map.
 * @param descriptor the ConnectedDescriptor to free
 */
void WidgetDetectorThread::FreeDescriptor(ConnectedDescriptor *descriptor) {
  DescriptorInfo &descriptor_info = m_active_descriptors[descriptor];
  m_active_paths.erase(descriptor_info.first);
  m_active_descriptors.erase(descriptor);
  delete descriptor;
}
}  // usbpro
}  // plugin
}  // ola
