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
 * slp-client.cpp
 * Copyright (C) 2012 Simon Newton
 */

#include <stdio.h>
#include <getopt.h>
#include <signal.h>
#include <sysexits.h>

#include <ola/Callback.h>
#include <ola/Logging.h>
#include <ola/base/Init.h>
#include <ola/io/SelectServer.h>

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "tools/slp/SLPClient.h"

using ola::NewCallback;
using ola::NewSingleCallback;
using ola::io::SelectServer;
using ola::network::TCPAcceptingSocket;
using ola::slp::SLPClient;
using ola::slp::SLPService;
using std::auto_ptr;
using std::cout;
using std::endl;
using std::string;
using std::vector;


struct Options {
  public:
    bool help;
    ola::log_level log_level;
    vector<string> extra_args;

    Options()
        : help(false),
          log_level(ola::OLA_LOG_WARN) {
    }
};


/*
 * Parse our command line options
 */
void ParseOptions(int argc, char *argv[],
                  Options *options) {
  static struct option long_options[] = {
      {"help", no_argument, 0, 'h'},
      {"log-level", required_argument, 0, 'l'},
      {0, 0, 0, 0}
  };

  int option_index = 0;

  while (1) {
    int c = getopt_long(argc, argv, "hl:", long_options, &option_index);

    if (c == -1)
      break;

    switch (c) {
      case 'h':
        options->help = true;
        break;
      case 'l':
        switch (atoi(optarg)) {
          case 0:
            // nothing is written at this level
            // so this turns logging off
            options->log_level = ola::OLA_LOG_NONE;
            break;
          case 1:
            options->log_level = ola::OLA_LOG_FATAL;
            break;
          case 2:
            options->log_level = ola::OLA_LOG_WARN;
            break;
          case 3:
            options->log_level = ola::OLA_LOG_INFO;
            break;
          case 4:
            options->log_level = ola::OLA_LOG_DEBUG;
            break;
          default :
            break;
        }
        break;
      case '?':
        break;
      default:
       break;
    }
  }

  int index = optind;
  for (; index < argc; index++)
    options->extra_args.push_back(argv[index]);
}


/*
 * Display the help message
 */
void DisplayHelpAndExit(char *argv[]) {
  cout << "Usage: " << argv[0] << " [options] command-and-arguments\n"
  "\n"
  "The OLA SLP client.\n"
  "\n"
  "  -h, --help               Display this help message and exit.\n"
  "  -l, --log-level <level>  Set the logging level 0 .. 4.\n"
  " Examples:\n"
  "   " << argv[0] << " register service:myserv.x://myhost.com\n"
  "   " << argv[0] << " findsrvs service:myserv.x\n"
  << endl;
  exit(0);
}

// The base slp client command class.
class Command {
  public:
    Command() : m_terminate(NULL) {}

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
    explicit FindCommand(const string service) : m_service(service) {}

    bool Execute(SLPClient *client) {
      return client->FindService(
          m_service,
          NewSingleCallback(this, &FindCommand::RequestComplete));
    }

  private:
    string m_service;

    void RequestComplete(const string &error,
                         const vector<SLPService> &services) {
      Terminate();
      if (IsError(error))
        return;
      vector<SLPService>::const_iterator iter = services.begin();
      cout << "Services:" << endl;
      for (; iter != services.end(); ++iter)
        cout << iter->name << ", " << iter->lifetime << endl;
    }
};


class RegisterCommand: public Command {
  public:
    explicit RegisterCommand(const string service) : m_service(service) {}

    bool Execute(SLPClient *client) {
      return client->RegisterPersistentService(
          m_service, 300,
          NewSingleCallback(this, &RegisterCommand::RequestComplete));
    }

  private:
    string m_service;

    void RequestComplete(const string &error, uint16_t code) {
      Terminate();
      if (IsError(error))
        return;
      cout << "SLP code is " << code;
    }
};


class DeRegisterCommand: public Command {
  public:
    explicit DeRegisterCommand(const string service) : m_service(service) {}

    bool Execute(SLPClient *client) {
      return client->DeRegisterService(
          m_service,
          NewSingleCallback(this, &DeRegisterCommand::RequestComplete));
    }

  private:
    string m_service;

    void RequestComplete(const string &error, uint16_t code) {
      Terminate();
      if (IsError(error))
        return;
      cout << "SLP code is " << code;
    }
};


/**
 * Return a command object or none if the args were invalid.
 */
Command *CreateCommand(const vector<string> &args) {
  if (args.empty())
    return NULL;

  if (args[0] == "findsrvs") {
    return args.size() == 2 ? new FindCommand(args[1]) : NULL;
  } else if (args[0] == "deregister") {
    return args.size() == 2 ? new DeRegisterCommand(args[1]) : NULL;
  } else if (args[0] == "register") {
    return args.size() == 2 ? new RegisterCommand(args[1]) : NULL;
  }
  return NULL;
}


/*
 * Startup the server.
 */
int main(int argc, char *argv[]) {
  Options options;
  ParseOptions(argc, argv, &options);

  if (options.help)
    DisplayHelpAndExit(argv);

  auto_ptr<Command> command(CreateCommand(options.extra_args));
  if (!command.get())
    DisplayHelpAndExit(argv);

  ola::InitLogging(options.log_level, ola::OLA_LOG_STDERR);
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
