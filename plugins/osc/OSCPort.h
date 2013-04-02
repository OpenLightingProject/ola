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
 * OSCPort.h
 * The OSCInputPort and OSCOutputPort classes.
 * Copyright (C) 2012 Simon Newton
 */

#ifndef PLUGINS_OSC_OSCPORT_H_
#define PLUGINS_OSC_OSCPORT_H_

#include <string>
#include "olad/Port.h"
#include "plugins/osc/OSCAddressTemplate.h"
#include "plugins/osc/OSCDevice.h"
#include "plugins/osc/OSCNode.h"

namespace ola {
namespace plugin {
namespace osc {

/**
 * The Input Port class, for receiving DMX via OSC. Note that the description
 * of the port may change as it's patched and unpatched from a universe (since
 * the description can contain %d). Therefore we store the description as a
 * template, as well as the current value.
 */
class OSCInputPort: public BasicInputPort {
  public:
    /**
     * Create an OSCInputPort.
     * @param device the parent device
     * @param port_id the id for this port
     * @param plugin_adaptor a PluginAdaptor object, used by the base class.
     * @param node the OSCNode object to use
     * @param address the OSC address string for this port
     */
    OSCInputPort(OSCDevice *parent,
                 unsigned int port_id,
                 PluginAdaptor *plugin_adaptor,
                 OSCNode *node,
                 const string &address)
        : BasicInputPort(parent, port_id, plugin_adaptor),
          m_node(node),
          m_address(address),
          m_actual_address(address) {
    }

    /**
     * Just return our DmxBuffer.
     */
    const DmxBuffer &ReadDMX() const { return m_buffer; }

    /**
     * Called during the patch process, just before the Universe of this port
     * changes.
     */
    bool PreSetUniverse(Universe *old_universe, Universe *new_universe) {
      // if the old_universe is not NULL, we need to de-register
      if (old_universe) {
        m_node->RegisterAddress(m_actual_address, NULL);
        // reset the actual address
        m_actual_address = m_address;
      }

      // if we've been supplied with a new universe, attempt to register
      if (new_universe) {
        string osc_address = ExpandTemplate(m_address,
                                            new_universe->UniverseId());
        bool ok = m_node->RegisterAddress(
            osc_address,
            NewCallback(this, &OSCInputPort::NewDMXData));

        if (!ok)
          // means that another port is registered with the same address
          return false;
        // update the address since the registration was successful.
        m_actual_address = osc_address;
      }
      return true;
    }

    /**
     * Return the actual description
     */
    string Description() const { return m_actual_address; }

  private:
    OSCNode *m_node;
    DmxBuffer m_buffer;
    const string m_address;
    string m_actual_address;

    /**
     * This is called when we receive new DMX values via OSC.
     */
    void NewDMXData(const DmxBuffer &data) {
      m_buffer = data;  // store the data
      DmxChanged();  // signal that our data has changed
    }
};


/**
 * The Output Port class, for sending DMX via OSC.
 */
class OSCOutputPort: public BasicOutputPort {
  public:
    /**
     * Create an OSCOutputPort.
     * @param device the parent device
     * @param port_id the id for this port
     * @param node the OSCNode object to use
     * @param description the string description for this port.
     */
    OSCOutputPort(OSCDevice *device,
                  unsigned int port_id,
                  OSCNode *node,
                  const string &description)
        : BasicOutputPort(device, port_id),
          m_node(node),
          m_description(description) {
    }

    /**
     * Send this DMX buffer using OSC. The second argument (priority) is not
     * used.
     * @param buffer the DMX data
     */
    bool WriteDMX(const DmxBuffer &buffer, uint8_t) {
      return m_node->SendData(this->PortId(), buffer);
    }

    /**
     * Return the description for this port
     */
    string Description() const { return m_description; }

  private:
    OSCNode *m_node;
    const string m_description;
};
}  // osc
}  // plugin
}  // ola
#endif  // PLUGINS_OSC_OSCPORT_H_
