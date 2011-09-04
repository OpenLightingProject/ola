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
 * WidgetDetectorThread.h
 * A thread that periodically looks for usb pro devices, and runs the callback
 * if they are valid widgets.
 * Copyright (C) 2011 Simon Newton
 */

#ifndef PLUGINS_USBPRO_WIDGETDETECTORTHREAD_H_
#define PLUGINS_USBPRO_WIDGETDETECTORTHREAD_H_

#include <map>
#include <set>
#include <string>
#include <vector>
#include "ola/Callback.h"
#include "ola/OlaThread.h"
#include "ola/network/SelectServer.h"
#include "plugins/usbpro/WidgetDetector.h"

namespace ola {
namespace plugin {
namespace usbpro {

using std::set;
using std::string;
using std::vector;

/*
 * Discovers new Usb Pro like widgets and runs the callback.
 */
class WidgetDetectorThread: public ola::OlaThread {
  public:
    WidgetDetectorThread(
        ola::Callback2<void, UsbWidget*, const WidgetInformation*> *callback);
    ~WidgetDetectorThread();

    // Must be called before Run()
    void SetDeviceDirectory(const string &directory);
    // Must be called before Run()
    void SetDevicePrefixes(const vector<string> &prefixes);

    // Start the thread, this will call the SuccessHandler whenever a new
    // Widget is located.
    void *Run();

    // Stop the thread.
    bool Join(void *ptr);

    // Can be called from any thread.
    void FreeWidget(UsbWidget *widget);

  private:
    ola::network::SelectServer m_ss;
    WidgetDetector m_detector;
    string m_directory;
    vector<string> m_prefixes;
    ola::Callback2<void, UsbWidget*, const WidgetInformation*> *m_callback;
    // those paths that are either in discovery, or in use
    set<string> m_active_paths;
    // a map of UsbWidget to (path, descriptor)
    typedef std::pair<string, ola::network::ConnectedDescriptor*>
      PathDescriptorPair;
    typedef map<UsbWidget*, PathDescriptorPair> ActiveWidgets;
    ActiveWidgets m_active_widgets;

    bool RunScan();
    void FindCandiateDevices(vector<string> *device_paths);
    void WidgetReady(UsbWidget *widget, const WidgetInformation *info);
    void InternalFreeWidget(UsbWidget *widget);

    static const unsigned int SCAN_INTERVAL_MS = 5000;
};
}  // usbpro
}  // plugin
}  // ola
#endif  // PLUGINS_USBPRO_WIDGETDETECTORTHREAD_H_
