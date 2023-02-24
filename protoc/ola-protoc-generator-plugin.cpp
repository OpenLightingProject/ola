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
 * ola-protoc-generator-plugin.cpp
 * Copyright (C) 2013 Simon Newton
 */

/**
 * @brief OLA protocol buffer compiler.
 *
 * Generates the service & stub code for an RPC service based on a protocol
 * buffer description.
 *
 * The original open source Protocol Buffers library came with a protoc that
 * would generate Service & Stub classes for services defined in a .proto file.
 *
 * From https://developers.google.com/protocol-buffers/docs/proto#services, as
 * of version 2.3.0 (January 2010), it is considered preferable for RPC
 * implementations to provide their own code to generate these files.
 *
 * The main advantage to generating the code ourselves is that we can define
 * the type of the RpcController, rather than inheriting the interface from
 * google::protobuf::RpcController. This is important because the
 * google::protobuf::RpcController has no method to determine the peer so it
 * has to be worked around with another layer of indirection on the server
 * side.
 *
 * This code should not depend on anything in libola*, since we need the
 * generated service & stub code to build libolacommon. Maybe one day someone
 * will sort out the dependency mess...
 */

#include <google/protobuf/compiler/plugin.h>
#include "protoc/CppGenerator.h"

int main(int argc, char *argv[]) {
  ola::CppGenerator cpp_service_generator;
  return google::protobuf::compiler::PluginMain(argc, argv,
                                                &cpp_service_generator);
}
