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
#include <utility>
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
#include "plugins/usbpro/UsbProExtWidget.h"

namespace ola {
namespace plugin {
namespace usbpro {

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
    virtual void NewWidget(class UsbProExtWidget *widget,
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
    void SetDeviceDirectory(const std::string &directory);
    // Must be called before Run()
    void SetDevicePrefixes(const std::vector<std::string> &prefixes);
    // Must be called before Run()
    void SetIgnoredDevices(const std::vector<std::string> &devices);

    // Start the thread, this will call the SuccessHandler whenever a new
    // Widget is located.
    void *Run();

    // Stop the thread.
    bool Join(void *ptr);

    // Used to release a widget. Should be called from the thread running the
    // SelectServerInterface that was passed to the constructor.
    void FreeWidget(SerialWidgetInterface *widget);

    // blocks until the thread is running
    void WaitUntilRunning();

 protected:
    virtual bool RunScan();
    void PerformDiscovery(const std::string &path,
                          ola::io::ConnectedDescriptor *descriptor);

 private:
    ola::io::SelectServerInterface *m_other_ss;
    ola::io::SelectServer m_ss;  // ss for this thread
    std::vector<WidgetDetectorInterface*> m_widget_detectors;
    std::string m_directory;  // directory to look for widgets in
    std::vector<std::string> m_prefixes;  // prefixes to try
    std::set<std::string> m_ignored_devices;  // devices to ignore
    NewWidgetHandler *m_handler;
    bool m_is_running;
    unsigned int m_usb_pro_timeout;
    unsigned int m_robe_timeout;
    ola::thread::Mutex m_mutex;
    ola::thread::ConditionVariable m_condition;

    // those paths that are either in discovery, or in use
    std::set<std::string> m_active_paths;
    // holds the path and current widget detector offset
    typedef std::pair<std::string, int> DescriptorInfo;
    // map of descriptor to DescriptorInfo
    typedef std::map<ola::io::ConnectedDescriptor*, DescriptorInfo>
      ActiveDescriptors;
    // the descriptors that are in the discovery process
    ActiveDescriptors m_active_descriptors;

    // called when we find new widgets of a particular type
    void UsbProWidgetReady(ola::io::ConnectedDescriptor *descriptor,
                           const UsbProWidgetInformation *info);
    void RobeWidgetReady(ola::io::ConnectedDescriptor *descriptor,
                         const RobeWidgetInformation *info);

    void DescriptorFailed(ola::io::ConnectedDescriptor *descriptor);
    void PerformNextDiscoveryStep(ola::io::ConnectedDescriptor *descriptor);
    void InternalFreeWidget(SerialWidgetInterface *widget);
    void FreeDescriptor(ola::io::ConnectedDescriptor *descriptor);

    template<typename WidgetType, typename InfoType>
    void DispatchWidget(WidgetType *widget, const InfoType *information);

    // All of these are called in a separate thread.
    template<typename WidgetType, typename InfoType>
    void SignalNewWidget(WidgetType *widget, const InfoType *information);

    void MarkAsRunning();

    static const unsigned int SCAN_INTERVAL_MS = 20000;

    // This is how device identification is done, see
    // https://wiki.openlighting.org/index.php/USB_Protocol_Extensions
    // OPEN_LIGHTING_ESTA_CODE is in Constants.h

    // DmxKing Device Models
    static const uint16_t DMX_KING_DMX512_ID = 0;
    static const uint16_t DMX_KING_ULTRA_ID = 1;
    static const uint16_t DMX_KING_ULTRA_PRO_ID = 2;
    static const uint16_t DMX_KING_ULTRA_MICRO_ID = 3;
    static const uint16_t DMX_KING_ULTRA_RDM_ID = 4;

    // Jese device models.
    static const uint16_t JESE_DMX_TRI_MK1_ID = 1;  // Original DMX-TRI
    static const uint16_t JESE_RDM_TRI_MK1_ID = 2;  // Original RDM-TRI
    static const uint16_t JESE_RDM_TRI_MK2_ID = 3;
    static const uint16_t JESE_RDM_TXI_MK2_ID = 4;
    // DMX-TRI, with new hardware
    static const uint16_t JESE_DMX_TRI_MK1_SE_ID = 5;

    // Goddard device models
    static const uint16_t GODDARD_DMXTER4_ID = 0x444d;
    static const uint16_t GODDARD_MINI_DMXTER4_ID = 0x4d49;
    static const uint16_t GODDARD_DMXTER4A_ID = 0x3441;

    // Open Lighting device models
    static const uint16_t OPEN_LIGHTING_PACKETHEADS_ID = 2;
    static const uint16_t OPEN_LIGHTING_RGB_MIXER_ID = 1;

    // ESTA Ids
    static const uint16_t DMX_KING_ESTA_ID = 0x6a6b;
    static const uint16_t GODDARD_ESTA_ID = 0x4744;
    static const uint16_t JESE_ESTA_ID = 0x6864;
    static const uint16_t ESTA_ID_EXPERIMENTAL = 0x7FF7;
};
}  // namespace usbpro
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBPRO_WIDGETDETECTORTHREAD_H_
