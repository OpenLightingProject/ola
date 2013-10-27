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

#ifndef PROTOC_SERVICEGENERATOR_H_
#define PROTOC_SERVICEGENERATOR_H_

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/descriptor.h>

#include <map>
#include <string>

namespace ola {

using google::protobuf::ServiceDescriptor;
using google::protobuf::io::Printer;
using std::string;

class ServiceGenerator {
 public:
  // Generator options:
  struct Options {
    Options() : safe_boundary_check(false) {
    }
    string dllexport_decl;
    bool safe_boundary_check;
  };

  // See generator.cc for the meaning of dllexport_decl.
  explicit ServiceGenerator(const ServiceDescriptor* descriptor,
                            const Options& options);
  ~ServiceGenerator();

  // Header stuff.

  // Generate the class definitions for the service's interface and the
  // stub implementation.
  void GenerateDeclarations(Printer* printer);

  // Source file stuff.

  // Generate code that initializes the global variable storing the service's
  // descriptor.
  void GenerateDescriptorInitializer(Printer* printer, int index);

  // Generate implementations of everything declared by GenerateDeclarations().
  void GenerateImplementation(Printer* printer);

 private:
  enum RequestOrResponse { REQUEST, RESPONSE };
  enum VirtualOrNon { VIRTUAL, NON_VIRTUAL };

  // Header stuff.

  // Generate the service abstract interface.
  void GenerateInterface(Printer* printer);

  // Generate the stub class definition.
  void GenerateStubDefinition(Printer* printer);

  // Prints signatures for all methods in the
  void GenerateMethodSignatures(VirtualOrNon virtual_or_non,
                                Printer* printer);

  // Source file stuff.

  // Generate the default implementations of the service methods, which
  // produce a "not implemented" error.
  void GenerateNotImplementedMethods(Printer* printer);

  // Generate the CallMethod() method of the service.
  void GenerateCallMethod(Printer* printer);

  // Generate the Get{Request,Response}Prototype() methods.
  void GenerateGetPrototype(RequestOrResponse which, Printer* printer);

  // Generate the stub's implementations of the service methods.
  void GenerateStubMethods(Printer* printer);

  const ServiceDescriptor* descriptor_;
  std::map<string, string> vars_;

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(ServiceGenerator);
};

}  // namespace ola
#endif  // PROTOC_SERVICEGENERATOR_H_
