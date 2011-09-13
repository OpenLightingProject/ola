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
#include "plugins/usbpro/UsbWidget.h"
#include "plugins/usbpro/WidgetDetectorThread.h"

namespace ola {
namespace plugin {
namespace usbpro {


/**
 * Constructor
 */
WidgetDetectorThread::WidgetDetectorThread(
    ola::Callback2<void, UsbWidget*, const WidgetInformation*> *callback)
    : m_detector(NULL),
      m_callback(callback) {
  if (!m_callback)
    OLA_FATAL << "No callback registered for the WidgetDetectorThread";
}


/**
 * Clean up
 */
WidgetDetectorThread::~WidgetDetectorThread() {
  if (m_callback)
    delete m_callback;
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
  m_detector = new UsbProWidgetDetector(
    &m_ss,
    ola::NewCallback(this, &WidgetDetectorThread::WidgetReady),
    ola::NewCallback(this, &WidgetDetectorThread::InternalFreeWidget));
  RunScan();
  m_ss.RegisterRepeatingTimeout(
      SCAN_INTERVAL_MS,
      ola::NewCallback(this, &WidgetDetectorThread::RunScan));
  m_ss.Run();
  // this will cal InternalFreeWidget for any remaining widgets
  delete m_detector;
  m_detector = NULL;
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
void WidgetDetectorThread::FreeWidget(UsbWidget *widget) {
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
    ola::network::ConnectedDescriptor *descriptor = UsbWidget::OpenDevice(*it);
    if (!descriptor)
      continue;

    UsbWidget *widget = new UsbWidget(descriptor);

    m_active_widgets[widget] = PathDescriptorPair(*it, descriptor);
    m_active_paths.insert(*it);
    m_ss.AddReadDescriptor(descriptor);
    m_detector->Discover(widget);
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
void WidgetDetectorThread::WidgetReady(UsbWidget *widget,
                                       const WidgetInformation *info) {
  ola::network::ConnectedDescriptor *descriptor = NULL;

  ActiveWidgets::iterator iter = m_active_widgets.find(widget);
  if (iter == m_active_widgets.end()) {
    OLA_WARN << "Missing widget " << widget << " in active map";
  } else {
    // remove descriptor from our ss
    descriptor = iter->second.second;
    m_ss.RemoveReadDescriptor(descriptor);
  }

  if (m_callback) {
    // default the remove handler to us.
    widget->SetOnRemove(NewSingleCallback(this,
                        &WidgetDetectorThread::FreeWidget,
                        widget));
    m_callback->Run(widget, info);
  } else {
    OLA_WARN << "No callback defined for new Usb widgets.";
    delete widget;
    delete info;
    if (descriptor)
      delete descriptor;
  }
}


/**
 * Free the widget and the associated descriptor.
 */
void WidgetDetectorThread::InternalFreeWidget(UsbWidget *widget) {
  ola::network::ConnectedDescriptor *descriptor = NULL;

  ActiveWidgets::iterator iter = m_active_widgets.find(widget);
  if (iter == m_active_widgets.end()) {
    OLA_WARN << "Missing widget " << widget << " in active map";
  } else {
    // remove descriptor from our ss if it's there
    descriptor = iter->second.second;
    m_ss.RemoveReadDescriptor(descriptor);

    // remove from active paths
    m_active_paths.erase(iter->second.first);
    m_active_widgets.erase(iter);
  }
  delete widget;
  if (descriptor)
    delete descriptor;
}
}  // usbpro
}  // plugin
}  // ola
