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
 * The provides operations on a lla_device.
 * Copyright (C) 2005  Simon Newton
 */

#include "llad.h" 

#include <lla/universe.h>
#include <lla/logger.h>
#include <stdio.h>
#include <string.h>

/*
 * create a new Llad
 *
 *
 */
Llad::Llad() {

	term = false ;
	sd = 0;
	dm = new DeviceManager() ;
	net = new Network() ;
	pa = new PluginAdaptor(dm,net) ;
	pm = new PluginLoader(pa) ;
}

/*
 * Clean up
 *
 */
Llad::~Llad() {

	Logger::instance()->log(Logger::DEBUG, "In llad destructor") ;

	// this stops and unloads all our plugins
	delete pm;

	// delete all universes
	Universe::clean_up() ;

	delete net;
	delete pa;
	delete dm;
}


/*
 * initialise the dameon
 *
 * @return	0 on success, -1 on failure
 */
int Llad::init() {
	Plugin *plug ;
	int i;

	// load plugins
	pm->load_plugins(PLUGIN_DIR) ;

	// enable all plugins
	for( i =0 ; i < pm->plugin_count() ; i++) {
		plug = pm->get_plugin(i) ;

		if (plug->start())
			Logger::instance()->log(Logger::WARN, "Failed to start %s plugin\n", plug->get_name()) ;
		else
			Logger::instance()->log(Logger::INFO, "%s plugin enabled", plug->get_name()) ;
	}

	// init the network socket
	net->init() ;

	return 0;
}


/*
 * run the daemon
 * 
 */
int Llad::run() {
	lla_msg msg ;
	int ret ;
	Device *dev = dm->get_dev(0) ;
	Port *prt = dev->get_port(0) ;
	Universe *uni = Universe::get_universe_or_create(1) ;

	uni->add_port(prt) ;
	
	
	Logger::instance()->log(Logger::DEBUG, "Size of lla_msg_plugin_info is %d",  sizeof(lla_msg_plugin_info)) ;
	Logger::instance()->log(Logger::DEBUG, "Size of lla_msg_device_info is %d",  sizeof(lla_msg_device_info)) ;
	Logger::instance()->log(Logger::DEBUG, "Size of lla_msg_port_info is %d",  sizeof(lla_msg_port_info)) ;
	
	dev = dm->get_dev(3) ;
	prt = dev->get_port(0) ;
	uni->add_port(prt) ;

	while(!term) {
		ret = net->read(&msg) ;

		if(ret < 0) 
			// error
			break;
		else if (ret > 0)
			// got msg
			this->handle_msg(&msg) ;
	}
}


/*
 * Stop the daemon
 *
 */
void Llad::terminate() {
	term = true ;
}




// Communication Functions (used to communicate with clients)
//-----------------------------------------------------------------------------

/*
 * handle a syn
 *
 * we send a syn/ack to the client
 */
int Llad::handle_syn(lla_msg *msg) {
	lla_msg reply ;

	reply.data.sack.op = LLA_MSG_SYN_ACK ;

	reply.to = msg->from ;
	reply.len = sizeof(reply.data.sack);
	
	Logger::instance()->log(Logger::DEBUG, "Got SYN, sending SYN ACK");
	net->send_msg(&reply);
	
	return 0;
}


/*
 * handle a fin
 *
 * we send a fin/ack to the client
 */
int Llad::handle_fin(lla_msg *msg) {
	lla_msg reply ;

	reply.data.fack.op = LLA_MSG_FIN_ACK ;

	reply.to = msg->from ;
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
/*int Llad::handle_read_request(ReadMsg *msg) {
	DmxMsg reply = new DmxMsg() ;
	int uid = msg->get_uid() ;
	Universe *uni = Universe::get_universe(uid) ;

	if(uni) {
		reply->set_to(msg->get_from) ;
		reply->set_uid(uid) ;
		
		reply->data.dmx.len = 512 ;
		uni->get_dmx(reply.data.dmx.data, 512) ;
		
		return reply->send(net) ;
	} else {
		printf("request for a universe not in use %d\n" , msg->get_uid()) ;
	}
	return 0;
}
*/

/*
 * handle a dmx datagram
 *
 *
 */
int Llad::handle_dmx_data (lla_msg *msg) {
	Universe *uni = Universe::get_universe( msg->data.dmx.uni ) ;

	// not interested in this universe
	if(uni == NULL) 
		return 0 ;

	Logger::instance()->log(Logger::DEBUG, "updating universe %d", uni->get_uid() );
	uni->set_dmx(msg->data.dmx.data, msg->data.dmx.len) ;

	return 0;
}


/*
 * handle a register request
 *
 *
 */
/*
int Llad::handle_register (Msg *msg) {


	int uid = msg->data.reg.uni ; 
 	Client *cli = Client::get_from_addr(msg->from) ;

	Universe *uni = Universe::get_universe(uid) ;

	// check if uni exists here, if not create it

	if(cli == NULL) {
		cli = new Client(msg->from) ;

		if(cli == NULL) 
			return -1 ;
	}
	
	if add
		uni->add_client(cli) ;
	else remove
		uni->remove_client(cli) ;

	return 0;
}
*/

/*
 * Handle a patch request
 *
 */

int Llad::handle_patch (lla_msg *msg) {
	Device  *dev ;
	Port *prt ;
	Universe *uni ;
	int uid = msg->data.patch.uni ;
	
	dev = dm->get_dev(msg->data.patch.dev ) ;

	if(dev == NULL) {
		Logger::instance()->log(Logger::WARN, "Device index out of bounds %d\n", msg->data.patch.dev ) ;
		return 0;
	}

	prt = dev->get_port(msg->data.patch.port) ;

	if(prt == NULL) {
		Logger::instance()->log(Logger::WARN, "Port index out of bounds %d\n", msg->data.patch.port ) ;
		return 0;
	}

	Logger::instance()->log(Logger::WARN, "Patch request for %d:%d to %d act %d\n", msg->data.patch.dev, msg->data.patch.port, msg->data.patch.uni, msg->data.patch.action   ) ;
	
	// patch request
	if (msg->data.patch.action == LLA_MSG_PATCH_ADD) {
		uni = Universe::get_universe_or_create(uid) ;
	
		// creation failed
		if( uni == NULL)
			return -1 ; 

		return uni->add_port(prt) ;
		
	// unpatch request
	} else if (msg->data.patch.action == LLA_MSG_PATCH_REMOVE) {
		return unpatch_port(prt) ;		
		
	} else {
		printf("undefined action in patch datagram 0x%hhx\n", msg->data.patch.action ) ;
	}

	return 0 ;
	
e_param:
	printf("patch msg: (unknown dev or port)\n") ;
	return 0 ;
}


/*
 * handle a plugin info request
 *
 */
int Llad::handle_plugin_info_request(lla_msg *msg) {

	return send_plugin_info(msg->from) ;
}

/*
 * handle a plugin desc request
 *
 */
int Llad::handle_plugin_desc_request(lla_msg *msg) {
	int pid = msg->data.pldreq.pid ;

	Plugin *plug = pm->get_plugin(pid) ;

	if(plug != NULL) 
		// this is a bit of a hack, plugins don't know there own id
		// so we have to pass it here as well
		return send_plugin_desc(msg->from, plug, pid) ;

	return 0;
}

/*
 * handle a device info request
 *
 */
int Llad::handle_device_info_request(lla_msg *msg) {

	return send_device_info(msg->from) ;
}


/*
 * handle a plugin info request
 *
 */
int Llad::handle_universe_info_request(lla_msg *msg) {

	return send_universe_info(msg->from) ;
}



/*
 * handle a port info request
 *
 */
int Llad::handle_port_info_request(lla_msg *msg) {
	Device *dev;

	dev = dm->get_dev(msg->data.prreq.devid) ;

	if(dev != NULL)
		// this is a bit of a hack, devices don't know there own id
		// so we have to pass it here as well
		return send_port_info(msg->from, dev, msg->data.prreq.devid) ;

	return 0;
}


/*
 * handle a msg from a client
 *
 *
 */
int Llad::handle_msg(lla_msg *msg) {

	switch (msg->data.syn.op) {
		case LLA_MSG_SYN:
			return handle_syn(msg) ;
			break ;
		case LLA_MSG_FIN:
			return handle_fin(msg) ;
			break ;
		case LLA_MSG_READ_REQ :
	//		return handle_read_request(msg) ;
			break;
		case LLA_MSG_DMX_DATA :
			handle_dmx_data(msg);
			break;
		case LLA_MSG_REGISTER :
	//		handle_register(msg);
			break ;

		case LLA_MSG_PATCH :
			handle_patch(msg) ;
			break ;

		case LLA_MSG_PLUGIN_INFO_REQUEST:
			handle_plugin_info_request(msg) ;
			break;
		case LLA_MSG_DEVICE_INFO_REQUEST:
			handle_device_info_request(msg) ;
			break;
		case LLA_MSG_PORT_INFO_REQUEST:
			handle_port_info_request(msg) ;
			break;
		case LLA_MSG_PLUGIN_DESC_REQUEST:
			handle_plugin_desc_request(msg) ;
			break;
		case LLA_MSG_UNI_INFO_REQUEST:
			handle_universe_info_request(msg) ;
			break;

//		case LLA_MSG_INFO:
			// we don't receive these
//			break ;
		default:
			Logger::instance()->log(Logger::INFO, "Unknown msg type from client %d\n", msg->data.syn.op) ;
	}

	return 0 ;
}








/*
 * Send a plugin_info reply
 *
 *
 *
 */
int Llad::send_plugin_info(struct sockaddr_in dst) {
	Plugin *plug ;
	
	lla_msg reply ;
	int i ;
	int nplugins = pm->plugin_count();
	

	// for now we don't worry about sending multiple datagrams
	// if oneday people need to use more than 30 plugins !!!, we can change it
	nplugins = nplugins > PLUGINS_PER_DATAGRAM ? PLUGINS_PER_DATAGRAM : nplugins ;

	printf("sending info reply\n") ;
	memset(&reply, 0x00, sizeof(reply) );
	reply.to = dst ;
	reply.len = sizeof(lla_msg_plugin_info) ;
	
	reply.data.plinfo.op = LLA_MSG_PLUGIN_INFO ;
	reply.data.plinfo.nplugins = nplugins ;
	reply.data.plinfo.offset = 0 ;
	reply.data.plinfo.count = nplugins ;
	
	for(i=0; i < nplugins ; i++) {
		plug = pm->get_plugin(i) ;

		if(plug != NULL) {

			reply.data.plinfo.plugins[i].id = i ;
			strncpy(reply.data.plinfo.plugins[i].name, plug->get_name(), PLUGIN_NAME_LENGTH) ;
				
		}
	}
	
	printf("sending...\n") ;
	net->send_msg(&reply);
	return 0;

}

/*
 * Send a device_info reply
 *
 * This provides a client with details on all devices active on the system
 *
 */
int Llad::send_device_info(struct sockaddr_in dst) {
	Device *dev ;
	
	lla_msg reply ;
	int i ;
	int ndevs = dm->device_count();
	

	// for now we don't worry about sending multiple datagrams
	// if oneday people need to use more than 30 devices !!!, we can change it
	ndevs = ndevs > DEVICES_PER_DATAGRAM ? DEVICES_PER_DATAGRAM : ndevs ;

	printf("sending dev info reply\n") ;
	memset(&reply, 0x00, sizeof(reply) );
	reply.to = dst ;
	reply.len = sizeof(lla_msg_device_info) ;
	
	reply.data.dinfo.op = LLA_MSG_DEVICE_INFO ;
	reply.data.dinfo.ndevs = ndevs ;
	reply.data.dinfo.offset = 0 ;
	reply.data.dinfo.count = ndevs ;
	
	for(i=0; i < ndevs ; i++) {
		dev = dm->get_dev(i) ;

		if(dev != NULL) {

			reply.data.dinfo.devices[i].id = i ;
			reply.data.dinfo.devices[i].ports = dev->port_count() ;
			strncpy(reply.data.dinfo.devices[i].name, dev->get_name(), DEVICE_NAME_LENGTH) ;
				
		}
	}
	
	printf("sending...\n") ;
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
	Port *prt ;
	Universe *uni ;
	
	lla_msg reply ;
	int i ;
	int nprts = dev->port_count();
	
	// for now we don't worry about sending multiple datagrams
	// if oneday people need to use more than 60 ports (insane?) !!!, we can change it
	nprts = nprts > PORTS_PER_DATAGRAM ? PORTS_PER_DATAGRAM : nprts ;

	printf("sending port info reply\n") ;
	memset(&reply, 0x00, sizeof(reply) );
	reply.to = dst ;
	reply.len = sizeof(lla_msg_port_info) ;
	
	reply.data.prinfo.op = LLA_MSG_PORT_INFO ;
	reply.data.prinfo.dev = devid ;
	reply.data.prinfo.nports = nprts ;
	reply.data.prinfo.offset = 0 ;
	reply.data.prinfo.count = nprts ;
	
	for(i=0; i < nprts ; i++) {
		prt = dev->get_port(i) ;

		if(prt != NULL) {

			reply.data.prinfo.ports[i].id = i ;
			reply.data.prinfo.ports[i].cap = (prt->can_read()?LLA_MSG_PORT_CAP_IN:0) |
												(prt->can_write()?LLA_MSG_PORT_CAP_OUT:0) ;

			printf("id %d, cap %hx\n", i, reply.data.prinfo.ports[i].cap ) ;
			if( (uni = prt->get_universe() ) ) {
				reply.data.prinfo.ports[i].uni = uni->get_uid();
				reply.data.prinfo.ports[i].actv = 1;
			} else {
				reply.data.prinfo.ports[i].uni = 0;
				reply.data.prinfo.ports[i].actv = 0;
				
			}
		}
	}
	
	printf("sending...\n") ;
	net->send_msg(&reply);
	return 0;

}



/*
 * Send a plugin_info reply
 *
 *
 *
 */
int Llad::send_plugin_desc(struct sockaddr_in dst, Plugin *plug, int pid) {
	
	lla_msg reply ;
	int i ;
	int nplugins = pm->plugin_count();
	

	memset(&reply, 0x00, sizeof(reply) );
	reply.to = dst ;
	reply.len = sizeof(lla_msg_plugin_info) ;
	
	printf("pid is %d\n", pid) ;
	reply.data.pldesc.op = LLA_MSG_PLUGIN_DESC ;
	reply.data.pldesc.pid = pid ;
	strncpy(reply.data.pldesc.desc , plug->get_desc()  , PLUGIN_DESC_LENGTH) ;

	net->send_msg(&reply);
	return 0;

}


/*
 * Send a universe_info reply
 *
 *
 *
 */
int Llad::send_universe_info(struct sockaddr_in dst) {
	Universe *uni ;
	
	lla_msg reply ;
	int i ;
	int nunis = Universe::universe_count();
	
	// for now we don't worry about sending multiple datagrams
	// if oneday people need to use more than 512 plugins !!!, we can change it
	// FIX: introducing a universe comment will drop the limit and
	// we'll need to send more than one datagram
	nunis = nunis > UNIVERSES_PER_DATAGRAM ? UNIVERSES_PER_DATAGRAM : nunis ;

	printf("sending uni info reply\n") ;
	memset(&reply, 0x00, sizeof(reply) );
	reply.to = dst ;
	reply.len = sizeof(lla_msg_uni_info) ;
	
	reply.data.uniinfo.op = LLA_MSG_UNI_INFO ;
	reply.data.uniinfo.nunis = nunis ;
	reply.data.uniinfo.offset = 0 ;
	reply.data.uniinfo.count = nunis ;
	
	// FIX : damnit all these loops need fixing, if the object is null we incorrectly 
	// increment
	for(i=0; i < nunis ; i++) {
		uni = Universe::get_universe_at_pos(i) ;

		if(uni != NULL) {
			reply.data.uniinfo.universes[i].id = uni->get_uid() ;
		}
	}
	
	printf("sending...\n") ;
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
	Universe *uni = prt->get_universe() ;
	
	// check it it's currently patched
	if(uni == NULL)
		return 0; 
	
	// it might not be there, this isn't a problem
	uni->remove_port(prt) ;

	// if there are no ports left we can destroy this universe
	if(uni->get_num_ports() == 0) {
		delete uni ;
	}
	return 0;
}
