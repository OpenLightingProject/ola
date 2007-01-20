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
 * llad.cpp
 * This is the main lla daemon
 * Copyright (C) 2005-2007  Simon Newton
 */

#include "llad.h"
#include "client.h"

#include <lla/LlaDevConfMsg.h>
#include <llad/universe.h>
#include <llad/logger.h>
#include <llad/pluginadaptor.h>
#include "UniverseStore.h"

#include <stdio.h>
#include <string.h>

/*
 * Create a new Llad
 *
 */
Llad::Llad() :
  m_term(false),
  m_reload_plugins(false),
  dm(NULL),
  pm(NULL),
  net(NULL),
  pa(NULL),
  m_uni_names("universes"),
  uni_store(NULL) {

}

/*
 * Destroy this object
 *
 */
Llad::~Llad() {

  // this stops and unloads all our plugins
  delete pm;

  // delete all universes
  Universe::clean_up();
  uni_store->save();

  // TODO: we prob want to send disconnect msgs here
  Client::clean_up();

  delete uni_store;
  delete net;
  delete pa;
  delete dm;
}


/*
 * Initialise this object
 *
 * @return  0 on success, -1 on failure
 */
int Llad::init() {
  Plugin *plug;
  int i;

  // setup the objects
  dm = new DeviceManager();
  net = new Network();
  pa = new PluginAdaptor(dm,net);
  pm = new PluginLoader(pa);
  uni_store = new UniverseStore();

  if(dm == NULL || net == NULL || pa == NULL || pm == NULL || uni_store == NULL) {
    delete uni_store;
    delete pm;
    delete net;
    delete pa;
    delete dm;
    return -1;
  }

  // the universe class needs access to the network object to send updates
  Universe::set_net(net);

  // load the universe settings
  uni_store->load();
  Universe::set_store(uni_store);

  // load plugins, this doesn't fail as such
  // rather just tries to load as many plugins as possible
  pm->load_plugins(PLUGIN_DIR);

  // enable all plugins
  for( i =0; i < pm->plugin_count(); i++) {
    plug = pm->get_plugin(i);

    if (plug->start())
      Logger::instance()->log(Logger::WARN, "Failed to start %s", plug->get_name().c_str());
    else
      Logger::instance()->log(Logger::INFO, "Started %s", plug->get_name().c_str());
  }

  // init the network socket
  return net->init();
}


/*
 * Run the daemon
 *
 */
int Llad::run() {
  lla_msg msg;
  int ret;

  Logger::instance()->log(Logger::DEBUG, "Size of lla_msg_plugin_info is %d",  sizeof(lla_msg_plugin_info));
  Logger::instance()->log(Logger::DEBUG, "Size of lla_msg_device_info is %d",  sizeof(lla_msg_device_info));
  Logger::instance()->log(Logger::DEBUG, "Size of lla_msg_port_info is %d",  sizeof(lla_msg_port_info));
  Logger::instance()->log(Logger::DEBUG, "Size of lla_msg_uni_info is %d",  sizeof(lla_msg_uni_info));

  while(!m_term) {

    if(m_reload_plugins)
      _reload_plugins();

    ret = net->read(&msg);

    if(ret < 0) {
      // error
      break;
    } else if (ret > 0)
      // got msg
      this->handle_msg(&msg);
  }
  return 0;
}


/*
 * Stop the daemon
 *
 */
void Llad::terminate() {
  m_term = true;
}

/*
 * Signal to reload plugins
 */
void Llad::reload_plugins() {
  m_reload_plugins = true;
}


/*
 * Reload all plugins
 */
int Llad::_reload_plugins() {
  int i;
  Plugin *plug;

  Logger::instance()->log(Logger::WARN, "Reloading...");

  pm->unload_plugins();

  // load plugins, this doesn't fail as such
  // rather just tries to load as many plugins as possible
  pm->load_plugins(PLUGIN_DIR);

  // enable all plugins
  for( i =0; i < pm->plugin_count(); i++) {
    plug = pm->get_plugin(i);

    if (plug->start())
      Logger::instance()->log(Logger::WARN, "Failed to start %s", plug->get_name().c_str());
    else
      Logger::instance()->log(Logger::INFO, "Started %s", plug->get_name().c_str());
  }

  m_reload_plugins = false;
  return 0;
}



// Communication Functions (used to communicate with clients)
//-----------------------------------------------------------------------------

/*
 * handle a syn
 *
 * we send a syn/ack to the client
 */
int Llad::handle_syn(lla_msg *msg) {
  lla_msg reply;
  Client *cli;

  reply.data.sack.op = LLA_MSG_SYN_ACK;

  reply.to = msg->from;
  reply.len = sizeof(reply.data.sack);

  Logger::instance()->log(Logger::DEBUG, "locating client %d", msg->from.sin_port);

  cli = Client::get_client(msg->from.sin_port);

  if (cli) {
    // the client already exists
    Logger::instance()->log(Logger::WARN, "Got SYN, but client already exists!");
    delete cli;
  }

  cli = Client::get_client_or_create(msg->from.sin_port);

  if(! cli) {
    Logger::instance()->log(Logger::WARN, "Failed to create new client");
    return 0;
  }

  Logger::instance()->log(Logger::DEBUG, "Got SYN, created client, sending SYN ACK");
  net->send_msg(&reply);

  return 0;
}


/*
 * handle a fin (client disconnect)
 *
 * we send a fin/ack to the client
 */
int Llad::handle_fin(lla_msg *msg) {
  lla_msg reply;
  Client *cli;
  vector<Universe *> *uni_v;
  vector<Universe *>::iterator iter;

  cli = Client::get_client_or_create(msg->from.sin_port);

  if(cli != NULL) {
    Logger::instance()->log(Logger::DEBUG, "Got FIN, deleting client");

    // remove from all unvierses
    uni_v = Universe::get_list();

    for(iter = uni_v->begin(); iter != uni_v->end(); ++iter) {
      (*iter)->remove_client(cli);
    }

    delete uni_v;

    Universe::check_for_unused();

    Logger::instance()->log(Logger::DEBUG, "Got FIN, deleting client");
    delete cli;
  }

  reply.data.fack.op = LLA_MSG_FIN_ACK;

  reply.to = msg->from;
  reply.len = sizeof(reply.data.fack);

  Logger::instance()->log(Logger::DEBUG, "Got FIN, sending FIN ACK");
  net->send_msg(&reply);

  return 0;
}


/*
 * Handle a request to fetch the dmx data
 *
 *
 */
int Llad::handle_read_request(lla_msg *msg) {
  Universe *uni = Universe::get_universe(msg->data.rreq.uni);
  Client *cli = Client::get_client(msg->from.sin_port);

  if(uni && cli)
    uni->send_dmx(cli);
  else
    Logger::instance()->log(Logger::DEBUG, "Request for a universe not in use updating universe %d", msg->data.rreq.uni);

  return 0;
}


/*
 * handle a dmx datagram
 *
 *
 */
int Llad::handle_dmx_data(lla_msg *msg) {
  Universe *uni = Universe::get_universe(msg->data.dmx.uni);

  // not interested in this universe
  if(uni == NULL)
    return 0;

  Logger::instance()->log(Logger::DEBUG, "updating universe %d, length %d", uni->get_uid(), msg->data.dmx.len);
  uni->set_dmx(msg->data.dmx.data, msg->data.dmx.len);

  return 0;
}


/*
 * handle a register request
 *
 *
 */
int Llad::handle_register(lla_msg *msg) {

  Client *cli = Client::get_client_or_create(msg->from.sin_port);
  Universe *uni = Universe::get_universe_or_create(msg->data.reg.uni);

  // if universe does not exist, ignore
  if(uni == NULL)
    return 0;

  // check if uni exists here, if not create it
  if(!cli) {
    Logger::instance()->log(Logger::WARN, "Failed to create new client");
    return 0;
  }

  if (msg->data.reg.action == LLA_MSG_REG_REG)
    uni->add_client(cli);
  else {
    uni->remove_client(cli);

    // if this universe is no longer in use we delete it
    if(! uni->in_use() ) {
      delete uni;
    }
  }

  return 0;
}


/*
 * Handle a universe name message
 *
 */
int Llad::handle_uni_name(lla_msg *msg) {
  Universe *uni;
  int uid = msg->data.uniname.uni;

  Logger::instance()->log(Logger::DEBUG, "Setting name for universe %d to %s", msg->data.uniname.uni, msg->data.uniname.name );

  uni = Universe::get_universe(uid);

  if(uni != NULL) {
    string name(msg->data.uniname.name);
    uni->set_name(name);
  }
  return 0;
}


/*
 * Handle a universe name message
 *
 */
int Llad::handle_uni_merge(lla_msg *msg) {
  Universe *uni;
  int uid = msg->data.unimerge.uni;

  Logger::instance()->log(Logger::DEBUG, "Setting merge mode for universe %d to %i", msg->data.unimerge.uni, msg->data.unimerge.mode );

  uni = Universe::get_universe(uid);

  if(uni != NULL) {
    uni->set_merge_mode( msg->data.unimerge.mode == UNI_MERGE_MODE_HTP ? Universe::MERGE_HTP : Universe::MERGE_LTP);
  }
  return 0;
}



/*
 * Handle a patch request
 *
 */
int Llad::handle_patch (lla_msg *msg) {
  Device  *dev;
  Port *prt;
  Universe *uni;
  int uid = msg->data.patch.uni;

  dev = dm->get_dev(msg->data.patch.dev );

  if(dev == NULL) {
    Logger::instance()->log(Logger::WARN, "Device index out of bounds %d", msg->data.patch.dev );
    return 0;
  }

  prt = dev->get_port(msg->data.patch.port);

  if(prt == NULL) {
    Logger::instance()->log(Logger::WARN, "Port index out of bounds %d", msg->data.patch.port );
    return 0;
  }

  Logger::instance()->log(Logger::DEBUG, "Patch request for %d:%d to %d act %d", msg->data.patch.dev, msg->data.patch.port, msg->data.patch.uni, msg->data.patch.action   );

  // patch request
  if (msg->data.patch.action == LLA_MSG_PATCH_ADD) {
    uni = Universe::get_universe_or_create(uid);

    // creation failed
    if( uni == NULL)
      return -1;

    return uni->add_port(prt);

  // unpatch request
  } else if (msg->data.patch.action == LLA_MSG_PATCH_REMOVE) {
    return unpatch_port(prt);

  } else {
    Logger::instance()->log(Logger::WARN, "Undefined action in patch datagram 0x%hhx", msg->data.patch.action );
  }

  return 0;
}


/*
 * handle a plugin info request
 *
 */
int Llad::handle_plugin_info_request(lla_msg *msg) {
  return send_plugin_info(msg->from);
}


/*
 * handle a plugin desc request
 *
 */
int Llad::handle_plugin_desc_request(lla_msg *msg) {
  int pid = msg->data.pldreq.pid;

  Plugin *plug = pm->get_plugin(pid);

  if(plug != NULL)
    // this is a bit of a hack, plugins don't know there own id
    // so we have to pass it here as well
    return send_plugin_desc(msg->from, plug, pid);

  return 0;
}


/*
 * handle a device info request
 *
 */
int Llad::handle_device_info_request(lla_msg *msg) {
  return send_device_info(msg->from, msg->data.dreq.plugin);
}


/*
 * handle a plugin info request
 *
 */
int Llad::handle_universe_info_request(lla_msg *msg) {
  return send_universe_info(msg->from);
}



/*
 * handle a port info request
 *
 */
int Llad::handle_port_info_request(lla_msg *msg) {
  Device *dev = dm->get_dev(msg->data.prreq.devid);

  if(dev != NULL)
    // this is a bit of a hack, devices don't know thier own id
    // so we have to pass it here as well
    return send_port_info(msg->from, dev, msg->data.prreq.devid);

  return 0;
}


/*
 * handle a device config request
 *
 */
int Llad::handle_device_config_request(lla_msg *msg) {
  Device *dev = dm->get_dev(msg->data.devreq.devid);
  LlaDevConfMsg *res = NULL;
  lla_msg reply;
  int l;

  reply.to = msg->from;
  reply.len = sizeof(lla_msg_device_config_rep) - sizeof(reply.data.devrep.rep);

  reply.data.devrep.op = LLA_MSG_DEV_CONFIG_REP;
  reply.data.devrep.status = 1;
  reply.data.devrep.seq = msg->data.devreq.seq;
  reply.data.devrep.len = 0;

  if(dev != NULL) {
    res = dev->configure(msg->data.devreq.req, msg->data.devreq.len);

    if(res != NULL) {
      reply.data.devrep.status = 0;
      l = res->pack(reply.data.devrep.rep, sizeof(reply.data.devrep.rep));

      reply.data.devrep.len = l;
      reply.data.devrep.dev = msg->data.devreq.devid;
      reply.len += l;

      delete res;
      net->send_msg(&reply);
    }
  }
  return 0;
}


/*
 * handle a msg from a client
 *
 *
 */
int Llad::handle_msg(lla_msg *msg) {

  Logger::instance()->log(Logger::DEBUG, "Got msg of type 0x%hhx", msg->data.syn.op );

  switch (msg->data.syn.op) {
    case LLA_MSG_SYN:
      return handle_syn(msg);
      break;
    case LLA_MSG_FIN:
      return handle_fin(msg);
      break;
    case LLA_MSG_READ_REQ :
  //    return handle_read_request(msg);
      break;
    case LLA_MSG_DMX_DATA :
      handle_dmx_data(msg);
      break;
    case LLA_MSG_REGISTER :
      handle_register(msg);
      break;
    case LLA_MSG_PATCH :
      handle_patch(msg);
      break;
    case LLA_MSG_UNI_NAME :
      handle_uni_name(msg);
      break;
    case LLA_MSG_UNI_MERGE :
      handle_uni_merge(msg);
      break;
    case LLA_MSG_PLUGIN_INFO_REQUEST:
      handle_plugin_info_request(msg);
      break;
    case LLA_MSG_DEVICE_INFO_REQUEST:
      handle_device_info_request(msg);
      break;
    case LLA_MSG_PORT_INFO_REQUEST:
      handle_port_info_request(msg);
      break;
    case LLA_MSG_PLUGIN_DESC_REQUEST:
      handle_plugin_desc_request(msg);
      break;
    case LLA_MSG_UNI_INFO_REQUEST:
      handle_universe_info_request(msg);
      break;
    case LLA_MSG_DEV_CONFIG_REQ:
      handle_device_config_request(msg);
      break;

//    case LLA_MSG_INFO:
      // we don't receive these
//      break;
    default:
      Logger::instance()->log(Logger::INFO, "Unknown msg type from client %d\n", msg->data.syn.op);
  }

  return 0;
}


/*
 * Send a plugin_info reply
 *
 */
int Llad::send_plugin_info(struct sockaddr_in dst) {
  Plugin *plug;
  lla_msg reply;
  int i;
  int nplugins = pm->plugin_count();

  // for now we don't worry about sending multiple datagrams
  // if oneday people need to use more than 30 plugins !!!, we can change it
  nplugins = nplugins > PLUGINS_PER_DATAGRAM ? PLUGINS_PER_DATAGRAM : nplugins;

  memset(&reply, 0x00, sizeof(reply));
  reply.to = dst;
  reply.len = sizeof(lla_msg_plugin_info);

  reply.data.plinfo.op = LLA_MSG_PLUGIN_INFO;
  reply.data.plinfo.nplugins = nplugins;
  reply.data.plinfo.offset = 0;
  reply.data.plinfo.count = nplugins;

  for(i=0; i < nplugins; i++) {
    plug = pm->get_plugin(i);

    if(plug != NULL) {
      reply.data.plinfo.plugins[i].id = i;
      strncpy(reply.data.plinfo.plugins[i].name, plug->get_name().c_str(), PLUGIN_NAME_LENGTH);
    }
  }
  Logger::instance()->log(Logger::DEBUG, "Got plugin req, sending reply");

  net->send_msg(&reply);
  return 0;
}


/*
 * Send a device_info reply
 *
 * This provides a client with details on all devices active on the system
 *
 */
int Llad::send_device_info(struct sockaddr_in dst, lla_plugin_id filter) {
  Device *dev;

  lla_msg reply;
  int i;
  int ndevs = dm->device_count();

  // for now we don't worry about sending multiple datagrams
  // if oneday people need to use more than 30 devices !!!, we can change it
  ndevs = ndevs > DEVICES_PER_DATAGRAM ? DEVICES_PER_DATAGRAM : ndevs;

  memset(&reply, 0x00, sizeof(reply));
  reply.to = dst;
  reply.len = sizeof(lla_msg_device_info);

  reply.data.dinfo.op = LLA_MSG_DEVICE_INFO;
  reply.data.dinfo.offset = 0;

  int j=0;
  for(i=0; i < ndevs; i++) {
    dev = dm->get_dev(i);

    if(dev != NULL) {
      Plugin *owner = dev->get_owner();
      if (filter == LLA_PLUGIN_ALL || ( owner != NULL && filter == owner->get_id() ) ) {
        reply.data.dinfo.devices[j].id = i;
        reply.data.dinfo.devices[j].plugin = owner->get_id();
        reply.data.dinfo.devices[j].ports = dev->port_count();
        strncpy(reply.data.dinfo.devices[j].name, dev->get_name().c_str(), DEVICE_NAME_LENGTH);
        ++j;
        if (j == DEVICES_PER_DATAGRAM)
          break;
      }
    }
  }

  reply.data.dinfo.ndevs = j;
  reply.data.dinfo.count = j;
  Logger::instance()->log(Logger::DEBUG, "Got device req, sending reply");

  net->send_msg(&reply);
  return 0;
}


/*
 * Send a port_info reply
 *
 * This provides a client with info on all ports available in a device
 *
 */
int Llad::send_port_info(struct sockaddr_in dst, Device *dev, int devid) {
  Port *prt;
  Universe *uni;

  lla_msg reply;
  int i;
  int nprts = dev->port_count();

  // for now we don't worry about sending multiple datagrams
  // if oneday people need to use more than 60 ports (insane?) !!!, we can change it
  nprts = nprts > PORTS_PER_DATAGRAM ? PORTS_PER_DATAGRAM : nprts;

  memset(&reply, 0x00, sizeof(reply) );
  reply.to = dst;
  reply.len = sizeof(lla_msg_port_info);

  reply.data.prinfo.op = LLA_MSG_PORT_INFO;
  reply.data.prinfo.dev = devid;
  reply.data.prinfo.nports = nprts;
  reply.data.prinfo.offset = 0;
  reply.data.prinfo.count = nprts;

  for(i=0; i < nprts; i++) {
    prt = dev->get_port(i);

    if(prt != NULL) {

      reply.data.prinfo.ports[i].id = i;
      reply.data.prinfo.ports[i].cap = (prt->can_read()?LLA_MSG_PORT_CAP_IN:0) |
                        (prt->can_write()?LLA_MSG_PORT_CAP_OUT:0);

      if( (uni = prt->get_universe() ) ) {
        reply.data.prinfo.ports[i].uni = uni->get_uid();
        reply.data.prinfo.ports[i].actv = 1;
      } else {
        reply.data.prinfo.ports[i].uni = 0;
        reply.data.prinfo.ports[i].actv = 0;
      }
    }
  }
  Logger::instance()->log(Logger::DEBUG, "Got port req, sending reply");

  net->send_msg(&reply);
  return 0;
}


/*
 * Send a plugin_info reply
 *
 */
int Llad::send_plugin_desc(struct sockaddr_in dst, Plugin *plug, int pid) {
  lla_msg reply;

  memset(&reply, 0x00, sizeof(reply) );
  reply.to = dst;
  reply.len = sizeof(lla_msg_plugin_info);

  reply.data.pldesc.op = LLA_MSG_PLUGIN_DESC;
  reply.data.pldesc.pid = pid;
  strncpy(reply.data.pldesc.desc, plug->get_desc().c_str(), PLUGIN_DESC_LENGTH);

  Logger::instance()->log(Logger::DEBUG, "Got plugin desc req, sending reply");
  net->send_msg(&reply);
  return 0;
}


/*
 * Send a universe_info reply
 *
 */
int Llad::send_universe_info(struct sockaddr_in dst) {
  vector<Universe *> *uni_v;
  vector<Universe *>::const_iterator iter;
  lla_msg reply;
  int i, nunis;
  // uni points to the first member
  uni_v = Universe::get_list();
  nunis = uni_v->size();

  // for now we don't worry about sending multiple datagrams
  // if oneday people need to use more than 512 universes !!!, we can change it
  // TODO: introducing a universe comment will drop the limit and
  // we'll need to send more than one datagram
  nunis = nunis > UNIVERSES_PER_DATAGRAM ? UNIVERSES_PER_DATAGRAM : nunis;

  memset(&reply, 0x00, sizeof(reply) );
  reply.to = dst;
  reply.len = sizeof(lla_msg_uni_info);

  reply.data.uniinfo.op = LLA_MSG_UNI_INFO;
  reply.data.uniinfo.nunis = nunis;
  reply.data.uniinfo.offset = 0;
  reply.data.uniinfo.count = nunis;

  for(iter = uni_v->begin(), i=0; iter != uni_v->end() && i < nunis; ++iter, ++i) {
    reply.data.uniinfo.universes[i].id = (*iter)->get_uid();
    (*iter)->get_merge_mode();
    reply.data.uniinfo.universes[i].merge = (*iter)->get_merge_mode() == Universe::MERGE_HTP ? UNI_MERGE_MODE_HTP : UNI_MERGE_MODE_LTP;
    strncpy(reply.data.uniinfo.universes[i].name, (*iter)->get_name().c_str(), UNIVERSE_NAME_LENGTH);
  }

  delete uni_v;
  Logger::instance()->log(Logger::DEBUG, "Got universe req, sending reply");
  net->send_msg(&reply);
  return 0;
}


/*
 * Unpatch a port
 *
 * @param prt the port to patch
 *
 */
int Llad::unpatch_port(Port *prt) {
  Universe *uni = prt->get_universe();

  // check it it's currently patched
  if(uni == NULL)
    return 0;

  // it might not be there, this isn't a problem
  uni->remove_port(prt);

  // if there are no ports left we can destroy this universe
  if( ! uni->in_use() ) {
    delete uni;
  }

  return 0;
}


