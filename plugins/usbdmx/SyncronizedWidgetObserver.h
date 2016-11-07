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
 * SyncronizedWidgetObserver.h
 * Transfers widget add/remove events to another thread.
 * Copyright (C) 2014 Simon Newton
 */

#ifndef PLUGINS_USBDMX_SYNCRONIZEDWIDGETOBSERVER_H_
#define PLUGINS_USBDMX_SYNCRONIZEDWIDGETOBSERVER_H_

#include "plugins/usbdmx/WidgetFactory.h"

#include "ola/io/SelectServerInterface.h"
#include "ola/thread/Future.h"
#include "ola/thread/Thread.h"

namespace ola {
namespace plugin {
namespace usbdmx {

/**
 * @brief Transfers widget add/remove events to another thread.
 *
 * The SyncronizedWidgetObserver ensures that all widget add/removed events are
 * handled in the thread that created the SyncronizedWidgetObserver object.
 */
class SyncronizedWidgetObserver : public WidgetObserver {
 public:
  /**
   * @brief Create a new SyncronizedWidgetObserver.
   * @param observer the observer to notify on add/remove events.
   * @param ss The ss to use the schedule events on.
   */
  SyncronizedWidgetObserver(WidgetObserver *observer,
                            ola::io::SelectServerInterface *ss);

  bool NewWidget(class AnymauDMX *widget) {
    return DispatchNewWidget(widget);
  }

  bool NewWidget(class AVLdiyD512 *widget) {
    return DispatchNewWidget(widget);
  }

  bool NewWidget(class DMXCProjectsNodleU1 *widget) {
    return DispatchNewWidget(widget);
  }

  bool NewWidget(class EurolitePro *widget) {
    return DispatchNewWidget(widget);
  }

  bool NewWidget(class ShowJockey *widget) {
    return DispatchNewWidget(widget);
  }

  bool NewWidget(ola::usb::JaRuleWidget *widget) {
    return DispatchNewWidget(widget);
  }

  bool NewWidget(class ScanlimeFadecandy *widget) {
    return DispatchNewWidget(widget);
  }

  bool NewWidget(class Sunlite *widget) {
    return DispatchNewWidget(widget);
  }

  bool NewWidget(class VellemanK8062 *widget) {
    return DispatchNewWidget(widget);
  }

 private:
  typedef ola::thread::Future<bool> AddFuture;

  WidgetObserver* const m_observer;
  ola::io::SelectServerInterface* const m_ss;
  const ola::thread::ThreadId m_main_thread_id;

  template<typename WidgetClass>
  bool DispatchNewWidget(WidgetClass *widget);

  template<typename WidgetClass>
  void HandleNewWidget(WidgetClass *widget, AddFuture *f);

  DISALLOW_COPY_AND_ASSIGN(SyncronizedWidgetObserver);
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_SYNCRONIZEDWIDGETOBSERVER_H_
