/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * CppGenerator.cpp
 * Copyright (C) 2013 Simon Newton
 */

#include <google/protobuf/descriptor.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/zero_copy_stream.h>

#include <memory>
#include <string>

#include "protoc/CppGenerator.h"
#include "protoc/CppFileGenerator.h"
#include "protoc/GeneratorHelpers.h"
#include "protoc/ServiceGenerator.h"
#include "protoc/StrUtil.h"

namespace ola {

using google::protobuf::FileDescriptor;
using google::protobuf::ServiceDescriptor;
using google::protobuf::compiler::GeneratorContext;
using google::protobuf::io::Printer;
using std::auto_ptr;
using std::string;

bool CppGenerator::Generate(const FileDescriptor *file,
                            const string&,
                            GeneratorContext *generator_context,
                            string*) const {
  string basename = StripProto(file->name()) + "Service";

  string header_name = basename + ".pb.h";
  string code_name = basename + ".pb.cpp";
  FileGenerator file_generator(file, basename);

  auto_ptr<google::protobuf::io::ZeroCopyOutputStream> header_output(
    generator_context->Open(header_name));
  Printer header_printer(header_output.get(), '$');
  file_generator.GenerateHeader(&header_printer);

  auto_ptr<google::protobuf::io::ZeroCopyOutputStream> code_output(
    generator_context->Open(code_name));
  Printer code_printer(code_output.get(), '$');
  file_generator.GenerateImplementation(&code_printer);

  return true;
}
}  // namespace ola
