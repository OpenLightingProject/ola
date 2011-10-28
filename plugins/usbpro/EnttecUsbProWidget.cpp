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
 * EnttecUsbProWidget.h
 * The Enttec USB Pro Widget
 * TODO(simon): implement RDM for this device - bug #146.
 * It doesn't do discovery onboard through which makes thing pretty difficult.
 * Copyright (C) 2010 Simon Newton
 */

#include <string.h>
#include "ola/BaseTypes.h"
#include "ola/Logging.h"
#include "ola/rdm/RDMCommand.h"
#include "ola/rdm/RDMEnums.h"
#include "ola/rdm/UID.h"
#include "ola/rdm/UIDSet.h"
#include "plugins/usbpro/BaseUsbProWidget.h"
#include "plugins/usbpro/EnttecUsbProWidget.h"

namespace ola {
namespace plugin {
namespace usbpro {

using ola::rdm::RDMCommand;
using ola::rdm::RDMRequest;
using ola::rdm::UID;
using ola::rdm::UIDSet;


/*
 * New Enttec Usb Pro Device.
 * This also works for the RDM Pro with the standard firmware loaded.
 */
EnttecUsbProWidgetImpl::EnttecUsbProWidgetImpl(
  ola::thread::SchedulerInterface *scheduler,
  ola::network::ConnectedDescriptor *descriptor)
    : GenericUsbProWidget(scheduler, descriptor) {
}


/**
 * EnttecUsbProWidget Constructor
 */
EnttecUsbProWidget::EnttecUsbProWidget(
    ola::thread::SchedulerInterface *scheduler,
    ola::network::ConnectedDescriptor *descriptor,
    unsigned int queue_size)
    : m_impl(scheduler, descriptor) {
    // m_controller(&m_impl, queue_size) {
  /*
  m_impl.SetDiscoveryCallback(
      NewCallback(this, &UsbProWidget::ResumeRDMCommands));
  */
  (void) queue_size;
}
}  // usbpro
}  // plugin
}  // ola
