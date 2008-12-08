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
 * LlaDummyer.cpp
 * LlaDummyer is a simple Lla Server
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <stdio.h>
#include <string.h>
#include <vector>
#include <iostream>

#include <google/protobuf/stubs/common.h>

#include <lla/select_server/SelectServer.h>
#include <lla/select_server/Socket.h>
#include <lla/LlaClient.h>
#include <lla/LlaDevice.h>
#include <llad/Preferences.h>
#include <llad/logger.h>

#include "DynamicPluginLoader.h"
#include "PluginLoader.h"
#include "LlaServer.h"

using namespace lla;
using namespace lla::select_server;
using lla::proto::LlaServerService_Stub;
using google::protobuf::Closure;
using google::protobuf::NewCallback;

SelectServer ss;
LlaClient *client;
PipeSocket *client_socket;


class SimpleObserver: public lla::LlaClientObserver {
  public:
    SimpleObserver() {}
    ~SimpleObserver() {}
    int Plugins(const vector <LlaPlugin> &plugins, const string &error);
    int Devices(const vector <LlaDevice> devices, const string &error);
    int Universes(const vector <LlaUniverse> universes, const string &error); 
};


int SimpleObserver::Plugins(const vector <LlaPlugin> &plugins,
                            const string &error) {
  printf("--Plugin--\n");
  vector<LlaPlugin>::const_iterator iter;
  for (iter = plugins.begin(); iter != plugins.end(); ++iter) {
    cout << (*iter).Id() << ": " << (*iter).Name() << endl;
  }
  client->FetchDeviceInfo();
}


int SimpleObserver::Devices(const vector<LlaDevice> devices,
                            const string &error) {
  printf("--Device--\n");
  vector<LlaDevice>::const_iterator iter;
  vector<LlaPort>::const_iterator port_iter;
  for (iter = devices.begin(); iter != devices.end(); ++iter) {
    cout << (*iter).Id() << ": " << (*iter).Name() << endl;
    vector<LlaPort> ports = (*iter).Ports();
    for(port_iter = ports.begin(); port_iter != ports.end(); ++port_iter) {
      cout << " " << (*port_iter).Id() << " " << (*port_iter).IsActive() << endl;
    }
  }
  client->FetchUniverseInfo();
}

int SimpleObserver::Universes(const vector<LlaUniverse> universes,
                              const string &error) {
  printf("--Universe--\n");
  vector<LlaUniverse>::const_iterator iter;
  for (iter = universes.begin(); iter != universes.end(); ++iter) {
    cout << (*iter).Id() << ": " << (*iter).Name() << endl;
  }
  ss.RemoveSocket(client_socket);
  client_socket->Close();
}


int main(int argc, char *argv[]) {
  // setup the logger object
  Logger::instance(Logger::INFO, Logger::STDERR);

  LlaServerServiceImplFactory factory;
  PluginLoader *plugin_loader = new DynamicPluginLoader();

  MemoryPreferencesFactory preferences_factory;
  TcpListeningSocket listening_socket("127.0.0.1", 9010);
  LlaServer *server = new LlaServer(&factory,
                                    plugin_loader,
                                    &preferences_factory,
                                    &ss,
                                    &listening_socket);
  int ret = server->Init();

  // setup a socket to talk to the server on
  PipeSocket server_socket;
  server_socket.Init();
  server->NewConnection(&server_socket);

  // call with the client
  client_socket = server_socket.OppositeEnd();
  client = new LlaClient(client_socket);
  client->Setup();
  SimpleObserver observer;
  client->SetObserver(&observer);
  ss.AddSocket(client_socket);

  client->FetchPluginInfo();
  ss.Run();

  server_socket.Close();
  listening_socket.Close();

  delete server;
  delete plugin_loader;
}
