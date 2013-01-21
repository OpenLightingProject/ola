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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
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
#include "ola/io/Descriptor.h"
#include "ola/io/SelectServer.h"
#include "ola/thread/Thread.h"
#include "plugins/usbpro/BaseUsbProWidget.h"
#include "plugins/usbpro/RobeWidget.h"
#include "plugins/usbpro/RobeWidgetDetector.h"
#include "plugins/usbpro/UsbProWidgetDetector.h"
#include "plugins/usbpro/SerialWidgetInterface.h"
#include "plugins/usbpro/WidgetDetectorInterface.h"

namespace ola {
namespace plugin {
namespace usbpro {

using std::set;
using std::string;
using std::vector;
using ola::io::ConnectedDescriptor;
using ola::thread::ConditionVariable;
using ola::thread::Mutex;


/**
 * The interface to implement to catch new widgets from the
 * WidgetDetectorThread.
 *
 * We overload the NewWidget method based on the type of widget
 * discovered.
 */
class NewWidgetHandler {
  public:
    virtual ~NewWidgetHandler() {}

    virtual void NewWidget(class ArduinoWidget *widget,
                           const UsbProWidgetInformation &information) = 0;
    virtual void NewWidget(class EnttecUsbProWidget *widget,
                           const UsbProWidgetInformation &information) = 0;
    virtual void NewWidget(class DmxTriWidget *widget,
                           const UsbProWidgetInformation &information) = 0;
    virtual void NewWidget(class DmxterWidget *widget,
                           const UsbProWidgetInformation &information) = 0;
    virtual void NewWidget(class RobeWidget *widget,
                           const RobeWidgetInformation &information) = 0;
    virtual void NewWidget(class UltraDMXProWidget *widget,
                           const UsbProWidgetInformation &information) = 0;
};


/*
 * Discovers new USB Serial widgets and calls the handler.
 */
class WidgetDetectorThread: public ola::thread::Thread {
  public:
    explicit WidgetDetectorThread(NewWidgetHandler *widget_handler,
                                  ola::io::SelectServerInterface *ss,
                                  unsigned int usb_pro_timeout = 200,
                                  unsigned int robe_timeout = 200);
    ~WidgetDetectorThread() {}

    // Must be called before Run()
    void SetDeviceDirectory(const string &directory);
    // Must be called before Run()
    void SetDevicePrefixes(const vector<string> &prefixes);
    // Must be called before Run()
    void SetIgnoredDevices(const vector<string> &devices);

    // Start the thread, this will call the SuccessHandler whenever a new
    // Widget is located.
    void *Run();

    // Stop the thread.
    bool Join(void *ptr);

    // Can be called from any thread.
    void FreeWidget(SerialWidgetInterface *widget);

    // blocks until the thread is running
    void WaitUntilRunning();

  protected:
    virtual bool RunScan();
    void PerformDiscovery(const string &path,
                          ConnectedDescriptor *descriptor);

  private:
    ola::io::SelectServerInterface *m_other_ss;
    ola::io::SelectServer m_ss;  // ss for this thread
    vector<WidgetDetectorInterface*> m_widget_detectors;
    string m_directory;  // directory to look for widgets in
    vector<string> m_prefixes;  // prefixes to try
    set<string> m_ignored_devices;  // devices to ignore
    NewWidgetHandler *m_handler;
    bool m_is_running;
    unsigned int m_usb_pro_timeout;
    unsigned int m_robe_timeout;
    Mutex m_mutex;
    ConditionVariable m_condition;

    // those paths that are either in discovery, or in use
    set<string> m_active_paths;
    // holds the path and current widget detector offset
    typedef std::pair<string, int> DescriptorInfo;
    // map of descriptor to DescriptorInfo
    typedef map<ConnectedDescriptor*, DescriptorInfo>
      ActiveDescriptors;
    // the descriptors that are in the discovery process
    ActiveDescriptors m_active_descriptors;

    void FindCandiateDevices(vector<string> *device_paths);

    // called when we find new widgets of a particular type
    void UsbProWidgetReady(ConnectedDescriptor *descriptor,
                           const UsbProWidgetInformation *info);
    void RobeWidgetReady(ConnectedDescriptor *descriptor,
                         const RobeWidgetInformation *info);

    void DescriptorFailed(ConnectedDescriptor *descriptor);
    void PerformNextDiscoveryStep(ConnectedDescriptor *descriptor);
    void InternalFreeWidget(SerialWidgetInterface *widget);
    void FreeDescriptor(ConnectedDescriptor *descriptor);

    template<typename WidgetType, typename InfoType>
    void DispatchWidget(WidgetType *widget, const InfoType *information);

    // All of these are called in a separate thread.
    template<typename WidgetType, typename InfoType>
    void SignalNewWidget(WidgetType *widget, const InfoType *information);

    void MarkAsRunning();

    static const unsigned int SCAN_INTERVAL_MS = 20000;

    // This is how device identification is done, see
    // http://opendmx.net/index.php/USB_Protocol_Extensions
    // OPEN_LIGHTING_ESTA_CODE is in BaseTypes.h
    static const uint16_t DMX_KING_DMX512_ID = 0;
    static const uint16_t DMX_KING_ULTRA_ID = 1;
    static const uint16_t DMX_KING_ULTRA_PRO_ID = 2;
    static const uint16_t DMX_KING_ULTRA_MICRO_ID = 3;
    static const uint16_t DMX_KING_ULTRA_RDM_ID = 4;
    static const uint16_t DMX_KING_ESTA_ID = 0x6a6b;
    static const uint16_t GODDARD_DMXTER4_ID = 0x444d;
    static const uint16_t GODDARD_ESTA_ID = 0x4744;
    static const uint16_t GODDARD_MINI_DMXTER4_ID = 0x4d49;
    static const uint16_t JESE_DMX_TRI_ID = 1;
    static const uint16_t JESE_ESTA_ID = 0x6864;
    static const uint16_t JESE_RDM_TRI_ID = 2;
    static const uint16_t OPEN_LIGHTING_PACKETHEADS_ID = 2;
    static const uint16_t OPEN_LIGHTING_RGB_MIXER_ID = 1;
};
}  // usbpro
}  // plugin
}  // ola
#endif  // PLUGINS_USBPRO_WIDGETDETECTORTHREAD_H_
