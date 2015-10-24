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
 * SyncronizedWidgetObserver.cpp
 * Transfers widget add/remove events to another thread.
 * Copyright (C) 2014 Simon Newton
 */

#include "plugins/usbdmx/SyncronizedWidgetObserver.h"

#include "ola/Callback.h"

namespace ola {
namespace plugin {
namespace usbdmx {

using ola::thread::Thread;

SyncronizedWidgetObserver::SyncronizedWidgetObserver(
    WidgetObserver *observer,
    ola::io::SelectServerInterface *ss)
    : m_observer(observer),
      m_ss(ss),
      m_main_thread_id(Thread::Self()) {
}

template<typename WidgetClass>
bool SyncronizedWidgetObserver::DispatchNewWidget(WidgetClass *widget) {
  if (pthread_equal(Thread::Self(), m_main_thread_id)) {
    return m_observer->NewWidget(widget);
  } else {
    AddFuture f;
    m_ss->Execute(
        NewSingleCallback(
            this, &SyncronizedWidgetObserver::HandleNewWidget<WidgetClass>,
            widget, &f));
    return f.Get();
  }
}

template<typename WidgetClass>
void SyncronizedWidgetObserver::HandleNewWidget(WidgetClass*widget,
                                                AddFuture *f) {
  f->Set(m_observer->NewWidget(widget));
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
