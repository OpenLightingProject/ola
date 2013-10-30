// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.
//
// Edited by Simon Newton for OLA

#ifndef PROTOC_CPPFILEGENERATOR_H_
#define PROTOC_CPPFILEGENERATOR_H_

#include <google/protobuf/descriptor.h>
#include <google/protobuf/io/printer.h>
#include <string>
#include <vector>

#include "protoc/ServiceGenerator.h"

namespace ola {

using google::protobuf::io::Printer;

class FileGenerator {
  public:
    FileGenerator(const google::protobuf::FileDescriptor *file,
                  const std::string &output_name);
    ~FileGenerator();

    void GenerateHeader(Printer *printer);
    void GenerateImplementation(Printer *printer);

  private:
    typedef std::vector<ServiceGenerator*> ServiceGenerators;

    const google::protobuf::FileDescriptor *m_file;
    const std::string m_output_name;
    std::vector<std::string> package_parts_;
    ServiceGenerators m_service_generators;

    void GenerateBuildDescriptors(Printer* printer);
    void GenerateNamespaceOpeners(Printer* printer);
    void GenerateNamespaceClosers(Printer* printer);
};

}  // namespace ola
#endif  // PROTOC_CPPFILEGENERATOR_H_
