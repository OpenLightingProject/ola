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
	Universe *uni = Universe::get_universe_or_create(0) ;

	uni->add_port(prt) ;
	
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
 */
int Llad::handle_fin(lla_msg *msg) {
	lla_msg reply ;

	reply.data.fack.op = LLA_MSG_FIN_ACK ;

	reply.to = msg->from ;
	reply.len = sizeof(reply.data.fack);
	
	Logger::instance()->log(Logger::DEBUG, "Got FIN, sending FIN ACK");
	net->send_msg(&reply);
	
	return 0;

	
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
 * handle a info request
 *
 
int Llad::handle_info_request (Msg *msg) {
	Device *dev ;
	Port *prt ;
	
	lla_msg reply ;	
	int i, j ;
	
	memset(&reply, 0x00, sizeof(reply) );
	
	reply.to = msg->from ;
	reply.len = sizeof(lla_msg_info) ;
	
	reply.data.info.op = LLA_MSG_INFO ;

	printf("got infor requr\n") ;

	// for each device
	for(i=0; i < dm->get_dev_count() ; i++) {
		dev= dm->get_dev(i) ;

		if(dev != NULL) {

			reply.data.info.dev = i ;
			strncpy(reply.data.info.plugin, dev->get_owner()->get_name , sizeof(reply.data.info.plugin) ) ;
			reply.data.info.nports = dev->get_ports() ;
			reply.data.info.offset = 0 ;
			
			for(j=0 ; j < dev->get_ports() ; j++) {
				prt = dev->get_port(j) ;
				
				if(prt != NULL) {

					reply.data.info.ports[j].id = prt->get_id() ;
					reply.data.info.ports[j].uni = prt->get_universe()->get_id()  ;
				}
			}
			printf("sending...\n") ;
			send_msg(&reply) ;
		}
	}
	// send a bunch of replies
	return 0;
}
*/

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

//		case LLA_MSG_INFO_REQUEST:
	//		handle_info_request(msg) ;
//			break;

//		case LLA_MSG_INFO:
			// we don't receive these
//			break ;
		default:
			Logger::instance()->log(Logger::INFO, "Unknown msg type from client %d\n", msg->data.syn.op) ;
	}

	return 0 ;
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
