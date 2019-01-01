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
 * E131Device.cpp
 * An E1.31 device
 * Copyright (C) 2007 Simon Newton
 *
 * Ids 0-3 : Input ports (recv dmx)
 * Ids 4-7 : Output ports (send dmx)
 */

#include <google/protobuf/service.h>
#include <google/protobuf/stubs/common.h>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "common/rpc/RpcController.h"
#include "ola/CallbackRunner.h"
#include "ola/Logging.h"
#include "ola/network/Interface.h"
#include "ola/network/InterfacePicker.h"
#include "ola/network/NetworkUtils.h"
#include "olad/Plugin.h"
#include "olad/PluginAdaptor.h"
#include "olad/Preferences.h"
#include "plugins/e131/E131Device.h"
#include "plugins/e131/E131Port.h"
#include "libs/acn/E131Node.h"

namespace ola {
namespace plugin {
namespace e131 {

const char E131Device::DEVICE_NAME[] = "E1.31 (DMX over ACN)";

using ola::acn::E131Node;
using ola::rpc::RpcController;
using std::ostringstream;
using std::set;
using std::string;
using std::vector;

/*
 * Create a new device
 */
E131Device::E131Device(Plugin *owner,
                       const ola::acn::CID &cid,
                       string ip_addr,
                       PluginAdaptor *plugin_adaptor,
                       const E131DeviceOptions &options)
    : Device(owner, DEVICE_NAME),
      m_plugin_adaptor(plugin_adaptor),
      m_options(options),
      m_ip_addr(ip_addr),
      m_cid(cid) {
}


/*
 * Start this device
 */
bool E131Device::StartHook() {
  // stupid Windows, 'interface' seems to be a struct so we use iface here.
  ola::network::Interface iface;
  std::auto_ptr<ola::network::InterfacePicker> picker(
      ola::network::InterfacePicker::NewPicker());
  if (!picker->ChooseInterface(&iface, m_ip_addr, m_plugin_adaptor->DefaultInterface())) {
    OLA_INFO << "Failed to find an interface";
    return false;
  }

  m_node.reset(new E131Node(m_plugin_adaptor, iface, m_options, m_cid));

  if (!m_node->Start()) {
    m_node.reset();
    DeleteAllPorts();
    return false;
  }

  ostringstream str;
  str << DEVICE_NAME << " [" << m_node->GetInterface().ip_address << "]";
  SetName(str.str());

  for (unsigned int i = 0; i < m_options.input_ports; i++) {
    E131InputPort *input_port = new E131InputPort(
        this, i, m_node.get(), m_plugin_adaptor);
    AddPort(input_port);
    m_input_ports.push_back(input_port);
  }

  for (unsigned int i = 0; i < m_options.output_ports; i++) {
    E131OutputPort *output_port = new E131OutputPort(
        this, i, m_node.get());
    AddPort(output_port);
    m_output_ports.push_back(output_port);
  }

  m_plugin_adaptor->AddReadDescriptor(m_node->GetSocket());
  return true;
}


/*
 * Stop this device
 */
void E131Device::PrePortStop() {
  m_plugin_adaptor->RemoveReadDescriptor(m_node->GetSocket());
}


/*
 * Stop this device
 */
void E131Device::PostPortStop() {
  m_node->Stop();
  m_node.reset();
}


/*
 * Handle device config messages
 * @param controller An RpcController
 * @param request the request data
 * @param response the response to return
 * @param done the closure to call once the request is complete
 */
void E131Device::Configure(RpcController *controller,
                           const string &request,
                           string *response,
                           ConfigureCallback *done) {
  CallbackRunner<ConfigureCallback> runner(done);
  Request request_pb;
  if (!request_pb.ParseFromString(request)) {
    controller->SetFailed("Invalid Request");
    return;
  }

  switch (request_pb.type()) {
    case ola::plugin::e131::Request::E131_PORT_INFO:
      HandlePortStatusRequest(response);
      break;
    case ola::plugin::e131::Request::E131_PREVIEW_MODE:
      HandlePreviewMode(&request_pb, response);
      break;
    case ola::plugin::e131::Request::E131_SOURCES_LIST:
      HandleSourceListRequest(&request_pb, response);
      break;
    default:
      controller->SetFailed("Invalid Request");
  }
}


/*
 * Handle a preview mode request.
 */
void E131Device::HandlePreviewMode(const Request *request, string *response) {
  if (request->has_preview_mode()) {
    const ola::plugin::e131::PreviewModeRequest preview_request =
      request->preview_mode();

    unsigned int port_id = preview_request.port_id();
    bool preview_mode = preview_request.preview_mode();

    if (preview_request.input_port()) {
      E131InputPort *e131_port = GetE131InputPort(port_id);
      if (e131_port) {
        // TODO(simon): figure out what to do here
        (void) e131_port;
      }
    } else {
      E131OutputPort *e131_port = GetE131OutputPort(port_id);
      if (e131_port) {
        e131_port->SetPreviewMode(preview_mode);
      }
    }
  }
  HandlePortStatusRequest(response);
}


/*
 * Handle an options request
 */
void E131Device::HandlePortStatusRequest(string *response) {
  ola::plugin::e131::Reply reply;
  reply.set_type(ola::plugin::e131::Reply::E131_PORT_INFO);
  ola::plugin::e131::PortInfoReply *port_reply = reply.mutable_port_info();

  vector<E131InputPort*>::iterator input_iter = m_input_ports.begin();
  for (; input_iter != m_input_ports.end(); ++input_iter) {
    ola::plugin::e131::InputPortInfo *input_port =
      port_reply->add_input_port();
    input_port->set_port_id((*input_iter)->PortId());
    input_port->set_preview_mode(m_options.ignore_preview);
  }

  vector<E131OutputPort*>::iterator output_iter = m_output_ports.begin();
  for (; output_iter != m_output_ports.end(); ++output_iter) {
    ola::plugin::e131::OutputPortInfo *output_port =
      port_reply->add_output_port();
    output_port->set_port_id((*output_iter)->PortId());
    output_port->set_preview_mode((*output_iter)->PreviewMode());
  }
  reply.SerializeToString(response);
}

void E131Device::HandleSourceListRequest(const Request *request,
                                         string *response) {
  typedef std::vector<E131Node::KnownController> KnownControllerList;
  (void) request;
  ola::plugin::e131::Reply reply;
  reply.set_type(ola::plugin::e131::Reply::E131_SOURCES_LIST);
  ola::plugin::e131::SourceListReply *sources_reply =
      reply.mutable_source_list();

  if (!m_options.enable_draft_discovery) {
    sources_reply->set_unsupported(true);
  } else {
    sources_reply->set_unsupported(false);
    KnownControllerList controllers;
    m_node->GetKnownControllers(&controllers);

    KnownControllerList::const_iterator iter = controllers.begin();
    for (; iter != controllers.end(); ++iter) {
      ola::plugin::e131::SourceEntry *entry = sources_reply->add_source();
      entry->set_cid(iter->cid.ToString());
      entry->set_ip_address(iter->ip_address.ToString());
      entry->set_source_name(iter->source_name);

      set<uint16_t>::const_iterator uni_iter = iter->universes.begin();
      for (; uni_iter != iter->universes.end(); ++uni_iter) {
        entry->add_universe(*uni_iter);
      }
    }
  }

  reply.SerializeToString(response);
}

E131InputPort *E131Device::GetE131InputPort(unsigned int port_id) {
  return (port_id < m_input_ports.size()) ? m_input_ports[port_id] : NULL;
}

E131OutputPort *E131Device::GetE131OutputPort(unsigned int port_id) {
  return (port_id < m_output_ports.size()) ? m_output_ports[port_id] : NULL;
}
}  // namespace e131
}  // namespace plugin
}  // namespace ola
