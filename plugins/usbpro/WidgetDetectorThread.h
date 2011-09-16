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
 * A thread that periodically looks for usb serial devices, and runs the
 * callbacks if they are valid widgets.
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
#include "plugins/usbpro/BaseUsbProWidget.h"
#include "plugins/usbpro/RobeWidget.h"
#include "plugins/usbpro/RobeWidgetDetector.h"
#include "plugins/usbpro/UsbProWidgetDetector.h"
#include "plugins/usbpro/UsbWidgetInterface.h"
#include "plugins/usbpro/WidgetDetectorInterface.h"

namespace ola {
namespace plugin {
namespace usbpro {

using std::set;
using std::string;
using std::vector;
using ola::network::ConnectedDescriptor;

/*
 * Discovers new Usb Pro like widgets and runs the callback.
 */
class WidgetDetectorThread: public ola::OlaThread {
  public:
    typedef ola::Callback2<void,
                           BaseUsbProWidget*,
                           const UsbProWidgetInformation*> UsbProCallback;
    typedef ola::Callback2<void,
                           RobeWidget*,
                           const RobeWidgetInformation*> RobeCallback;
    WidgetDetectorThread(
        UsbProCallback *usb_pro_callback,
        RobeCallback *robe_callback);
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
    void FreeWidget(UsbWidgetInterface *widget);

  private:
    ola::network::SelectServer m_ss;  // ss for this thread
    vector<WidgetDetectorInterface*> m_widget_detectors;
    string m_directory;  // directory to look for widgets in
    vector<string> m_prefixes;  // prefixes to try

    UsbProCallback *m_usb_pro_callback;
    RobeCallback *m_robe_callback;

    // those paths that are either in discovery, or in use
    set<string> m_active_paths;
    // holds the path and current widget detector offset
    typedef std::pair<string, int> DescriptorInfo;
    // map of descriptor to DescriptorInfo
    typedef map<ConnectedDescriptor*, DescriptorInfo>
      ActiveDescriptors;
    // the descriptors that are in the discovery process
    ActiveDescriptors m_active_descriptors;

    bool RunScan();
    void FindCandiateDevices(vector<string> *device_paths);

    // called when we find new widgets of a particular type
    void UsbProWidgetReady(BaseUsbProWidget *widget,
                           const UsbProWidgetInformation *info);
    void RobeWidgetReady(RobeWidget *widget,
                         const RobeWidgetInformation *info);

    void DescriptorFailed(ConnectedDescriptor *descriptor);
    void PerformNextDiscoveryStep(ConnectedDescriptor *descriptor);
    void InternalFreeWidget(UsbWidgetInterface *widget);
    void FreeDescriptor(ConnectedDescriptor *descriptor);

    static const unsigned int SCAN_INTERVAL_MS = 20000;
};
}  // usbpro
}  // plugin
}  // ola
#endif  // PLUGINS_USBPRO_WIDGETDETECTORTHREAD_H_
