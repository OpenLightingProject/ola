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
 * ola-protoc.cpp
 * Copyright (C) 2013 Simon Newton
 */

/**
 * @brief OLA protocol buffer compiler.
 *
 * Generates the service & stub code for an RPC service based on a protocol
 * buffer description.
 *
 * The orginal open source Protocol Buffers library came with a protoc that
 * would generate Service & Stub classes for services defined in a .proto file.
 *
 * From https://developers.google.com/protocol-buffers/docs/proto#services, as
 * of version 2.3.0 (January 2010), it is considered preferrable for RPC
 * implementations to provide their own code to generate these files.
 *
 * The main advantage to generating the code ourselves is that we can define
 * the type of the RpcController, rather than inheriting the interface from
 * google::protobuf::RpcController. This is important because the
 * google::protobuf::RpcController has no method to determine the peer so it
 * has to be worked around with another layer of indirection on the server
 * side.
 *
 * Ideally this would be a protoc code generator plugin, instead of it's own
 * binary but that's more work than I'm willing to do right now.
 *
 * This code should not depend on anything in libola*, since we need the
 * generated service & stub code to build libolacommon. Maybe one day someone
 * will sort out the dependency mess...
 */

#include <google/protobuf/compiler/command_line_interface.h>
#include "protoc/CppGenerator.h"

int main(int argc, char *argv[]) {
  google::protobuf::compiler::CommandLineInterface cli;
  ola::CppGenerator cpp_service_generator;
  cli.RegisterGenerator("--cppservice_out", &cpp_service_generator,
                        "Generate C++ Service file.");
  return cli.Run(argc, argv);
}
