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
 * E131Device.cpp
 * An E1.31 device
 * Copyright (C) 2007-2009 Simon Newton
 *
 * Ids 0-3 : Input ports (recv dmx)
 * Ids 4-7 : Output ports (send dmx)
 */

#include "plugins/e131/e131/E131Includes.h"  //  NOLINT, this has to be first
#include <google/protobuf/service.h>
#include <google/protobuf/stubs/common.h>
#include <string>
#include <vector>

#include "ola/Logging.h"
#include "olad/Plugin.h"
#include "olad/PluginAdaptor.h"
#include "olad/Preferences.h"
#include "plugins/e131/E131Device.h"
#include "plugins/e131/E131Port.h"
#include "plugins/e131/e131/E131Node.h"

namespace ola {
namespace plugin {
namespace e131 {

/*
 * Create a new device
 */
E131Device::E131Device(Plugin *owner, const string &name,
                       const ola::plugin::e131::CID &cid,
                       std::string ip_addr,
                       const PluginAdaptor *plugin_adaptor,
                       bool use_rev2,
                       bool prepend_hostname,
                       bool ignore_preview,
                       uint8_t dscp)
    : Device(owner, name),
      m_plugin_adaptor(plugin_adaptor),
      m_node(NULL),
      m_use_rev2(use_rev2),
      m_prepend_hostname(prepend_hostname),
      m_ignore_preview(ignore_preview),
      m_dscp(dscp),
      m_ip_addr(ip_addr),
      m_cid(cid) {
}


/*
 * Start this device
 */
bool E131Device::StartHook() {
  m_node = new E131Node(m_ip_addr, m_cid, m_use_rev2, m_ignore_preview,
                        m_dscp);

  if (!m_node->Start()) {
    delete m_node;
    m_node = NULL;
    DeleteAllPorts();
    return false;
  }

  for (unsigned int i = 0; i < NUMBER_OF_E131_PORTS; i++) {
    E131InputPort *input_port = new E131InputPort(
        this,
        i,
        m_node,
        m_plugin_adaptor->WakeUpTime());
    AddPort(input_port);
    E131OutputPort *output_port = new E131OutputPort(this,
                                                     i,
                                                     m_node,
                                                     m_prepend_hostname);
    AddPort(output_port);
  }

  m_plugin_adaptor->AddSocket(m_node->GetSocket());
  return true;
}


/*
 * Stop this device
 */
void E131Device::PrePortStop() {
  m_plugin_adaptor->RemoveSocket(m_node->GetSocket());
}


/*
 * Stop this device
 */
void E131Device::PostPortStop() {
  m_node->Stop();
  delete m_node;
  m_node = NULL;
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
                           google::protobuf::Closure *done) {
    Request request_pb;
    if (!request_pb.ParseFromString(request)) {
      controller->SetFailed("Invalid Request");
      done->Run();
      return;
    }

    switch (request_pb.type()) {
      case ola::plugin::e131::Request::E131_PORT_INFO:
        HandlePortStatusRequest(response);
        break;
      case ola::plugin::e131::Request::E131_PREVIEW_MODE:
        HandlePreviewMode(&request_pb, response);
        break;
      default:
        controller->SetFailed("Invalid Request");
    }
    done->Run();
}


/*
 * Handle an preview mode request.
 */
void E131Device::HandlePreviewMode(Request *request, string *response) {
  if (request->has_preview_mode()) {
    const ola::plugin::e131::PreviewModeRequest preview_request =
      request->preview_mode();

    unsigned int port_id = preview_request.port_id();
    bool preview_mode = preview_request.preview_mode();

    if (preview_request.input_port()) {
      InputPort *port = GetInputPort(port_id);
      if (port) {
        E131InputPort *e131_port = reinterpret_cast<E131InputPort*>(port);
        // TODO(simon): figure out what to do here
        (void) e131_port;
        // e131_port->SetPreviewMode(preview_mode);
      }
    } else {
      OutputPort *port = GetOutputPort(port_id);
      if (port) {
        E131OutputPort *e131_port = reinterpret_cast<E131OutputPort*>(port);
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

  vector<InputPort*> input_ports;
  vector<OutputPort*> output_ports;
  InputPorts(&input_ports);
  OutputPorts(&output_ports);

  for (unsigned int i = 0; i < input_ports.size(); i++) {
    ola::plugin::e131::InputPortInfo *input_port =
      port_reply->add_input_port();
    input_port->set_port_id(i);
    input_port->set_preview_mode(m_ignore_preview);
  }

  for (unsigned int i = 0; i < output_ports.size(); i++) {
    ola::plugin::e131::OutputPortInfo *output_port =
      port_reply->add_output_port();
    output_port->set_port_id(i);
    E131OutputPort *e131_port =
      reinterpret_cast<E131OutputPort*>(output_ports[i]);
    output_port->set_preview_mode(e131_port->PreviewMode());
  }
  reply.SerializeToString(response);
}
}  // e131
}  // plugin
}  // ola
