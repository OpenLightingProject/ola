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

#include <google/protobuf/io/printer.h>

#include <map>
#include <string>

#include "protoc/GeneratorHelpers.h"
#include "protoc/ServiceGenerator.h"
#include "protoc/StrUtil.h"

namespace ola {

using google::protobuf::ServiceDescriptor;
using google::protobuf::MethodDescriptor;
using google::protobuf::io::Printer;
using std::map;
using std::string;

ServiceGenerator::ServiceGenerator(const ServiceDescriptor* descriptor,
                                   const Options& options)
  : descriptor_(descriptor) {
  vars_["classname"] = descriptor_->name();
  vars_["full_name"] = descriptor_->full_name();
  if (options.dllexport_decl.empty()) {
    vars_["dllexport"] = "";
  } else {
    vars_["dllexport"] = options.dllexport_decl + " ";
  }
}

ServiceGenerator::~ServiceGenerator() {}

void ServiceGenerator::GenerateDeclarations(Printer* printer) {
  // Forward-declare the stub type.
  printer->Print(vars_,
    "class $classname$_Stub;\n"
    "\n");

  GenerateInterface(printer);
  GenerateStubDefinition(printer);
}

void ServiceGenerator::GenerateInterface(Printer* printer) {
  printer->Print(vars_,
    "class $dllexport$$classname$ : public ola::rpc::RpcService {\n"
    " protected:\n"
    "  // This class should be treated as an abstract interface.\n"
    "  inline $classname$() {};\n"
    " public:\n"
    "  virtual ~$classname$();\n");
  printer->Indent();

  printer->Print(vars_,
    "\n"
    "static const ::google::protobuf::ServiceDescriptor* descriptor();\n"
    "\n");

  GenerateMethodSignatures(VIRTUAL, printer);

  printer->Print(
    "\n"
    "// implements Service ----------------------------------------------\n"
    "\n"
    "const ::google::protobuf::ServiceDescriptor* GetDescriptor();\n"
    "void CallMethod(const ::google::protobuf::MethodDescriptor* method,\n"
    "                ola::rpc::RpcController* controller,\n"
    "                const ::google::protobuf::Message* request,\n"
    "                ::google::protobuf::Message* response,\n"
    "                ola::rpc::RpcService::CompletionCallback* done);\n"
    "const ::google::protobuf::Message& GetRequestPrototype(\n"
    "  const ::google::protobuf::MethodDescriptor* method) const;\n"
    "const ::google::protobuf::Message& GetResponsePrototype(\n"
    "  const ::google::protobuf::MethodDescriptor* method) const;\n");

  printer->Outdent();
  printer->Print(vars_,
    "\n"
    " private:\n"
    "  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS($classname$);\n"
    "};\n"
    "\n");
}

void ServiceGenerator::GenerateStubDefinition(Printer* printer) {
  printer->Print(vars_,
    "class $dllexport$$classname$_Stub : public $classname$ {\n"
    " public:\n");

  printer->Indent();

  printer->Print(vars_,
    "$classname$_Stub(ola::rpc::RpcChannel* channel);\n"
    "$classname$_Stub(ola::rpc::RpcChannel* channel,\n"
    "                 ::google::protobuf::Service::ChannelOwnership ownership"
    ");\n"
    "~$classname$_Stub();\n"
    "\n"
    "inline ola::rpc::RpcChannel* channel() { return channel_; }\n"
    "\n"
    "// implements $classname$ ------------------------------------------\n"
    "\n");

  GenerateMethodSignatures(NON_VIRTUAL, printer);

  printer->Outdent();
  printer->Print(vars_,
    " private:\n"
    "  ola::rpc::RpcChannel* channel_;\n"
    "  bool owns_channel_;\n"
    "  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS($classname$_Stub);\n"
    "};\n"
    "\n");
}

void ServiceGenerator::GenerateMethodSignatures(
    VirtualOrNon virtual_or_non, Printer* printer) {
  for (int i = 0; i < descriptor_->method_count(); i++) {
    const MethodDescriptor* method = descriptor_->method(i);
    map<string, string> sub_vars;
    sub_vars["name"] = method->name();
    sub_vars["input_type"] = ClassName(method->input_type(), true);
    sub_vars["output_type"] = ClassName(method->output_type(), true);
    sub_vars["virtual"] = virtual_or_non == VIRTUAL ? "virtual " : "";

    printer->Print(sub_vars,
      "$virtual$void $name$(ola::rpc::RpcController* controller,\n"
      "                     const $input_type$* request,\n"
      "                     $output_type$* response,\n"
      "                     ola::rpc::RpcService::CompletionCallback* done"
      ");\n");
  }
}

// ===================================================================

void ServiceGenerator::GenerateDescriptorInitializer(
    Printer* printer, int index) {
  map<string, string> vars;
  vars["classname"] = descriptor_->name();
  vars["index"] = SimpleItoa(index);

  printer->Print(vars,
    "$classname$_descriptor_ = file->service($index$);\n");
}

// ===================================================================

void ServiceGenerator::GenerateImplementation(Printer* printer) {
  printer->Print(vars_,
    "$classname$::~$classname$() {}\n"
    "\n"
    "const ::google::protobuf::ServiceDescriptor* $classname$::descriptor() {\n"
    "  protobuf_AssignDescriptorsOnce();\n"
    "  return $classname$_descriptor_;\n"
    "}\n"
    "\n"
    "const ::google::protobuf::ServiceDescriptor* $classname$::GetDescriptor()"
    " {\n"
    "  protobuf_AssignDescriptorsOnce();\n"
    "  return $classname$_descriptor_;\n"
    "}\n"
    "\n");

  // Generate methods of the interface.
  GenerateNotImplementedMethods(printer);
  GenerateCallMethod(printer);
  GenerateGetPrototype(REQUEST, printer);
  GenerateGetPrototype(RESPONSE, printer);

  // Generate stub implementation.
  printer->Print(vars_,
    "$classname$_Stub::$classname$_Stub(ola::rpc::RpcChannel* channel)\n"
    "  : channel_(channel), owns_channel_(false) {}\n"
    "$classname$_Stub::$classname$_Stub(\n"
    "    ola::rpc::RpcChannel* channel,\n"
    "    ::google::protobuf::Service::ChannelOwnership ownership)\n"
    "  : channel_(channel),\n"
    "    owns_channel_(ownership == "
    "::google::protobuf::Service::STUB_OWNS_CHANNEL) {}\n"
    "$classname$_Stub::~$classname$_Stub() {\n"
    "  if (owns_channel_) delete channel_;\n"
    "}\n"
    "\n");

  GenerateStubMethods(printer);
}

void ServiceGenerator::GenerateNotImplementedMethods(Printer* printer) {
  for (int i = 0; i < descriptor_->method_count(); i++) {
    const MethodDescriptor* method = descriptor_->method(i);
    map<string, string> sub_vars;
    sub_vars["classname"] = descriptor_->name();
    sub_vars["name"] = method->name();
    sub_vars["index"] = SimpleItoa(i);
    sub_vars["input_type"] = ClassName(method->input_type(), true);
    sub_vars["output_type"] = ClassName(method->output_type(), true);

    printer->Print(sub_vars,
      "void $classname$::$name$(ola::rpc::RpcController* controller,\n"
      "                         const $input_type$*,\n"
      "                         $output_type$*,\n"
      "                         ola::rpc::RpcService::CompletionCallback* done"
      ") {\n"
      "  controller->SetFailed(\"Method $name$() not implemented.\");\n"
      "  done->Run();\n"
      "}\n"
      "\n");
  }
}

void ServiceGenerator::GenerateCallMethod(Printer* printer) {
  printer->Print(vars_,
    "void $classname$::CallMethod(const ::google::protobuf::MethodDescriptor* "
    "method,\n"
    "                             ola::rpc::RpcController* controller,\n"
    "                             const ::google::protobuf::Message* request,\n"
    "                             ::google::protobuf::Message* response,\n"
    "                             ola::rpc::RpcService::CompletionCallback* "
    "done) {\n"
    "  GOOGLE_DCHECK_EQ(method->service(), $classname$_descriptor_);\n"
    "  switch(method->index()) {\n");

  for (int i = 0; i < descriptor_->method_count(); i++) {
    const MethodDescriptor* method = descriptor_->method(i);
    map<string, string> sub_vars;
    sub_vars["name"] = method->name();
    sub_vars["index"] = SimpleItoa(i);
    sub_vars["input_type"] = ClassName(method->input_type(), true);
    sub_vars["output_type"] = ClassName(method->output_type(), true);

    // Note:  down_cast does not work here because it only works on pointers,
    //   not references.
    printer->Print(sub_vars,
      "    case $index$:\n"
      "      $name$(controller,\n"
      "             ::google::protobuf::down_cast<const $input_type$*>(request"
      "),\n"
      "             ::google::protobuf::down_cast< $output_type$*>(response),\n"
      "             done);\n"
      "      break;\n");
  }

  printer->Print(vars_,
    "    default:\n"
    "      GOOGLE_LOG(FATAL) << \"Bad method index; this should never "
    "happen.\";\n"
    "      break;\n"
    "  }\n"
    "}\n"
    "\n");
}

void ServiceGenerator::GenerateGetPrototype(RequestOrResponse which,
                                            Printer* printer) {
  if (which == REQUEST) {
    printer->Print(vars_,
      "const ::google::protobuf::Message& $classname$::GetRequestPrototype(\n");
  } else {
    printer->Print(vars_,
      "const ::google::protobuf::Message& $classname$::GetResponsePrototype"
      "(\n");
  }

  printer->Print(vars_,
    "    const ::google::protobuf::MethodDescriptor* method) const {\n"
    "  GOOGLE_DCHECK_EQ(method->service(), descriptor());\n"
    "  switch(method->index()) {\n");

  for (int i = 0; i < descriptor_->method_count(); i++) {
    const MethodDescriptor* method = descriptor_->method(i);
    const google::protobuf::Descriptor* type =
      (which == REQUEST) ? method->input_type() : method->output_type();

    map<string, string> sub_vars;
    sub_vars["index"] = SimpleItoa(i);
    sub_vars["type"] = ClassName(type, true);

    printer->Print(sub_vars,
      "    case $index$:\n"
      "      return $type$::default_instance();\n");
  }

  printer->Print(vars_,
    "    default:\n"
    "      GOOGLE_LOG(FATAL) << \"Bad method index; this should never happen."
    "\";\n"
    "      return *reinterpret_cast< ::google::protobuf::Message*>(NULL);\n"
    "  }\n"
    "}\n"
    "\n");
}

void ServiceGenerator::GenerateStubMethods(Printer* printer) {
  for (int i = 0; i < descriptor_->method_count(); i++) {
    const MethodDescriptor* method = descriptor_->method(i);
    map<string, string> sub_vars;
    sub_vars["classname"] = descriptor_->name();
    sub_vars["name"] = method->name();
    sub_vars["index"] = SimpleItoa(i);
    sub_vars["input_type"] = ClassName(method->input_type(), true);
    sub_vars["output_type"] = ClassName(method->output_type(), true);

    printer->Print(sub_vars,
      "void $classname$_Stub::$name$(ola::rpc::RpcController* controller,\n"
      "                              const $input_type$* request,\n"
      "                              $output_type$* response,\n"
      "                              ola::rpc::RpcService::CompletionCallback*"
      " done) {\n"
      "  channel_->CallMethod(descriptor()->method($index$),\n"
      "                       controller, request, response, done);\n"
      "}\n");
  }
}
}  // namespace ola
