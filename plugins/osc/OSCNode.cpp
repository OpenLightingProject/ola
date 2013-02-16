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
 * OSCNode.cpp
 * A self contained object for sending and receiving OSC messages.
 * Copyright (C) 2012 Simon Newton
 */

#include <ola/BaseTypes.h>
#include <ola/Callback.h>
#include <ola/ExportMap.h>
#include <ola/Logging.h>
#include <ola/StringUtils.h>
#include <ola/stl/STLUtils.h>
#include <algorithm>
#include <string>
#include <utility>
#include <vector>
#include "plugins/osc/OSCNode.h"

namespace ola {
namespace plugin {
namespace osc {

using ola::IntToString;
using std::pair;

const char OSCNode::OSC_PORT_VARIABLE[] = "osc-listen-port";

/*
 * The Error handler for the OSC server.
 */
void OSCErrorHandler(int error_code, const char *msg, const char *stack) {
  OLA_WARN << "OSC Error. Code " << error_code << ", " << msg << ", " << stack;
}

/**
 * Called when liblo receives data.
 * @param osc_address the OSC Address this data was sent to
 * @param types the OSC data type for the data
 * @param argv the data itself
 * @param argc the number of data blocks
 * @param user_data a pointer to the OSCNode object.
 */
int OSCDataHandler(const char *osc_address, const char *types, lo_arg **argv,
                   int argc, void *, void *user_data) {
  OLA_DEBUG << "Got OSC message for " << osc_address << ", types are " << types;

  OSCNode *node = reinterpret_cast<OSCNode*>(user_data);
  if (string(types) != "b") {
    OLA_WARN << "Got invalid OSC message to " << osc_address << ", types was "
             << types;
    return 0;
  }

  if (argc != 1) {
    OLA_WARN << "Got invalid OSC message to " << osc_address << ", argc was "
             << argc;
    return 0;
  }

  lo_blob blob = argv[0];
  unsigned int size = std::min(static_cast<uint32_t>(DMX_UNIVERSE_SIZE),
                               lo_blob_datasize(blob));
  DmxBuffer data_buffer(static_cast<uint8_t*>(lo_blob_dataptr(blob)), size);
  node->HandleDMXData(osc_address, data_buffer);
  return 0;
}


/**
 * Create a new OSCNode.
 * @param ss the SelectServer to use
 * @param export_map a pointer to an ExportMap (may be NULL)
 * @para options the OSCNodeOptions
 */
OSCNode::OSCNode(SelectServerInterface *ss,
                 ExportMap *export_map,
                 const OSCNodeOptions &options)
    : m_ss(ss),
      m_export_map(export_map),
      m_listen_port(options.listen_port),
      m_descriptor(NULL),
      m_osc_server(NULL) {
  if (export_map) {
    // export the OSC listening port if we have an export map
    ola::IntegerVariable *osc_port_var =
      export_map->GetIntegerVar(OSC_PORT_VARIABLE);
    osc_port_var->Set(options.listen_port);
  }
}


/**
 * Cleanup
 */
OSCNode::~OSCNode() {
  Stop();
}


/*
 * Setup the Node
 */
bool OSCNode::Init() {
  // create a new lo_server
  m_osc_server = lo_server_new_with_proto(IntToString(m_listen_port).c_str(),
                                          LO_UDP, OSCErrorHandler);
  if (!m_osc_server)
    return false;

  // Get the socket descriptor that liblo is using, create a
  // UnmanagedFileDescriptor, assign a callback and register with the
  // SelectServer.
  int fd = lo_server_get_socket_fd(m_osc_server);
  m_descriptor = new ola::io::UnmanagedFileDescriptor(fd);
  m_descriptor->SetOnData(NewCallback(this, &OSCNode::DescriptorReady));
  m_ss->AddReadDescriptor(m_descriptor);
  return true;
}


/**
 * Stop this node. This removes all registrations and targets.
 */
void OSCNode::Stop() {
  // Clean up the m_target_by_group map
  OSCTargetMap::iterator group_iter = m_target_by_group.begin();
  for (; group_iter != m_target_by_group.end(); ++group_iter) {
    OSCTargetVector *targets = group_iter->second;

    // For each target, free the lo_address and delete the NodeOSCTarget struct
    OSCTargetVector::iterator target_iter = targets->begin();
    for (; target_iter != targets->end(); ++target_iter) {
      lo_address_free((*target_iter)->liblo_address);
      delete *target_iter;
    }
    // delete the vector itself
    delete targets;
  }
  m_target_by_group.clear();

  // Delete all the RX callbacks.
  STLDeleteValues(&m_address_callbacks);

  if (m_descriptor) {
    // if there was an UnmanagedFileDescriptor, de-register from the
    // SelectServer and delete it.
    m_ss->RemoveReadDescriptor(m_descriptor);
    delete m_descriptor;
    m_descriptor = NULL;
  }
  if (m_osc_server) {
    // If there was an lo_server then free it.
    lo_server_free(m_osc_server);
    m_osc_server = NULL;
  }
}


/**
 * Add a target for a particular group.
 * @param group the group to add this target to
 * @param target the OSC address for the target
 */
void OSCNode::AddTarget(unsigned int group, const OSCTarget &target) {
  OSCTargetVector *targets = NULL;
  // Find the OSCTargetVector that belongs to this group, if there isn't one we
  // create a new one
  OSCTargetMap::iterator group_iter = m_target_by_group.find(group);
  if (group_iter == m_target_by_group.end()) {
    // not found, create a new one
    targets = new OSCTargetVector;
    m_target_by_group.insert(
        pair<unsigned int, OSCTargetVector*>(group, targets));
  } else {
    targets = group_iter->second;
  }

  // Check if this target already exists in the group. If it does log a warning
  // and return
  OSCTargetVector::iterator target_iter = targets->begin();
  for (; target_iter != targets->end(); ++target_iter) {
    if ((*target_iter)->socket_address == target.socket_address &&
        (*target_iter)->osc_address == target.osc_address) {
      OLA_WARN << "Attempted to add " << target.socket_address
               << target.osc_address << " twice";
      return;
    }
  }

  // The target wasn't found, add it
  NodeOSCTarget *node_target = new NodeOSCTarget(target);
  // Create a new lo_address
  node_target->liblo_address = lo_address_new(
      target.socket_address.Host().ToString().c_str(),
      IntToString(target.socket_address.Port()).c_str());
  // Add to the list of targets
  targets->push_back(node_target);
}


/**
 * De-Register a target from this group.
 * @param group the group to remove the target from
 * @param target the OSCTarget to remove
 * @returns true if the target was removed, false if it wasn't in the group
 */
bool OSCNode::RemoveTarget(unsigned int group, const OSCTarget &target) {
  OSCTargetMap::iterator group_iter = m_target_by_group.find(group);
  if (group_iter == m_target_by_group.end())
    // the group doesn't exist, just return false
    return false;
  OSCTargetVector *targets = group_iter->second;

  OSCTargetVector::iterator target_iter = targets->begin();
  for (; target_iter != targets->end(); ++target_iter) {
    if ((*target_iter)->socket_address == target.socket_address &&
        (*target_iter)->osc_address == target.osc_address) {
      // the target was found, free the lo_address, delete the NodeOSCTarget
      // struct and remove the entry from the vector.
      lo_address_free((*target_iter)->liblo_address);
      delete *target_iter;
      targets->erase(target_iter);
      return true;
    }
  }
  return false;
}


/**
 * Send the DMX data to all targets registered for this group.
 * @param group the group to send the data to
 * @param dmx_data the DmxBuffer to send
 * @returns true if sucesfully sent, false if any error occured.
 */
bool OSCNode::SendData(unsigned int group, const ola::DmxBuffer &dmx_data) {
  OSCTargetMap::iterator group_iter = m_target_by_group.find(group);
  if (group_iter == m_target_by_group.end())
    // group doesn't exist, just return
    return true;

  OSCTargetVector *targets = group_iter->second;
  // create the new OSC blob
  lo_blob osc_data = lo_blob_new(dmx_data.Size(), dmx_data.GetRaw());

  bool ok = true;
  // iterate over all the targets, and send to each one.
  OSCTargetVector::iterator target_iter = targets->begin();
  for (; target_iter != targets->end(); ++target_iter) {
    OLA_DEBUG << "Sending to " << (*target_iter)->socket_address;
    int ret = lo_send_from((*target_iter)->liblo_address,
                           m_osc_server,
                           LO_TT_IMMEDIATE,
                           (*target_iter)->osc_address.c_str(),
                           "b", osc_data);
    ok &= (ret > 0);
  }
  // free the blob
  lo_blob_free(osc_data);
  return ok;
}


/**
 * Register a callback to be run when we receive data for an address.
 * De-registration can be performed by passing NULL as a callback. Attempting
 * to register more than once on the same address will return false.
 * @param address the OSC address to register.
 * @param callback the callback to run, ownership is transferred. The callback
 *   can be set to NULL to de-register.
 * @returns false if callback was non-NULL, but the address was already
 *   registered. true otherwise.
 */
bool OSCNode::RegisterAddress(const string &osc_address,
                              DMXCallback *callback) {
  AddressCallbackMap::iterator iter = m_address_callbacks.find(osc_address);
  if (iter == m_address_callbacks.end()) {
    // The osc_address wasn't registered, if this is a de-registration just
    // return
    if (callback == NULL)
      return true;

    // This is a new registration, insert into the AddressCallbackMap and
    // register with liblo.
    m_address_callbacks.insert(
        pair<string, DMXCallback*>(osc_address, callback));
    lo_server_add_method(m_osc_server, osc_address.c_str(), "b", OSCDataHandler,
                         this);
  } else {
    // The osc_address is already registered.
    if (callback == NULL) {
      // De-registration request
      lo_server_del_method(m_osc_server, osc_address.c_str(), "b");
      delete iter->second;
      m_address_callbacks.erase(iter);
    } else {
      // This was an attempt to re-register. This is an error.
      OLA_WARN << "Attempt to register a second callback for " << osc_address;
      // delete the callback, since we have ownership of it
      delete callback;
      return false;
    }
  }
  return true;
}


/**
 * Called by OSCDataHandler when there is new data.
 * @param osc_address the OSC address this data arrived on
 * @param data, the DmxBuffer containing the data.
 */
void OSCNode::HandleDMXData(const string &osc_address, const DmxBuffer &data) {
  // Lookup the callback associated with this address and if it exists, run it.
  AddressCallbackMap::iterator iter = m_address_callbacks.find(osc_address);
  if (iter != m_address_callbacks.end())
    iter->second->Run(data);
}


/**
 * Called when the OSC FD is readable.
 */
void OSCNode::DescriptorReady() {
  // Call into liblo with a timeout of 0 so we don't block.
  lo_server_recv_noblock(m_osc_server, 0);
}
}  // osc
}  // plugin
}  // ola
