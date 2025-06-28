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
 * OSCNode.cpp
 * A self contained object for sending and receiving OSC messages.
 * Copyright (C) 2012 Simon Newton
 */

#ifdef _WIN32
#include <ola/win/CleanWinSock2.h>
#endif  // _WIN32

#include <ola/Callback.h>
#include <ola/Constants.h>
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

#ifdef _WIN32
class UnmanagedSocketDescriptor : public ola::io::UnmanagedFileDescriptor {
 public:
  explicit UnmanagedSocketDescriptor(int fd) :
      ola::io::UnmanagedFileDescriptor(fd) {
    m_handle.m_type = ola::io::SOCKET_DESCRIPTOR;
    // Set socket to nonblocking to enable WSAEventSelect
    u_long mode = 1;
    ioctlsocket(fd, FIONBIO, &mode);
  }
 private:
  DISALLOW_COPY_AND_ASSIGN(UnmanagedSocketDescriptor);
};
#endif  // _WIN32

using ola::IntToString;
using ola::io::SelectServerInterface;
using std::make_pair;
using std::max;
using std::min;
using std::string;
using std::vector;

const char OSCNode::OSC_PORT_VARIABLE[] = "osc-listen-port";

/*
 * The Error handler for the OSC server.
 */
void OSCErrorHandler(int error_code, const char *msg, const char *stack) {
  string msg_str, stack_str;
  if (msg) {
    msg_str.assign(msg);
  }
  if (stack) {
    stack_str.assign(stack);
  }
  OLA_WARN << "OSC Error. Code " << error_code << ", " << msg_str << ", "
           << stack_str;
}


/**
 * Extract the slot number and group address from an OSC address
 */
bool ExtractSlotFromPath(const string &osc_address,
                         string *group_address,
                         uint16_t *slot) {
  size_t pos = osc_address.find_last_of("/");
  if (pos == string::npos) {
    OLA_WARN << "Got invalid OSC message to " << osc_address;
    return false;
  }
  if (!StringToInt(osc_address.substr(pos + 1), slot, true)) {
    OLA_WARN << "Unable to extract slot from " << osc_address.substr(pos + 1);
    return false;
  }

  if (*slot == 0 || *slot > DMX_UNIVERSE_SIZE) {
    OLA_WARN << "Ignoring slot " << *slot;
    return 0;
  }
  (*slot)--;

  *group_address = osc_address.substr(0, pos);
  return true;
}

/**
 * Extract the slot and value from a tuple (either ii or if)
 */
bool ExtractSlotValueFromPair(const string &type, lo_arg **argv,
                              int argc, uint16_t *slot, uint8_t *value) {
  if (argc != 2 || !(type == "ii" || type == "if")) {
    OLA_WARN << "Unknown OSC message type " << type;
    return false;
  }

  int raw_slot = argv[0]->i;
  if (raw_slot <= 0 || raw_slot > DMX_UNIVERSE_SIZE) {
    OLA_WARN << "Invalid slot # " << raw_slot;
    return false;
  }
  *slot = static_cast<uint16_t>(raw_slot - 1);

  if (type == "ii") {
    *value = min(static_cast<int>(DMX_MAX_SLOT_VALUE), max(0, argv[1]->i));
  } else if (type == "if") {
    float val = max(0.0f, min(1.0f, argv[1]->f));
    *value = val * DMX_MAX_SLOT_VALUE;
  }

  return true;
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
                   int argc, lo_message, void *user_data) {
  OLA_DEBUG << "Got OSC message for " << osc_address << ", types are " << types;

  OSCNode *node = reinterpret_cast<OSCNode*>(user_data);
  const string type(types);
  uint16_t slot;

  if (argc == 1) {
    if (type == "b") {
      lo_blob blob = (lo_blob)argv[0];
      unsigned int size = min(static_cast<uint32_t>(DMX_UNIVERSE_SIZE),
                              lo_blob_datasize(blob));
      node->SetUniverse(
          osc_address, static_cast<uint8_t*>(lo_blob_dataptr(blob)), size);
      return 0;
    } else if (type == "f") {
      float val = max(0.0f, min(1.0f, argv[0]->f));
      string group_address;
      if (!ExtractSlotFromPath(osc_address, &group_address, &slot))
        return 0;

      node->SetSlot(group_address, slot,  val * DMX_MAX_SLOT_VALUE);
      return 0;
    } else if (type == "i") {
      int val = min(static_cast<int>(DMX_MAX_SLOT_VALUE), max(0, argv[0]->i));
      string group_address;
      if (!ExtractSlotFromPath(osc_address, &group_address, &slot))
        return 0;

      node->SetSlot(group_address, slot,  val);
      return 0;
    }
  } else if (argc == 2) {
    uint8_t value;
    if (!ExtractSlotValueFromPair(type, argv, argc, &slot, &value)) {
      return 0;
    }

    node->SetSlot(osc_address, slot,  value);
    return 0;
  }
  OLA_WARN << "Unknown OSC message type " << type;
  return 0;
}


OSCNode::NodeOSCTarget::NodeOSCTarget(const OSCTarget &target)
  : socket_address(target.socket_address),
    osc_address(target.osc_address),
    liblo_address(lo_address_new(
      socket_address.Host().ToString().c_str(),
      IntToString(socket_address.Port()).c_str())) {
}

OSCNode::NodeOSCTarget::~NodeOSCTarget() {
  lo_address_free(liblo_address);
}


/**
 * Create a new OSCNode.
 * @param ss the SelectServer to use
 * @param export_map a pointer to an ExportMap (may be NULL)
 * @param options the OSCNodeOptions
 */
OSCNode::OSCNode(SelectServerInterface *ss,
                 ExportMap *export_map,
                 const OSCNodeOptions &options)
    : m_ss(ss),
      m_listen_port(options.listen_port),
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
  // lo_server_new_with_proto doesn't understand that "0" means "any port".
  // Instead you have to pass in NULL. Weird.
  if (m_listen_port) {
    m_osc_server = lo_server_new_with_proto(IntToString(m_listen_port).c_str(),
                                            LO_UDP, OSCErrorHandler);
  } else {
    m_osc_server = lo_server_new_with_proto(NULL, LO_UDP, OSCErrorHandler);
  }

  if (!m_osc_server)
    return false;

  // Get the socket descriptor that liblo is using, create a
  // UnmanagedFileDescriptor, assign a callback and register with the
  // SelectServer.
  int fd = lo_server_get_socket_fd(m_osc_server);
#ifdef _WIN32
  m_descriptor.reset(new UnmanagedSocketDescriptor(fd));
#else
  m_descriptor.reset(new ola::io::UnmanagedFileDescriptor(fd));
#endif  // _WIN32
  m_descriptor->SetOnData(NewCallback(this, &OSCNode::DescriptorReady));
  m_ss->AddReadDescriptor(m_descriptor.get());

  // liblo doesn't support address pattern matching. So rather than registering
  // a bunch of handlers, we just register for any address matching the types
  // we want, and handle the dispatching ourselves. NULL means 'any address'
  // Similarly liblo tries to coerce types so rather than letting it do
  // anything we just register for all types and sort it out ourselves.
  lo_server_add_method(m_osc_server, NULL, NULL, OSCDataHandler, this);
  return true;
}


/**
 * Stop this node. This removes all registrations and targets.
 */
void OSCNode::Stop() {
  if (m_osc_server) {
    lo_server_del_method(m_osc_server, NULL, NULL);
  }

  // Clean up the m_output_map map
  OutputGroupMap::iterator group_iter = m_output_map.begin();
  for (; group_iter != m_output_map.end(); ++group_iter) {
    STLDeleteElements(&group_iter->second->targets);
    delete group_iter->second;
  }
  m_output_map.clear();

  // Delete all the RX callbacks.
  STLDeleteValues(&m_input_map);

  if (m_descriptor.get()) {
    // if there was an UnmanagedFileDescriptor, de-register from the
    // SelectServer and delete it.
    m_ss->RemoveReadDescriptor(m_descriptor.get());
    m_descriptor.reset();
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
  OSCOutputGroup *output_group = STLFindOrNull(m_output_map, group);

  if (!output_group) {
    // not found, create a new one
    output_group = new OSCOutputGroup();
    STLReplaceAndDelete(&m_output_map, group, output_group);
  }

  OSCTargetVector &targets = output_group->targets;

  // Check if this target already exists in the group. If it does log a warning
  // and return
  OSCTargetVector::iterator target_iter = targets.begin();
  for (; target_iter != targets.end(); ++target_iter) {
    if (**target_iter == target) {
      OLA_WARN << "Attempted to add " << target.socket_address
               << target.osc_address << " twice";
      return;
    }
  }

  // Add to the list of targets
  output_group->targets.push_back(new NodeOSCTarget(target));
}


/**
 * De-Register a target from this group.
 * @param group the group to remove the target from
 * @param target the OSCTarget to remove
 * @returns true if the target was removed, false if it wasn't in the group
 */
bool OSCNode::RemoveTarget(unsigned int group, const OSCTarget &target) {
  OSCOutputGroup *output_group = STLFindOrNull(m_output_map, group);

  if (!output_group) {
    return false;
  }

  OSCTargetVector &targets = output_group->targets;
  OSCTargetVector::iterator target_iter = targets.begin();
  for (; target_iter != targets.end(); ++target_iter) {
    if (**target_iter == target) {
      // the target was found, delete the NodeOSCTarget and remove the entry
      // from the vector.
      delete *target_iter;
      targets.erase(target_iter);
      return true;
    }
  }
  return false;
}


/**
 * Send the DMX data to all targets registered for this group.
 * @param group the group to send the data to
 * @param data_format the format of data to send
 * @param dmx_data the DmxBuffer to send
 * @returns true if successfully sent, false if any error occurred.
 */
bool OSCNode::SendData(unsigned int group, DataFormat data_format,
                       const ola::DmxBuffer &dmx_data) {
  OSCOutputGroup *output_group = STLFindOrNull(m_output_map, group);
  if (!output_group) {
    OLA_WARN << "failed to find " << group;
    // group doesn't exist, just return
    return false;
  }

  switch (data_format) {
    case FORMAT_BLOB:
      return SendBlob(dmx_data, output_group->targets);
    case FORMAT_INT_INDIVIDUAL:
      return SendIndividualInts(dmx_data, output_group);
    case FORMAT_INT_ARRAY:
      return SendIntArray(dmx_data, output_group->targets);
    case FORMAT_FLOAT_INDIVIDUAL:
      return SendIndividualFloats(dmx_data, output_group);
    case FORMAT_FLOAT_ARRAY:
      return SendFloatArray(dmx_data, output_group->targets);
    default:
      OLA_WARN << "Unimplemented data format";
      return false;
  }
}


/**
 * Register a callback to be run when we receive data for an address.
 * De-registration can be performed by passing NULL as a callback. Attempting
 * to register more than once on the same address will return false.
 * @param osc_address the OSC address to register.
 * @param callback the callback to run, ownership is transferred. The callback
 *   can be set to NULL to de-register.
 * @returns false if callback was non-NULL, but the address was already
 *   registered. true otherwise.
 */
bool OSCNode::RegisterAddress(const string &osc_address,
                              DMXCallback *callback) {
  if (callback) {
    // register
    OSCInputGroup *universe_data = STLFindOrNull(m_input_map, osc_address);
    if (universe_data) {
      OLA_WARN << "Attempt to register a second callback for " << osc_address;
      // delete the callback, since we have ownership of it
      delete callback;
      return false;
    } else {
      // This is a new registration, insert into the AddressCallbackMap and
      // register with liblo.
      m_input_map.insert(
          make_pair(osc_address, new OSCInputGroup(callback)));
    }
  } else {
    // deregister
    STLRemoveAndDelete(&m_input_map, osc_address);
  }
  return true;
}


/**
 * Called by OSCDataHandler when there is new data.
 * @param osc_address the OSC address this data arrived on
 * @param data the DmxBuffer containing the data.
 * @param size the number of slots.
 */
void OSCNode::SetUniverse(const string &osc_address, const uint8_t *data,
                          unsigned int size) {
  OSCInputGroup *universe_data = STLFindOrNull(m_input_map, osc_address);
  if (!universe_data)
    return;

  universe_data->dmx.Set(data, size);
  if (universe_data->callback.get()) {
    universe_data->callback->Run(universe_data->dmx);
  }
}

/**
 * Called by OSCDataHandler when there is new data.
 * @param osc_address the OSC address this data arrived on
 * @param slot the slot offset to set.
 * @param value the DMX value for the slot
 */
void OSCNode::SetSlot(const string &osc_address, uint16_t slot, uint8_t value) {
  OSCInputGroup *universe_data = STLFindOrNull(m_input_map, osc_address);
  if (!universe_data)
    return;

  universe_data->dmx.SetChannel(slot, value);

  if (universe_data->callback.get()) {
    universe_data->callback->Run(universe_data->dmx);
  }
}


/**
 * Return the listening port. Will be 0 if the node isn't setup.
 */
uint16_t OSCNode::ListeningPort() const {
  if (m_osc_server) {
    return lo_server_get_port(m_osc_server);
  }
  return 0;
}


/**
 * Called when the OSC FD is readable.
 */
void OSCNode::DescriptorReady() {
  // Call into liblo with a timeout of 0 so we don't block.
  lo_server_recv_noblock(m_osc_server, 0);
}


/**
 * Send a DMXBuffer as a blob to a set of targets
 */
bool OSCNode::SendBlob(const DmxBuffer &dmx_data,
                       const OSCTargetVector &targets) {
  // create the new OSC blob
  lo_blob osc_data = lo_blob_new(dmx_data.Size(), dmx_data.GetRaw());

  bool ok = true;
  // iterate over all the targets, and send to each one.
  OSCTargetVector::const_iterator target_iter = targets.begin();
  for (; target_iter != targets.end(); ++target_iter) {
    OLA_DEBUG << "Sending to " << (*target_iter)->socket_address;
    int ret = lo_send_from((*target_iter)->liblo_address,
                           m_osc_server,
                           LO_TT_IMMEDIATE,
                           (*target_iter)->osc_address.c_str(),
                           "b", osc_data,
                           LO_ARGS_END);
    ok &= (ret > 0);
  }
  // free the blob
  lo_blob_free(osc_data);
  return ok;
}


/**
 * Send a DMXBuffer as individual float messages to a set of targets.
 */
bool OSCNode::SendIndividualFloats(const DmxBuffer &dmx_data,
                                   OSCOutputGroup *group) {
  return SendIndividualMessages(dmx_data, group, "f");
}

/**
 * Send a DMXBuffer as individual int messages to a set of targets.
 */
bool OSCNode::SendIndividualInts(const DmxBuffer &dmx_data,
                                 OSCOutputGroup *group) {
  return SendIndividualMessages(dmx_data, group, "i");
}

/**
 * Send the DmxBuffer as an array of ints.
 */
bool OSCNode::SendIntArray(const DmxBuffer &dmx_data,
                           const OSCTargetVector &targets) {
  lo_message message = lo_message_new();

  for (size_t i = 0; i < dmx_data.Size(); ++i) {
    lo_message_add_int32(message, dmx_data.Get(i));
  }

  bool ok = SendMessageToTargets(message, targets);
  lo_message_free(message);
  return ok;
}

/**
 * Send the DmxBuffer as an array of normalized floats.
 */
bool OSCNode::SendFloatArray(const DmxBuffer &dmx_data,
                             const OSCTargetVector &targets) {
  lo_message message = lo_message_new();

  for (size_t i = 0; i < dmx_data.Size(); ++i) {
    lo_message_add_float(message, dmx_data.Get(i) / 255.0f);
  }

  bool ok = SendMessageToTargets(message, targets);
  lo_message_free(message);
  return ok;
}

/**
 * Send an lo_message to each target in a set.
 * @param message the lo_message to send.
 * @param targets the list of targets to send the message to.
 */
bool OSCNode::SendMessageToTargets(lo_message message,
                                   const OSCTargetVector &targets) {
  bool ok = true;
  OSCTargetVector::const_iterator target_iter = targets.begin();
  for (; target_iter != targets.end(); ++target_iter) {
    int ret = lo_send_message_from(
        (*target_iter)->liblo_address,
        m_osc_server,
        (*target_iter)->osc_address.c_str(),
        message);
    ok &= (ret > 0);
  }
  return ok;
}


/**
 * Send individual messages (one slot per message) to a set of targets.
 * @param dmx_data the DmxBuffer to send
 * @param group the OSCOutputGroup with the targets.
 * @param osc_type the type of OSC message, either "i" or "f"
 */
bool OSCNode::SendIndividualMessages(const DmxBuffer &dmx_data,
                                     OSCOutputGroup *group,
                                     const string &osc_type) {
  bool ok = true;
  const OSCTargetVector &targets = group->targets;

  vector<SlotMessage> messages;

  // We only send the slots that have changed.
  for (unsigned int i = 0; i < dmx_data.Size(); ++i) {
    if (i > group->dmx.Size() || dmx_data.Get(i) != group->dmx.Get(i)) {
      SlotMessage message = {i, lo_message_new()};
      if (osc_type == "i") {
        lo_message_add_int32(message.message, dmx_data.Get(i));
      } else {
        lo_message_add_float(message.message, dmx_data.Get(i) / 255.0f);
      }
      messages.push_back(message);
    }
  }
  group->dmx.Set(dmx_data);

  // Send all messages to each target.
  OSCTargetVector::const_iterator target_iter = targets.begin();
  for (; target_iter != targets.end(); ++target_iter) {
    OLA_DEBUG << "Sending to " << (*target_iter)->socket_address;

    vector<SlotMessage>::const_iterator message_iter = messages.begin();
    for (; message_iter != messages.end(); ++message_iter) {
      std::ostringstream path;
      path << (*target_iter)->osc_address << "/" << message_iter->slot + 1;

      int ret = lo_send_message_from((*target_iter)->liblo_address,
                                     m_osc_server,
                                     path.str().c_str(),
                                     message_iter->message);
      ok &= (ret > 0);
    }
  }

  // Clean up the messages.
  vector<SlotMessage>::iterator message_iter = messages.begin();
  for (; message_iter != messages.end(); ++message_iter) {
    lo_message_free(message_iter->message);
  }

  return ok;
}
}  // namespace osc
}  // namespace plugin
}  // namespace ola
