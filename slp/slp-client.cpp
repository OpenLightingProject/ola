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
 * slp-client.cpp
 * Copyright (C) 2012 Simon Newton
 */

#include <stdio.h>
#include <signal.h>

#include <ola/Callback.h>
#include <ola/Logging.h>
#include <ola/StringUtils.h>
#include <ola/base/Flags.h>
#include <ola/base/Init.h>
#include <ola/base/SysExits.h>
#include <ola/io/SelectServer.h>

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "ola/slp/SLPClient.h"
#include "slp/SLPUtil.h"

using ola::NewCallback;
using ola::NewSingleCallback;
using ola::io::SelectServer;
using ola::network::TCPAcceptingSocket;
using ola::slp::SLPClient;
using ola::slp::SLPErrorToString;
using ola::slp::URLEntry;
using std::auto_ptr;
using std::cerr;
using std::cout;
using std::endl;
using std::string;
using std::vector;

DEFINE_s_string(scopes, s, ola::slp::DEFAULT_SLP_SCOPE,
                "Comma separated list of scopes.");
DEFINE_uint16(lifetime, 300, "The lifetime of the service (seconds).");

// The base slp client command class.
class Command {
  public:
    explicit Command(const string &scopes)
        : m_terminate(NULL) {
      ola::StringSplit(scopes, m_scopes, ",");
    }

    virtual ~Command() {
      if (m_terminate)
        delete m_terminate;
    }

    void SetTermination(ola::Callback0<void> *terminate) {
      m_terminate = terminate;
    }

    void Terminate() { m_terminate->Run(); }
    virtual bool Execute(SLPClient *client) = 0;

  protected:
    vector<string> m_scopes;

    bool IsError(const string &error) {
      if (error.empty())
        return false;
      OLA_WARN << error;
      return true;
    }

  private:
    ola::Callback0<void> *m_terminate;
};


class FindCommand: public Command {
  public:
    explicit FindCommand(const string &scopes,
                         const string service)
        : Command(scopes),
          m_service(service) {
    }

    bool Execute(SLPClient *client) {
      return client->FindService(
          m_scopes,
          m_service,
          NewSingleCallback(this, &FindCommand::RequestComplete));
    }

  private:
    const string m_service;

    void RequestComplete(const string &error,
                         const vector<URLEntry> &services) {
      Terminate();
      if (IsError(error))
        return;
      for (vector<URLEntry>::const_iterator iter = services.begin();
           iter != services.end(); ++iter)
        cout << iter->url() << ", " << iter->lifetime() << endl;
    }
};


class RegisterCommand: public Command {
  public:
    explicit RegisterCommand(const string &scopes,
                             const string service,
                             uint16_t lifetime)
      : Command(scopes),
        m_service(service),
        m_lifetime(lifetime) {
    }

    bool Execute(SLPClient *client) {
      return client->RegisterPersistentService(
          m_scopes,
          m_service, m_lifetime,
          NewSingleCallback(this, &RegisterCommand::RequestComplete));
    }

  private:
    string m_service;
    uint16_t m_lifetime;

    void RequestComplete(const string &error, uint16_t code) {
      Terminate();
      if (IsError(error))
        return;
      cerr << "SLP code is " << code << " : " << SLPErrorToString(code) << endl;
    }
};


class DeRegisterCommand: public Command {
  public:
    explicit DeRegisterCommand(const string &scopes,
                               const string service)
      : Command(scopes),
        m_service(service) {
    }

    bool Execute(SLPClient *client) {
      return client->DeRegisterService(
          m_scopes,
          m_service,
          NewSingleCallback(this, &DeRegisterCommand::RequestComplete));
    }

  private:
    string m_service;

    void RequestComplete(const string &error, uint16_t code) {
      Terminate();
      if (IsError(error))
        return;
      cerr << "SLP code is " << code << " : " << SLPErrorToString(code) << endl;
    }
};


/**
 * Return a command object or none if the args were invalid.
 */
Command *CreateCommand(const vector<string> &args) {
  if (args.empty())
    return NULL;

  if (args[0] == "findsrvs") {
    return args.size() == 2 ?
           new FindCommand(FLAGS_scopes.str(), args[1]) :
           NULL;
  } else if (args[0] == "deregister") {
    return args.size() == 2 ?
           new DeRegisterCommand(FLAGS_scopes.str(), args[1])
           : NULL;
  } else if (args[0] == "register") {
    return args.size() == 2 ?
           new RegisterCommand(FLAGS_scopes.str(), args[1], FLAGS_lifetime)
           : NULL;
  }
  return NULL;
}


/*
 * Startup the server.
 */
int main(int argc, char *argv[]) {
  std::ostringstream help_msg;
  help_msg <<
      "The OLA SLP client.\n"
      "\n"
      "Examples:\n"
      "   " << argv[0] << " register service:myserv.x://myhost.com\n"
      "   " << argv[0] << " findsrvs service:myserv.x\n"
      "   " << argv[0] << " findsrvs service:myserv.x";

  ola::SetHelpString(" [options] command-and-arguments", help_msg.str());
  ola::ParseFlags(&argc, argv);

  vector<string> args;
  for (int i = 1; i < argc; i++) {
    args.push_back(argv[i]);
  }

  auto_ptr<Command> command(CreateCommand(args));
  if (!command.get()) {
    ola::DisplayUsage();
    exit(ola::EXIT_OK);
  }

  ola::InitLoggingFromFlags();
  ola::AppInit(argc, argv);

  ola::slp::SLPClientWrapper client_wrapper;
  if (!client_wrapper.Setup())
    exit(1);

  SelectServer *ss = client_wrapper.GetSelectServer();
  command->SetTermination(NewCallback(ss, &SelectServer::Terminate));

  if (!command->Execute(client_wrapper.GetClient()))
    exit(1);
  ss->Run();
}
